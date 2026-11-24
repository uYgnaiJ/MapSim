#include "vulkan.hpp"
#include "cartography.hpp"
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <set>
#include <sstream>

namespace fs = std::filesystem;
constexpr Ink dark = 0x243c33, muted = 0x66746b, paper = 0xf8f8f2, border = 0xd5dacf, white = 0xffffff, green = 0x34664d;
enum Action
{
    Open = 1,
    Save,
    Grid,
    Labels,
    Select,
    Pan,
    Paint,
    Fit,
    Minus,
    Plus,
    RegionMenu,
    TerrainMenu,
    RegionStart = 100
};
struct Hit
{
    Box box;
    int action;
};
struct Edit
{
    Hex hex;
    std::optional<Tile> before, after;
};

class Editor
{
public:
    HWND win{};
    bool ready = false, dirty = false, grid = true, labels = true, panning = false, painting = false;
    Renderer renderer;
    Canvas c;
    Map map;
    fs::path file;
    float W = 1200, H = 760, scale = 1, radius = 15, zoom = 1;
    V focus{}, mouse{-1, -1}, last{};
    Box area{}, side{};
    std::vector<Hit> hits;
    int tool = Select, regionScroll = 0;
    size_t active = 0;
    Terrain brush = Terrain::Plains;
    std::optional<Hex> selected;
    std::vector<std::vector<Edit>> history, redo;
    std::vector<Edit> stroke;
    struct Stats
    {
        int count{};
        V sum{};
        Hex anchor{};
        float best = 1e30f;
    };
    std::vector<Stats> stats, kingdomStats;
    float fitRadius = 15;
    V fitCenter{};
    bool boundsDirty = true;
    void invalidate() { InvalidateRect(win, nullptr, FALSE); }
    void title() { SetWindowTextW(win, ((dirty ? L"* " : L"") + wide(map.name) + L"  —  Field Atlas").c_str()); }
    void touch()
    {
        dirty = true;
        boundsDirty = true;
        title();
        invalidate();
    }
    int hit(V p) const
    {
        for (auto it = hits.rbegin(); it != hits.rend(); ++it)
            if (it->box.has(p))
                return it->action;
        return 0;
    }
    V origin() const { return {area.x + area.w * .5f, area.y + area.h * .5f}; }
    V screen(Hex h) const { return origin() + (world(h) - focus) * radius; }
    Hex pick(V p) const { return axial(focus + (p - origin()) * (1 / radius)); }
    void refreshStats()
    {
        if (!boundsDirty)
            return;
        stats.assign(map.regions.size(), {});
        kingdomStats.assign(map.kingdoms.size(), {});
        auto accumulate = [](Stats &st, V p)
        {
            st.count++;
            st.sum = st.sum + p;
        };
        auto anchor = [](Stats &st, Hex h, V p)
        {
            V d = p - st.sum * (1.f / st.count);
            float dist = d.x * d.x + d.y * d.y;
            if (dist < st.best)
            {
                st.best = dist;
                st.anchor = h;
            }
        };
        for (auto &[h, t] : map.tiles)
        {
            V p = world(h);
            accumulate(stats[t.region], p);
            if (auto k = map.regions[t.region].kingdom)
                accumulate(kingdomStats[*k], p);
        }
        for (auto &[h, t] : map.tiles)
        {
            V p = world(h);
            anchor(stats[t.region], h, p);
            if (auto k = map.regions[t.region].kingdom)
                anchor(kingdomStats[*k], h, p);
        }
        boundsDirty = false;
    }
    void fit()
    {
        float minX = 0, maxX = 0, minY = 0, maxY = 0;
        bool first = true;
        for (auto &[h, _] : map.tiles)
        {
            V p = world(h);
            if (first)
            {
                minX = maxX = p.x;
                minY = maxY = p.y;
                first = false;
            }
            minX = std::min(p.x, minX);
            maxX = std::max(p.x, maxX);
            minY = std::min(p.y, minY);
            maxY = std::max(p.y, maxY);
        }
        fitCenter = {(minX + maxX) * .5f, (minY + maxY) * .5f};
        float horizontal = (area.w - 86) / std::max(12.f, maxX - minX + 2);
        float vertical = (area.h - 83) / std::max(10.f, maxY - minY + 2);
        fitRadius = std::clamp(std::min(horizontal, vertical), .0001f, 31.f);
        radius = fitRadius;
        focus = fitCenter;
        zoom = 1;
    }
    void layout()
    {
        float pad = W < 1000 ? 18 : 28, sideW = W < 1080 ? 240 : 278, top = 152, bottom = H - 47;
        side = {W - pad - sideW, top, sideW, bottom - top};
        area = {pad + 1, top + 43, side.x - pad - 16 - 2, bottom - top - 44};
    }
    void load(const fs::path &next)
    {
        auto loaded = loadMap(next);
        map = std::move(loaded);
        file = next;
        dirty = false;
        boundsDirty = true;
        active = 0;
        selected.reset();
        history.clear();
        redo.clear();
        stroke.clear();
        regionScroll = 0;
        tool = Select;
        brush = Terrain::Plains;
        refreshStats();
        if (!map.tiles.empty())
        {
            selected = map.tiles.contains(Hex{0, 0}) ? Hex{0, 0} : map.tiles.begin()->first;
            active = map.tiles.at(*selected).region;
        }
        fit();
        title();
        invalidate();
    }
    fs::path dialog(bool writing)
    {
        wchar_t name[32768]{};
        if (writing)
            wcsncpy_s(name, file.filename().c_str(), _TRUNCATE);
        auto dir = file.parent_path().wstring();
        OPENFILENAMEW of{sizeof(of)};
        of.hwndOwner = win;
        of.lpstrFilter = L"Field Atlas maps (*.atlas)\0*.atlas\0All files\0*.*\0";
        of.lpstrFile = name;
        of.nMaxFile = 32768;
        of.lpstrInitialDir = dir.c_str();
        of.lpstrDefExt = L"atlas";
        of.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (writing ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
        return (writing ? GetSaveFileNameW(&of) : GetOpenFileNameW(&of)) ? fs::path(name) : fs::path{};
    }
    void error(const std::exception &e) { MessageBoxW(win, wide(e.what()).c_str(), L"Field Atlas", MB_OK | MB_ICONERROR); }
    bool save(bool as = false)
    {
        auto dest = as ? dialog(true) : file;
        if (dest.empty())
            return false;
        try
        {
            saveMap(map, dest);
            file = dest;
            dirty = false;
            title();
            invalidate();
            return true;
        }
        catch (const std::exception &e)
        {
            error(e);
            return false;
        }
    }
    bool leave()
    {
        if (!dirty)
            return true;
        int answer = MessageBoxW(win, L"Save your changes before continuing?", L"Field Atlas", MB_YESNOCANCEL | MB_ICONQUESTION);
        return answer == IDNO || (answer == IDYES && save());
    }
    void setZoom(float factor, V anchor)
    {
        float next = std::clamp(radius * factor, std::min(fitRadius, .8f), 85.f);
        V offset = anchor - origin();
        focus = focus + offset * (1 / radius - 1 / next);
        radius = next;
        zoom = radius / fitRadius;
        invalidate();
    }
    void apply(Hex h, std::optional<Tile> value)
    {
        if (value)
            map.tiles[h] = *value;
        else
            map.tiles.erase(h);
    }
    void edit(Hex h, std::optional<Tile> next)
    {
        if (std::abs(h.q) > 100000 || std::abs(h.r) > 100000)
            return;
        auto it = map.tiles.find(h);
        std::optional<Tile> old;
        if (it != map.tiles.end())
            old = it->second;
        if (old == next)
            return;
        stroke.push_back({h, old, next});
        apply(h, next);
        redo.clear();
        touch();
    }
    void finish()
    {
        if (!stroke.empty())
        {
            history.push_back(std::move(stroke));
            stroke.clear();
        }
    }
    void undo(bool forward)
    {
        finish();
        auto &from = forward ? redo : history;
        auto &to = forward ? history : redo;
        if (from.empty())
            return;
        auto changes = std::move(from.back());
        from.pop_back();
        if (forward)
        {
            for (auto &e : changes)
                apply(e.hex, e.after);
        }
        else
        {
            for (auto it = changes.rbegin(); it != changes.rend(); ++it)
                apply(it->hex, it->before);
        }
        to.push_back(std::move(changes));
        touch();
    }
    void chooseRegion(size_t i)
    {
        active = i;
        if (tool != Paint)
        {
            refreshStats();
            if (stats[i].count)
                selected = stats[i].anchor;
            else
                selected.reset();
        }
        invalidate();
    }
    void popup(bool chooseTerrain)
    {
        HMENU menu = CreatePopupMenu();
        std::optional<Tile> tile;
        if (selected && map.tiles.contains(*selected))
            tile = map.tiles.at(*selected);
        bool editing = tool != Paint;
        if (editing && !tile)
        {
            DestroyMenu(menu);
            return;
        }
        size_t current = chooseTerrain ? size_t(editing ? tile->terrain : brush) : (editing ? tile->region : active);
        size_t count = chooseTerrain ? terrainNames.size() : map.regions.size();
        for (size_t i = 0; i < count; i++)
            AppendMenuW(menu, MF_STRING | (i == current ? MF_CHECKED : 0), i + 1, wide(chooseTerrain ? terrainNames[i] : map.regions[i].name).c_str());
        POINT pos;
        GetCursorPos(&pos);
        int id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pos.x, pos.y, 0, win, nullptr);
        DestroyMenu(menu);
        if (!id)
            return;
        if (chooseTerrain)
        {
            if (editing)
                tile->terrain = Terrain(id - 1);
            else
                brush = Terrain(id - 1);
        }
        else
        {
            active = id - 1;
            if (editing)
                tile->region = active;
        }
        if (editing)
        {
            edit(*selected, tile);
            finish();
        }
        invalidate();
    }
    void action(int id)
    {
        if (id >= RegionStart)
        {
            chooseRegion(id - RegionStart);
            return;
        }
        switch (id)
        {
        case Open:
        {
            auto next = dialog(false);
            if (!next.empty() && leave())
                try
                {
                    load(next);
                }
                catch (const std::exception &e)
                {
                    error(e);
                }
            break;
        }
        case Save:
            save();
            break;
        case Grid:
            grid = !grid;
            break;
        case Labels:
            labels = !labels;
            break;
        case Select:
        case Pan:
        case Paint:
            tool = id;
            break;
        case Fit:
            fit();
            break;
        case Minus:
            setZoom(1 / 1.25f, origin());
            break;
        case Plus:
            setZoom(1.25f, origin());
            break;
        case RegionMenu:
        case TerrainMenu:
            popup(id == TerrainMenu);
            break;
        }
        invalidate();
    }
    void text(float x, float y, const std::wstring &s, Font f, int z, Ink i, float track = 0) { c.text(x, y, s, f, z, i, track); }
    void capped(float x, float y, float maxWidth, std::wstring s, Font f, int size, Ink color)
    {
        if (c.width(s, f, size) > maxWidth)
        {
            while (!s.empty() && c.width(s + L"…", f, size) > maxWidth)
                s.pop_back();
            s += L"…";
        }
        text(x, y, s, f, size, color);
    }
    void btn(Box b, int id, Icon ic, const std::wstring &label, Ink bg, Ink fg)
    {
        hits.push_back({b, id});
        c.round(b, 5, b.has(mouse) ? mix(bg, fg, .09f) : bg);
        float contents = c.width(label, Strong, 13) + 24, left = b.x + (b.w - contents) * .5f;
        icon(c, ic, {left + 9, b.y + b.h * .5f}, 17, fg);
        text(left + 24, b.y + (b.h - 18) * .5f, label, Strong, 13, fg);
    }
    void outline(Box b, Ink edge, Ink fill, float r = 5)
    {
        c.round(b, r, edge);
        c.round({b.x + 1, b.y + 1, b.w - 2, b.h - 2}, r - 1, fill);
    }
    void mapView();
    void panel();
    void draw();
    void down(V p, bool middle)
    {
        mouse = p;
        last = p;
        int id = hit(p);
        if (id && !middle)
        {
            action(id);
            return;
        }
        if (!area.has(p))
            return;
        if (middle || tool == Pan || GetKeyState(VK_SPACE) < 0)
        {
            panning = true;
            SetCapture(win);
        }
        else if (tool == Paint)
        {
            painting = true;
            SetCapture(win);
            selected = pick(p);
            edit(*selected, Tile{active, brush});
        }
        else
        {
            selected = pick(p);
            auto it = map.tiles.find(*selected);
            if (it != map.tiles.end())
            {
                active = it->second.region;
                brush = it->second.terrain;
            }
        }
        invalidate();
    }
    void move(V p)
    {
        if (panning)
            focus = focus - (p - last) * (1 / radius);
        if (painting && area.has(p))
        {
            Hex h = pick(p);
            if (selected)
            {
                V a = world(*selected), b = world(h);
                int steps = std::clamp(int(std::ceil(std::hypot(b.x - a.x, b.y - a.y))), 1, 800);
                for (int i = 1; i <= steps; i++)
                    edit(axial(a + (b - a) * (float(i) / steps)), Tile{active, brush});
            }
            selected = h;
            edit(h, Tile{active, brush});
        }
        last = mouse = p;
        invalidate();
    }
    void up()
    {
        panning = painting = false;
        finish();
        if (GetCapture() == win)
            ReleaseCapture();
    }
    void wheel(V p, int delta)
    {
        if (side.has(p))
        {
            int visible = visibleRows();
            regionScroll = std::clamp(regionScroll + (delta > 0 ? -1 : 1), 0, std::max(0, int(map.regions.size()) - visible));
            invalidate();
        }
        else if (area.has(p))
            setZoom(powf(1.16f, delta / 120.f), p);
    }
    int visibleRows() const { return std::max(1, int((side.h - (map.kingdoms.empty() ? 235 : 282)) / 42)); }
};

void Editor::mapView()
{
    const Ink sea = 0xe0eae3, wave = 0xa0b9ab;
    c.clip(area);
    c.rect(area, sea);
    // Use world-anchored marks so water moves with the land when panning.
    const bool showEmptyGrid = grid && radius >= 5 && (map.tiles.empty() || tool == Paint);
    int spacing = showEmptyGrid ? 1 : std::max(1, int(ceilf(16 / radius)));
    float lattice = spacing * radius;
    V corner = focus + (V{area.x, area.y} - origin()) * (1 / radius);
    int firstR = int(floorf(corner.y / (spacing * 1.5f))) - 1;
    int rows = int(area.h / (1.5f * lattice)) + 4, cols = int(area.w / (root3 * lattice)) + 4;
    for (int r = firstR; r < firstR + rows; r++)
    {
        int firstQ = int(floorf(corner.x / (spacing * root3) - r * .5f)) - 1;
        for (int q = firstQ; q < firstQ + cols; q++)
        {
            Hex h{q * spacing, r * spacing};
            V p = screen(h);
            uint32_t n = noise(h);
            if (showEmptyGrid)
                hexLine(c, p, lattice, .55f, 0xc2d4c8);
            if (n % 19 == 0)
            {
                float s = std::clamp(lattice / 17.f, .6f, 1.4f);
                for (int i = 0; i < 2; i++)
                {
                    float x = p.x + i * 4 * s, y = p.y + i * 3 * s;
                    c.path({{x - 5 * s, y}, {x - 3 * s, y - .8f * s}, {x, y + .1f * s}, {x + 3 * s, y - .8f * s}, {x + 5 * s, y}}, .7f, wave);
                }
            }
            else if (n % 3 == 0)
                c.circle(p, .65f, 0xc3d5c8);
        }
    }
    auto inView = [&](V p)
    { return p.x > area.x - radius * 1.7f && p.x < area.x + area.w + radius * 1.7f && p.y > area.y - radius * 1.7f && p.y < area.y + area.h + radius * 1.7f; };
    for (auto &[h, t] : map.tiles)
    {
        V p = screen(h);
        if (inView(p))
            hexagon(c, p, radius + std::clamp(radius * .30f, 1.f, 6.f), 0xc0d5c5);
    }
    for (auto &[h, t] : map.tiles)
    {
        V p = screen(h);
        if (inView(p))
            hexagon(c, p, radius + std::clamp(radius * .15f, .8f, 3.f), 0xf2efe0);
    }
    for (auto &[h, t] : map.tiles)
    {
        V p = screen(h);
        if (!inView(p))
            continue;
        uint32_t n = noise(h);
        Ink base = map.regions[t.region].color;
        Ink fill = mix(base, n % 2 ? 0xf7f2d9 : 0x4c5d48, (n % 17) / 150.f);
        hexagon(c, p, radius + .08f, fill);
    }
    for (auto &[h, t] : map.tiles)
    {
        V p = screen(h);
        if (!inView(p))
            continue;
        Ink base = map.regions[t.region].color;
        uint32_t n = noise(h);
        for (int i = 0; i < 6; i++)
        {
            auto a = adjacent[i];
            auto nb = map.tiles.find({h.q + a.q, h.r + a.r});
            bool coast = nb == map.tiles.end(), boundary = !coast && nb->second.region != t.region;
            bool kingdomBorder = boundary && map.regions[t.region].kingdom != map.regions[nb->second.region].kingdom;
            if (!coast && i >= 3)
                continue;
            if (coast || boundary || (grid && radius >= 4))
                c.line(p + corners[i] * radius, p + corners[(i + 1) % 6] * radius,
                       coast || kingdomBorder ? 1.35f : boundary ? (map.kingdoms.empty() ? 1.25f : .65f) : .65f,
                       coast ? 0x647e65 : kingdomBorder ? 0x42584d : boundary ? 0x768264 : mix(base, 0x465343, .26f));
        }
        if (radius > 4 && (t.terrain != Terrain::Plains || n % 4 == 0))
            landSymbol(c, p, radius, t.terrain, mix(base, 0x293c2e, .57f), n);
    }
    if (labels && radius >= (map.kingdoms.empty() ? 4 : 12))
    {
        for (size_t i = 0; i < stats.size(); i++)
        {
            auto &st = stats[i];
            if (!st.count)
                continue;
            V p = screen(st.anchor);
            if (!inView(p))
                continue;
            int z = radius < 10 ? 12 : radius < 17 ? 15
                                                   : 18;
            auto name = wide(map.regions[i].name);
            float width = c.width(name, Italic, z, .5f);
            if (width > area.w * .65f)
                continue;
            // A light cartographic halo preserves the terrain under each label.
            for (auto d : {V{-1.4f, 0}, V{1.4f, 0}, V{0, -1.4f}, V{0, 1.4f}, V{-1, 1}, V{1, -1}})
                c.center(p.x + d.x, p.y - z * .62f + d.y, name, Italic, z, 0xf1edda, .5f);
            c.center(p.x, p.y - z * .62f, name, Italic, z, 0x344336, .5f);
        }
    }
    if (labels)
    {
        for (size_t i = 0; i < kingdomStats.size(); ++i)
        {
            auto &st = kingdomStats[i];
            if (!st.count)
                continue;
            V p = screen(st.anchor);
            if (!area.has(p))
                continue;
            const auto &kingdom = map.kingdoms[i];
            auto name = wide(kingdom.name), ruler = wide(kingdom.ruler);
            float width = std::max(c.width(name, Serif, 16), c.width(ruler, Strong, 11)) + 16;
            outline({p.x - width / 2, p.y - 20, width, 40}, 0xacb6a4, 0xf8f6e9);
            c.center(p.x, p.y - 17, name, Serif, 16, dark);
            c.center(p.x, p.y + 3, ruler, Strong, 11, muted);
        }
    }
    if (selected && inView(screen(*selected)))
    {
        auto p = screen(*selected);
        hexLine(c, p, radius - 1, 3, 0xf7f5e3);
        hexLine(c, p, radius, 1.6f, 0x254b38);
    }
    if (area.has(mouse) && !hit(mouse) && !panning)
    {
        Hex h = pick(mouse);
        hexLine(c, screen(h), radius, 1.7f, 0x52765e);
    }
    compass(c, {area.x + area.w - 41, area.y + 54});
    float tx = area.x + 16, ty = area.y + 16;
    outline({tx, ty, 39, 112}, 0xc2cec2, 0xf9faf3, 6);
    int ids[] = {Select, Pan, Paint};
    Icon icons[] = {Icon::Select, Icon::Hand, Icon::Brush};
    for (int i = 0; i < 3; i++)
    {
        Box b{tx + 3, ty + 3 + i * 36.f, 33, 33};
        hits.push_back({b, ids[i]});
        bool chosen = tool == ids[i];
        if (chosen || b.has(mouse))
            c.round(b, 4, chosen ? 0xd5e4d5 : 0xe7eee4);
        icon(c, icons[i], {b.x + 16.5f, b.y + 16.5f}, 17, chosen ? green : muted);
    }
    float by = area.y + area.h - 16, bx = area.x + area.w - 15 - 151;
    outline({bx, by - 33, 151, 33}, 0xc2cec2, 0xf9faf3, 5);
    auto toolButton = [&](float x, float width, int id, Icon ic)
    {Box b{x,by-32,width,31};hits.push_back({b,id});if(b.has(mouse))c.round(b,3,0xe1e9df);icon(c,ic,{x+width/2,by-16},15,dark); };
    toolButton(bx + 1, 30, Minus, Icon::Minus);
    c.center(bx + 56, by - 24, std::to_wstring(int(std::round(zoom * 100))) + L"%", Mono, 12, dark);
    toolButton(bx + 84, 30, Plus, Icon::Plus);
    c.rect({bx + 115, by - 26, 1, 19}, border);
    toolButton(bx + 117, 32, Fit, Icon::Fit);
    int step = 1;
    while (root3 * radius * step < 39 && step < 100000)
        step *= 2;
    float sx = area.x + 18, len = root3 * radius * step;
    text(sx, by - 23, std::to_wstring(step) + (step == 1 ? L" hex" : L" hexes"), Mono, 10, 0x536e60);
    c.line({sx, by - 4}, {sx + len, by - 4}, 1.3f, 0x627b69);
    for (float x : {sx, sx + len / 2, sx + len})
        c.line({x, by - 4}, {x, by - 9}, 1.f, 0x627b69);
}

void Editor::panel()
{
    c.clip(side);
    c.rect(side, white);
    float x = side.x + 18, y = side.y + 17, width = side.w - 36;
    icon(c, Icon::Layers, {x + 9, y + 11}, 19, green);
    text(x + 28, y, L"Region index", Strong, 17, dark);
    auto count = std::to_wstring(map.regions.size());
    c.round({side.x + side.w - 44, y + 1, 25, 21}, 5, 0xeef1e9);
    c.center(side.x + side.w - 31.5f, y + 3, count, Mono, 12, muted);
    c.rect({side.x + 1, y + 35, side.w - 2, 1}, border);
    y += 47;
    int n = std::min(int(stats.size()), visibleRows());
    regionScroll = std::clamp(regionScroll, 0, std::max(0, int(stats.size()) - n));
    for (int j = 0; j < n; j++)
    {
        int i = j + regionScroll;
        Box b{side.x + 9, y + j * 42, side.w - 18, 38};
        hits.push_back({b, RegionStart + i});
        if (size_t(i) == active || b.has(mouse))
            c.round(b, 5, size_t(i) == active ? 0xeaf0e5 : 0xf2f5ef);
        V pos{x + 12, b.y + 19};
        hexagon(c, pos, 10, map.regions[i].color);
        hexLine(c, pos, 10, .8f, mix(map.regions[i].color, dark, .25f));
        capped(x + 33, b.y + 9, width - 84, wide(map.regions[i].name), Strong, 14, dark);
        auto quant = std::to_wstring(stats[i].count);
        text(side.x + side.w - 20 - c.width(quant, Mono, 12), b.y + 11, quant, Mono, 12, muted);
    }
    y += n * 42 + 12;
    c.rect({x, y, width, 1}, border);
    y += 17;
    text(x, y, tool == Paint ? L"BRUSH SETTINGS" : L"HEX INSPECTOR", Strong, 10, muted, 1.5f);
    y += 26;
    std::optional<Tile> tile;
    if (selected && map.tiles.contains(*selected))
        tile = map.tiles.at(*selected);
    auto region = tool == Paint ? active : tile ? tile->region
                                                : active;
    auto terrain = tool == Paint ? brush : tile ? tile->terrain
                                                : Terrain::Plains;
    if (tool != Paint && !tile)
    {
        text(x, y, selected ? L"Empty hex" : L"No hex selected", Serif, 19, dark);
        text(x, y + 32, L"Use the brush to add a hex.", Body, 13, muted);
        return;
    }
    Box badge{x, y, 38, 38};
    c.round(badge, 6, mix(map.regions[region].color, white, .7f));
    hexagon(c, {x + 19, y + 19}, 12, map.regions[region].color);
    landSymbol(c, {x + 19, y + 19}, 13, terrain, dark, 1);
    text(x + 48, y - 1, tool == Paint ? L"Terrain brush" : L"Selected hex", Strong, 14, dark);
    std::wstring position = tool == Paint ? L"Drag to paint" : map.grid ? L"ID " + wide(formatUnitId(makeUnitId(*selected)))
                                                                       : L"q " + std::to_wstring(selected->q) + L"   r " + std::to_wstring(selected->r);
    text(x + 48, y + 18, position, Mono, 11, muted);
    y += 49;
    auto field = [&](const wchar_t *label, std::wstring value, int id)
    {
        Box b{x, y, width, 29};
        hits.push_back({b, id});
        if (b.has(mouse))
            c.round(b, 3, 0xf0f4ec);
        text(x, y + 5, label, Body, 13, muted);
        float right = x + width - 17;
        float tw = std::min(width - 70, c.width(value, Strong, 13));
        capped(right - tw, y + 5, tw + .1f, value, Strong, 13, dark);
        icon(c, Icon::Chevron, {x + width - 5, y + 15}, 13, muted);
        y += 29;
    };
    field(L"Region", wide(map.regions[region].name), RegionMenu);
    field(L"Terrain", wide(terrainNames[int(terrain)]), TerrainMenu);
    if (auto k = map.regions[region].kingdom)
    {
        const auto &kingdom = map.kingdoms[*k];
        capped(x, y + 3, width, wide(kingdom.name), Strong, 13, dark);
        capped(x, y + 22, width, wide(kingdom.ruler), Body, 12, muted);
    }
}
void Editor::draw()
{
    RECT rc{};
    GetClientRect(win, &rc);
    if (!rc.right || !rc.bottom)
        return;
    float nextW = rc.right / scale, nextH = rc.bottom / scale;
    if (nextW != W || nextH != H)
    {
        bool atFit = zoom == 1 && std::abs(focus.x - fitCenter.x) < .01f && std::abs(focus.y - fitCenter.y) < .01f;
        W = nextW;
        H = nextH;
        layout();
        if (atFit)
            fit();
    }
    refreshStats();
    Box full{0, 0, W, H};
    c.reset(full);
    hits.clear();
    c.rect(full, paper);
    c.rect({0, 0, W, 56}, dark);
    float pad = area.x - 1;
    V logo{pad + 14, 28};
    hexLine(c, logo, 15, 1.5f, 0xe5eacb);
    c.path({logo + V{-10, 4}, logo + V{-4, -4}, logo + V{0, 0}, logo + V{4, -6}, logo + V{10, 3}}, 1.55f, 0xe5eacb);
    c.path({logo + V{-8, 7}, logo + V{-2, 3}, logo + V{3, 6}, logo + V{8, 4}}, 1.15f, 0xe5eacb);
    text(pad + 39, 13, L"Field Atlas", Serif, 23, 0xf6f6e8);
    c.rect({pad + 159, 19, 1, 18}, 0x587163);
    text(pad + 175, 20, L"Map editor", Body, 13, 0xc6d2c5);
    btn({W - pad - 214, 11, 101, 34}, Open, Icon::Folder, L"Open map", 0x3c5147, 0xf5f6ec);
    btn({W - pad - 103, 11, 103, 34}, Save, Icon::Save, L"Save map", 0xe6eccf, dark);
    text(pad, 71, L"MAP WORKSPACE", Strong, 10, 0x65765e, 1.7f);
    capped(pad, 89, W - pad * 2 - 218, wide(map.name), Serif, 32, dark);
    icon(c, Icon::File, {pad + 6, 133}, 13, muted);
    capped(pad + 18, 124, W - pad * 2 - 205, file.filename().wstring(), Mono, 12, muted);
    auto metric = [&](float x, std::wstring value, const wchar_t *label)
    {
        text(x, 85, value, Serif, 24, dark);
        text(x, 119, label, Strong, 10, muted, 1.2f);
    };
    metric(W - pad - 170, std::to_wstring(map.tiles.size()), L"HEX UNITS");
    c.rect({W - pad - 79, 92, 1, 34}, border);
    metric(W - pad - 58, std::to_wstring(map.regions.size()), L"REGIONS");
    Box card{area.x - 1, side.y, area.w + 2, side.h};
    outline(card, border, white, 6);
    icon(c, Icon::Hex, {area.x + 21, side.y + 21}, 16, green);
    text(area.x + 36, side.y + 11, L"Map canvas", Strong, 15, dark);
    auto toggle = [&](float x, int id, bool on, const wchar_t *label, Icon ic, float width)
    {
        Box b{x, side.y + 8, width, 27};
        hits.push_back({b, id});
        if (b.has(mouse))
            c.round(b, 4, 0xf0f4eb);
        icon(c, ic, {x + 9, side.y + 21.5f}, 14, on ? green : 0x92998e);
        text(x + 21, side.y + 12, label, Body, 12, on ? dark : muted);
        c.round({x + width - 20, side.y + 16, 17, 10}, 5, on ? 0x62826a : 0xa0a69c);
        c.circle({x + width - (on ? 8 : 15), side.y + 21}, 3.2f, white);
    };
    toggle(area.x + area.w - 167, Grid, grid, L"Grid", Icon::Grid, 68);
    toggle(area.x + area.w - 87, Labels, labels, L"Labels", Icon::Eye, 77);
    c.rect({area.x, area.y - 1, area.w, 1}, border);
    mapView();
    c.clip(full);
    outline(side, border, white, 6);
    Box ext = side;
    side = {ext.x + 1, ext.y + 1, ext.w - 2, ext.h - 2};
    panel();
    side = ext;
    c.clip(full);
    c.rect({0, H - 32, W, 1}, border);
    c.rect({0, H - 31, W, 31}, 0xf1f3eb);
    c.circle({pad + 4, H - 15}, 3.3f, dirty ? 0xb08847 : 0x698d65);
    text(pad + 15, H - 23, dirty ? L"Unsaved changes" : L"Saved to disk", Body, 12, muted);
    text(pad + 148, H - 23, tool == Paint ? L"Paint  ·  Drag to add or recolor hexes" : tool == Pan ? L"Pan  ·  Drag to move the map"
                                                                                                    : L"Select  ·  Click a hex to inspect",
         Body, 12, muted);
    auto coord = area.has(mouse) ? std::optional<Hex>(pick(mouse)) : selected;
    auto pos = coord ? L"Q " + std::to_wstring(coord->q) + L"   R " + std::to_wstring(coord->r) : L"Axial grid";
    text(W - pad - c.width(pos, Mono, 11), H - 22, pos, Mono, 11, muted);
    renderer.draw(c, rc.right, rc.bottom, W, H);
}
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto *e = reinterpret_cast<Editor *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE)
    {
        e = static_cast<Editor *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
        e->win = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, LONG_PTR(e));
    }
    if (!e)
        return DefWindowProcW(hwnd, msg, wp, lp);
    auto point = [&]()
    { return V{GET_X_LPARAM(lp) / e->scale, GET_Y_LPARAM(lp) / e->scale}; };
    try
    {
        switch (msg)
        {
        case WM_GETMINMAXINFO:
        {
            auto *m = reinterpret_cast<MINMAXINFO *>(lp);
            m->ptMinTrackSize = {LONG(780 * e->scale), LONG(615 * e->scale)};
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            if (e->ready)
                e->draw();
            return 0;
        }
        case WM_SIZE:
            e->invalidate();
            return 0;
        case WM_DPICHANGED:
        {
            e->scale = HIWORD(wp) / 96.f;
            auto *r = reinterpret_cast<RECT *>(lp);
            SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&track);
            e->move(point());
            return 0;
        }
        case WM_MOUSELEAVE:
            e->mouse = {-1, -1};
            e->invalidate();
            return 0;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
            SetFocus(hwnd);
            e->down(point(), msg == WM_MBUTTONDOWN);
            return 0;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_CAPTURECHANGED:
            e->up();
            return 0;
        case WM_MOUSEWHEEL:
        {
            POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ScreenToClient(hwnd, &p);
            e->wheel({p.x / e->scale, p.y / e->scale}, GET_WHEEL_DELTA_WPARAM(wp));
            return 0;
        }
        case WM_SETCURSOR:
            if (LOWORD(lp) == HTCLIENT)
            {
                auto cursor = e->hit(e->mouse) ? IDC_HAND : e->area.has(e->mouse) ? (e->tool == Pan || e->panning ? IDC_SIZEALL : e->tool == Paint ? IDC_CROSS
                                                                                                                                                   : IDC_ARROW)
                                                                                  : IDC_ARROW;
                SetCursor(LoadCursorW(nullptr, cursor));
                return TRUE;
            }
            break;
        case WM_KEYDOWN:
        {
            bool ctrl = GetKeyState(VK_CONTROL) < 0, shift = GetKeyState(VK_SHIFT) < 0;
            if (ctrl && wp == 'S')
                e->save(shift);
            else if (ctrl && wp == 'O')
                e->action(Open);
            else if (ctrl && (wp == 'Z' || wp == 'Y'))
                e->undo(wp == 'Y' || shift);
            else if (wp == 'V')
                e->action(Select);
            else if (wp == 'H')
                e->action(Pan);
            else if (wp == 'B')
                e->action(Paint);
            else if (wp == 'G')
                e->action(Grid);
            else if (wp == 'L')
                e->action(Labels);
            else if (wp == 'F')
                e->action(Fit);
            else if (wp == VK_DELETE && e->selected)
            {
                e->edit(*e->selected, std::nullopt);
                e->finish();
            }
            else if (wp == VK_OEM_PLUS || wp == VK_ADD)
                e->action(Plus);
            else if (wp == VK_OEM_MINUS || wp == VK_SUBTRACT)
                e->action(Minus);
            return 0;
        }
        case WM_CLOSE:
            if (e->leave())
                DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
    }
    catch (const std::exception &ex)
    {
        e->ready = false;
        e->error(ex);
        PostQuitMessage(1);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    try
    {
        Editor app;
        WNDCLASSW wc{};
        wc.hInstance = instance;
        wc.lpfnWndProc = windowProc;
        wc.lpszClassName = L"FieldAtlasEditor";
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        if (!RegisterClassW(&wc))
            throw std::runtime_error("Window class registration failed");
        app.scale = GetDpiForSystem() / 96.f;
        RECT desktop{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &desktop, 0);
        int dw = desktop.right - desktop.left, dh = desktop.bottom - desktop.top;
        int w = std::min(dw, int(1350 * app.scale)), h = std::min(dh, int(850 * app.scale));
        HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Field Atlas", WS_OVERLAPPEDWINDOW, desktop.left + (dw - w) / 2, desktop.top + (dh - h) / 2, w, h, nullptr, nullptr, instance, &app);
        if (!hwnd)
            throw std::runtime_error("Cannot create the editor window");
        RECT rc{};
        GetClientRect(hwnd, &rc);
        app.W = rc.right / app.scale;
        app.H = rc.bottom / app.scale;
        app.layout();
        wchar_t exe[32768];
        GetModuleFileNameW(nullptr, exe, 32768);
        auto dir = fs::path(exe).parent_path();
        auto source = dir / L"maps" / L"avelin-coast.atlas";
        if (!fs::exists(source))
            source = dir.parent_path() / L"maps" / L"avelin-coast.atlas";
        int argc{};
        auto args = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argc > 1)
            source = fs::absolute(args[1]);
        LocalFree(args);
        app.load(source);
        app.renderer.init(hwnd);
        app.ready = true;
        ShowWindow(hwnd, show);
        UpdateWindow(hwnd);
        MSG m{};
        while (GetMessageW(&m, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
        return int(m.wParam);
    }
    catch (const std::exception &e)
    {
        MessageBoxW(nullptr, wide(e.what()).c_str(), L"Field Atlas could not start", MB_OK | MB_ICONERROR);
        return 1;
    }
}
