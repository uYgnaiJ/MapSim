#include "canvas.hpp"
#include <algorithm>
#include <stdexcept>

Ink mix(Ink a, Ink b, float t)
{
    Ink result = 0;
    for (int shift : {0, 8, 16})
        result |= uint32_t(((a >> shift) & 255) * (1 - t) + ((b >> shift) & 255) * t) << shift;
    return result;
}
static uint32_t packed(Ink c, float a)
{
    return ((c >> 16) & 255) | (c & 0xff00) | ((c & 255) << 16) | (uint32_t(a * 255) << 24);
}
Atlas::Atlas()
{
    dc = CreateCompatibleDC(nullptr);
    BITMAPINFO info{};
    info.bmiHeader = {sizeof(BITMAPINFOHEADER), side, -side, 1, 32, BI_RGB};
    bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, (void **)&pixels, nullptr, 0);
    if (!bitmap)
        throw std::runtime_error("Cannot create font atlas");
    oldBitmap = SelectObject(dc, bitmap);
    oldFont = GetCurrentObject(dc, OBJ_FONT);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    pixels[0] = 0x00ffffff;
}
Atlas::~Atlas()
{
    SelectObject(dc, oldFont);
    SelectObject(dc, oldBitmap);
    for (auto &[_, f] : fonts)
        DeleteObject(f);
    DeleteObject(bitmap);
    DeleteDC(dc);
}
Glyph Atlas::glyph(Font f, int size, wchar_t ch)
{
    auto key = std::make_tuple(int(f), size, ch);
    if (auto it = glyphs.find(key); it != glyphs.end())
        return it->second;
    auto &font = fonts[{int(f), size}];
    if (!font)
        font = CreateFontW(-size * 2, 0, 0, 0, f == Strong ? 600 : 400, f == Italic, 0, 0, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH,
                           f == Mono ? L"Consolas" : (f == Serif || f == Italic) ? L"Georgia"
                                                                                 : L"Segoe UI");
    SelectObject(dc, font);
    SIZE sizePx{};
    GetTextExtentPoint32W(dc, &ch, 1, &sizePx);
    const int pad = 6, w = sizePx.cx + pad * 2, h = sizePx.cy + 2;
    if (cursorX + w + 1 >= side)
    {
        cursorX = 1;
        cursorY += rowH + 2;
        rowH = 0;
    }
    if (cursorY + h >= side)
    {
        if (ch != L'?')
            return glyph(f, size, L'?');
        throw std::runtime_error("Font atlas capacity exceeded");
    }
    TextOutW(dc, cursorX + pad, cursorY, &ch, 1);
    Glyph g{cursorX, cursorY, w, h, sizePx.cx * .5f};
    cursorX += w + 2;
    rowH = std::max(rowH, h);
    dirty = true;
    return glyphs.emplace(key, g).first->second;
}
void Canvas::reset(Box full)
{
    vertices.clear();
    batches.clear();
    clip(full);
}
void Canvas::clip(Box b)
{
    clipping = b;
    batches.push_back({b, uint32_t(vertices.size()), 0});
}
static constexpr V white{.5f / Atlas::side, .5f / Atlas::side};
void Canvas::tri(V a, V b, V c, Ink ink, float opacity)
{
    auto t = packed(ink, opacity);
    vertices.insert(vertices.end(), {{a, white, t}, {b, white, t}, {c, white, t}});
    batches.back().count += 3;
}
void Canvas::quad(V a, V b, V c, V d, Ink ink, V uv0, V uv1, float opacity)
{
    auto t = packed(ink, opacity);
    Vertex v0{a, uv0, t}, v1{b, {uv1.x, uv0.y}, t}, v2{c, uv1, t}, v3{d, {uv0.x, uv1.y}, t};
    vertices.insert(vertices.end(), {v0, v1, v2, v0, v2, v3});
    batches.back().count += 6;
}
void Canvas::rect(Box b, Ink c, float a) { quad({b.x, b.y}, {b.x + b.w, b.y}, {b.x + b.w, b.y + b.h}, {b.x, b.y + b.h}, c, white, white, a); }
void Canvas::line(V a, V b, float width, Ink ink, float opacity)
{
    V diff = b - a;
    float len = std::hypot(diff.x, diff.y);
    if (len < .001f)
        return;
    V n = {-diff.y / len * width * .5f, diff.x / len * width * .5f};
    quad(a + n, b + n, b - n, a - n, ink, white, white, opacity);
}
void Canvas::circle(V p, float r, Ink c)
{
    int steps = std::clamp(int(r * 2), 12, 64);
    V prev{p.x + r, p.y};
    for (int i = 1; i <= steps; ++i)
    {
        float a = 6.2831853f * i / steps;
        V next = p + V{cosf(a), sinf(a)} * r;
        tri(p, prev, next, c);
        prev = next;
    }
}
void Canvas::round(Box b, float r, Ink c)
{
    r = std::min({r, b.w * .5f, b.h * .5f});
    rect({b.x + r, b.y, b.w - r * 2, b.h}, c);
    rect({b.x, b.y + r, r, b.h - 2 * r}, c);
    rect({b.x + b.w - r, b.y + r, r, b.h - 2 * r}, c);
    for (int j = 0; j < 4; j++)
    {
        V p{j == 0 || j == 3 ? b.x + b.w - r : b.x + r, j < 2 ? b.y + b.h - r : b.y + r};
        float a = 1.5707963f * j;
        V prev = p + V{cosf(a), sinf(a)} * r;
        for (int i = 1; i <= 6; i++)
        {
            float t = a + 1.5707963f * i / 6;
            V next = p + V{cosf(t), sinf(t)} * r;
            tri(p, prev, next, c);
            prev = next;
        }
    }
}
void Canvas::path(const std::vector<V> &pts, float w, Ink c, bool closed)
{
    for (size_t i = 1; i < pts.size(); i++)
        line(pts[i - 1], pts[i], w, c);
    if (closed && pts.size() > 2)
        line(pts.back(), pts.front(), w, c);
}
float Canvas::width(const std::wstring &s, Font f, int z, float track)
{
    float w = 0;
    for (wchar_t ch : s)
        w += atlas.glyph(f, z, ch).advance + track;
    return s.empty() ? 0 : w - track;
}
void Canvas::text(float x, float y, const std::wstring &s, Font f, int z, Ink ink, float track)
{
    for (wchar_t ch : s)
    {
        auto g = atlas.glyph(f, z, ch);
        if (ch != L' ')
        {
            float l = x - 3, r = l + g.w * .5f, b = y + g.h * .5f;
            quad({l, y}, {r, y}, {r, b}, {l, b}, ink, {float(g.x) / Atlas::side, float(g.y) / Atlas::side},
                 {float(g.x + g.w) / Atlas::side, float(g.y + g.h) / Atlas::side}, 1);
        }
        x += g.advance + track;
    }
}
void Canvas::center(float x, float y, const std::wstring &s, Font f, int z, Ink ink, float track)
{
    text(x - width(s, f, z, track) * .5f, y, s, f, z, ink, track);
}