// Raff shelf navigation, independent of rendering and controller hardware.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace raff {
struct Shelf {
 std::string key, name;
 std::vector<int> items; // Indices into the filtered catalog, never raw game IDs.
 int cursor=0;
 float offset=0;
};
struct Shelves {
 std::vector<Shelf> rows;
 int active=0;
 float vertical=0;
 static constexpr float pitch=292, rowHeight=418, viewport=1776, cardWidth=272;
 int selected() const {return rows.empty()?-1:rows[active].items[rows[active].cursor];}
 float target(const Shelf& row) const {
  const float extent=std::max(0.f,(row.items.size()-1)*pitch+cardWidth-viewport);
  return std::clamp((row.cursor-2)*pitch,0.f,extent);
 }
 void reset(std::vector<Shelf> next,int selection,std::string preferred="") {
  rows=std::move(next);
  rows.erase(std::remove_if(rows.begin(),rows.end(),[](const Shelf&r){return r.items.empty();}),rows.end());
  active=0;bool found=false;
  for(int pass=0;pass<2&&!found;pass++)for(int n=0;n<(int)rows.size();n++) {
   if(pass==0&&rows[n].key!=preferred)continue;
   // Avoid libc++'s wmemchr optimization: the PS5 runtime can miss integer
   // matches past the midpoint of a short range. Verified on the console.
   int at=-1;
   for(int pos=0;pos<(int)rows[n].items.size();pos++)if(rows[n].items[pos]==selection){at=pos;break;}
   if(at>=0){active=n;rows[n].cursor=at;found=true;break;}
  }
  for(auto&r:rows)r.offset=target(r);
  vertical=active*rowHeight;
 }
 // True means Up should hand focus back to the platform bar.
 bool move(int dx,int dy,int page=0) {
  if(rows.empty())return dy<0;
  if(dy<0&&active==0)return true;
  active=std::clamp(active+dy,0,(int)rows.size()-1);
  auto&r=rows[active];r.cursor=std::clamp(r.cursor+dx+page*6,0,(int)r.items.size()-1);
  return false;
 }
 void animate(float dt,bool reduced=false) {
  const float blend=reduced?1.f:1.f-std::exp(-16.f*std::clamp(dt,0.f,.1f));
  vertical+=(active*rowHeight-vertical)*blend;
  for(auto&r:rows)r.offset+=(target(r)-r.offset)*blend;
 }
};
struct Grid {
 static constexpr int columns=6, pageSize=12;
 static int move(int selection,int count,int dx,int dy,int page=0) {
  if(count<=0)return 0;
  selection=std::clamp(selection,0,count-1);
  if(page)return std::clamp(selection+page*pageSize,0,count-1);
  if(dy)return std::clamp(selection+dy*columns,0,count-1);
  const int row=selection/columns;
  return std::clamp(selection+dx,row*columns,std::min(count-1,row*columns+columns-1));
 }
 static int first(int selection){return std::max(0,selection/columns-1)*columns;}
};
}
