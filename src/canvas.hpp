#pragma once
#include <windows.h>
#include <cmath>
#include <map>
#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <tuple>

struct V {
    float x{},y{};
    V operator+(V b) const {return {x+b.x,y+b.y};}
    V operator-(V b) const {return {x-b.x,y-b.y};}
    V operator*(float k) const {return {x*k,y*k};}
};
struct Box {float x,y,w,h; bool has(V p) const {return p.x>=x && p.x<x+w && p.y>=y && p.y<y+h;}};
using Ink=uint32_t; // RGB, wrapped into a packed vertex in Canvas.
Ink mix(Ink a,Ink b,float t);
struct Vertex { V pos,uv; uint32_t color; };
struct Batch { Box clip; uint32_t first,count; };
enum Font { Body, Strong, Mono, Serif, Italic };
struct Glyph { int x,y,w,h; float advance; };
class Atlas {
    HDC dc;
    HBITMAP bitmap; HGDIOBJ oldBitmap,oldFont; int cursorX=4,cursorY=1,rowH=0;
    std::map<std::pair<int,int>,HFONT> fonts;
    std::map<std::tuple<int,int,wchar_t>,Glyph> glyphs;
public:
    static constexpr int side=2048;
    uint32_t* pixels{};
    bool dirty=true;
    Atlas();
    ~Atlas();
    Glyph glyph(Font font,int size,wchar_t ch);
};
class Canvas {
    Box clipping{};
    void quad(V a,V b,V c,V d,Ink ink,V uv0,V uv1,float opacity);
public:
    Atlas atlas;
    std::vector<Vertex> vertices;
    std::vector<Batch> batches;
    void reset(Box full);
    void clip(Box rect);
    void tri(V a,V b,V c,Ink ink,float opacity=1);
    void rect(Box b,Ink ink,float opacity=1);
    void round(Box b,float radius,Ink ink);
    void line(V a,V b,float width,Ink ink,float opacity=1);
    void circle(V center,float radius,Ink ink);
    void path(const std::vector<V>& points,float width,Ink ink,bool closed=false);
    float width(const std::wstring& s,Font font,int size,float track=0);
    void text(float x,float y,const std::wstring& s,Font font,int size,Ink ink,float track=0);
    void center(float x,float y,const std::wstring& s,Font font,int size,Ink ink,float track=0);
};
