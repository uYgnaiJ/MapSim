#pragma once
#include "canvas.hpp"
#include "model.hpp"
#include <algorithm>
#include <optional>

inline constexpr float root3=1.7320508f;
inline constexpr std::array<V,6> corners={V{0,-1},V{root3/2,-.5},V{root3/2,.5},V{0,1},V{-root3/2,.5},V{-root3/2,-.5}};
inline constexpr std::array<Hex,6> adjacent={Hex{1,-1},Hex{1,0},Hex{0,1},Hex{-1,1},Hex{-1,0},Hex{0,-1}};
inline V world(Hex h){return {root3*(h.q+h.r*.5f),1.5f*h.r};}
inline Hex axial(V v){
    float r=v.y/1.5f,q=v.x/root3-r*.5f,s=-q-r;
    int rq=int(std::round(q)),rr=int(std::round(r)),rs=int(std::round(s));
    float dq=std::abs(rq-q),dr=std::abs(rr-r),ds=std::abs(rs-s);
    if(dq>dr && dq>ds)rq=-rr-rs;else if(dr>ds)rr=-rq-rs;
    return {rq,rr};
}
inline uint32_t noise(Hex h){
    uint32_t n=uint32_t(h.q)*0x9e3779b1u+uint32_t(h.r)*0x85ebca77u;
    n^=n>>16;n*=0x7feb352du;n^=n>>15;return n;
}
inline void hexagon(Canvas& c,V p,float r,Ink fill) {
    for(int i=0;i<6;i++)c.tri(p,p+corners[i]*r,p+corners[(i+1)%6]*r,fill);
}
inline void hexLine(Canvas& c,V p,float r,float width,Ink ink) {
    for(int i=0;i<6;i++)c.line(p+corners[i]*r,p+corners[(i+1)%6]*r,width,ink);
}
inline void landSymbol(Canvas& c,V p,float radius,Terrain terrain,Ink color,uint32_t n) {
    float scale=radius/15;
    auto pt=[&](float x,float y){return p+V{x,y}*scale;};
    auto stroke=[&](float x,float y,float a,float b,float w=.85f){c.line(pt(x,y),pt(a,b),std::max(.56f,w*scale),color);};
    auto mountain=[&](float x,float y,float s){
        auto a=pt(x-5*s,y+3*s),b=pt(x,y-6*s),d=pt(x+6*s,y+3*s);
        c.tri(b,d,pt(x+1*s,y+2*s),color,.35f);
        c.path({a,b,d},std::max(.7f,scale*.9f),color);
        stroke(x,y-6*s,x-.9f*s,y-2*s);stroke(x-.9f*s,y-2*s,x+1*s,y-2.8f*s);
    };
    auto tree=[&](float x,float y,float s) {
        c.tri(pt(x,y-5*s),pt(x-2.6f*s,y),pt(x+2.6f*s,y),color,.9f);
        c.tri(pt(x,y-2.6f*s),pt(x-3.4f*s,y+2.2f*s),pt(x+3.4f*s,y+2.2f*s),color,.9f);
        stroke(x,y+1.8f*s,x,y+4.1f*s,1.05f);
    };
    switch(terrain) {
    case Terrain::Forest:
        tree(-3.7f,.7f,.85f);tree(2.7f,-1.7f,1.0f);if(n%3)tree(4.9f,3.4f,.65f);break;
    case Terrain::Mountains:
        mountain(3.8f,1.8f,.72f);mountain(-2,0,1.17f);break;
    case Terrain::Hills:
        c.path({pt(-8,2),pt(-6,.5),pt(-3,-1.1),pt(-1,-1.4),pt(2,-.3),pt(5,2)},std::max(.7f,scale*.9f),color);
        c.path({pt(1,-1),pt(3,-2.3),pt(5,-2.4),pt(8,-.6)},std::max(.6f,scale*.8f),color);break;
    case Terrain::Marsh:
        stroke(-5,2,4,2);stroke(-2,4,5,4);stroke(-1,1,-3,-3);stroke(0,1,0,-4);stroke(1,1,3,-2);break;
    case Terrain::Plains:
        stroke(-3,1,-4,-1.5f);stroke(-2,1,-2.2f,-2.8f);stroke(0,1,1,-1.3f);
        if(n%2)stroke(4,3,5,1.1f);break;
    }
}
inline void compass(Canvas& c,V p) {
    const Ink dark=0x496963, light=0xc9d8cf;
    c.center(p.x,p.y-38,L"N",Serif,13,dark);
    constexpr float pi=3.14159265f;
    for(int i=0;i<48;i++){
        float a=2*pi*i/48,b=2*pi*(i+1)/48;
        c.line(p+V{cosf(a),sinf(a)}*13,p+V{cosf(b),sinf(b)}*13,.8f,0x9bafa7);
    }
    for(int i=0;i<4;i++){
        float a=i*pi/2;V dir{cosf(a),sinf(a)},perp{-dir.y,dir.x};
        V tip=p+dir*(i%2?20.f:16.f);
        c.tri(p,tip,p+perp*3.7f,dark);c.tri(p,tip,p-perp*3.7f,light);
        c.path({p+perp*3.7f,tip,p-perp*3.7f},.7f,dark);
    }
}
enum class Icon { Hex, Folder, Save, Select, Hand, Brush, Fit, Minus, Plus, Layers, Grid, Chevron, Check, File, Eye };
inline void icon(Canvas& c,Icon type,V p,float size,Ink ink){
    float s=size/20;
    auto coords=[&](float x,float y){return p+V{x-10,y-10}*s;};
    auto line=[&](std::initializer_list<V> path){std::vector<V> pts;for(auto a:path)pts.push_back(coords(a.x,a.y));c.path(pts,1.45f*s,ink);};
    switch(type){
    case Icon::Hex:hexLine(c,p,size*.43f,1.4f*s,ink);break;
    case Icon::Folder:line({{2,15},{2,5},{8,5},{10,7},{17,7},{17,9}});line({{2,15},{4,9},{19,9},{16,16},{2,16}});break;
    case Icon::Save:line({{3,3},{14,3},{17,6},{17,17},{3,17},{3,3}});line({{6,3},{6,8},{13,8},{13,3}});line({{6,17},{6,11},{14,11},{14,17}});break;
    case Icon::Select:line({{4,2},{15,10},{10,11},{13,17},{10,18},{7,12},{4,15},{4,2}});break;
    case Icon::Hand:line({{5,10},{5,6},{7,6},{7,11},{7,3},{9,3},{9,10},{9,2},{11,2},{11,10},{11,4},{13,4},{13,11},{14,6},{16,6},{16,13},{13,17},{8,17},{3,12},{3,10},{5,11}});break;
    case Icon::Brush:line({{7,12},{14,3},{16,3},{17,5},{10,14},{7,12}});line({{8,12},{5,12},{4,15},{2,17},{6,17},{9,16},{10,14}});break;
    case Icon::Fit:line({{3,8},{3,3},{8,3}});line({{12,3},{17,3},{17,8}});line({{17,12},{17,17},{12,17}});line({{8,17},{3,17},{3,12}});break;
    case Icon::Plus:line({{10,4},{10,16}});[[fallthrough]];
    case Icon::Minus:line({{4,10},{16,10}});break;
    case Icon::Layers:line({{2,7},{10,3},{18,7},{10,11},{2,7}});line({{2,11},{10,15},{18,11}});line({{3,15},{10,18},{17,15}});break;
    case Icon::Grid:hexLine(c,p,6*s,1.3f*s,ink);line({{10,4},{10,10},{15,13}});line({{10,10},{5,13}});break;
    case Icon::Chevron:line({{6,8},{10,12},{14,8}});break;
    case Icon::Check:line({{3,10},{8,15},{17,5}});break;
    case Icon::File:line({{5,2},{12,2},{16,6},{16,18},{5,18},{5,2}});line({{12,2},{12,7},{16,7}});line({{7,11},{13,11}});line({{7,14},{12,14}});break;
    case Icon::Eye:line({{2,10},{6,6},{10,5},{14,6},{18,10},{14,14},{10,15},{6,14},{2,10}});c.circle(p,2*s,ink);break;
    }
}