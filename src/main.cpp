// Raff native PS5 library. SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/system.hpp"
#include "platform/ps5/pad.hpp"
#include "core/input.hpp"
#include "vendor/stb_image.h"
#include "network.hpp"
#include "artwork.hpp"
#include "shelves.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <set>

using raff::Json;using namespace hui::gfx;using hui::Action;using hui::Direction;
extern "C" void hui_heap_stats(size_t*,size_t*,size_t*,size_t*);
namespace{
void log_heap(const char*stage){size_t live=0,peak=0,blocks=0,failures=0;hui_heap_stats(&live,&peak,&blocks,&failures);hui::sys::log("Raff heap %s: live=%zu peak=%zu blocks=%zu failures=%zu",stage,live,peak,blocks,failures);}
std::string read_file(const char*p){FILE*f=fopen(p,"rb");if(!f)return{};fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<0||n>16*1024*1024){fclose(f);return{};}std::string s(n,'\0');if(fread(s.data(),1,n,f)!=(size_t)n)s.clear();fclose(f);return s;}
Json load_json(const char*p){auto j=Json::parse(read_file(p),nullptr,false);return j.is_discarded()?Json():j;}
std::string number(double v,int decimals=1){char b[64];snprintf(b,sizeof(b),decimals?"%.1f":"%.0f",v);return b;}
std::string bytes(double n){if(n>=1073741824)return number(n/1073741824)+" GB";if(n>=1048576)return number(n/1048576)+" MB";return number(n/1024,0)+" KB";}
std::string eta(const Json&j){if(!j.contains("eta")||j["eta"].is_null())return"--:--";int n=j["eta"].get<int>();char b[64];if(n>=3600)snprintf(b,sizeof(b),"%d:%02d:%02d",n/3600,n/60%60,n%60);else snprintf(b,sizeof(b),"%d:%02d",n/60,n%60);return b;}
std::string lower(std::string s){for(char&c:s)if(c>='A'&&c<='Z')c+=32;return s;}
std::string search_key(std::string s){s=lower(s);for(auto&c:s)if((unsigned char)c<128&&!std::isalnum((unsigned char)c))c=' ';std::string t,out;size_t pos=0;const std::map<std::string,std::string> romans={{"i","1"},{"ii","2"},{"iii","3"},{"iv","4"},{"v","5"},{"vi","6"},{"vii","7"},{"viii","8"},{"ix","9"},{"x","10"}};while(pos<s.size()){pos=s.find_first_not_of(' ',pos);if(pos==std::string::npos)break;size_t end=s.find(' ',pos);if(end==std::string::npos)end=s.size();t=s.substr(pos,end-pos);pos=end;auto i=romans.find(t);if(i!=romans.end())t=i->second;if(!out.empty())out+=' ';out+=t;}return out;}
GLuint texture_file(const std::string&p){int w,h,n;auto*b=stbi_load(p.c_str(),&w,&h,&n,4);if(!b){hui::sys::log("Raff image load failed %s: %s",p.c_str(),stbi_failure_reason());return 0;}if(w>8192||h>8192){stbi_image_free(b);return 0;}GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,b);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);stbi_image_free(b);return t;}
struct Cover{GLuint texture=0;int used=0,width=1,height=1;bool pending=false;int loadedFrame=0,attemptedFrame=0;float uvY=0,uvH=1;};
struct App{
 Renderer renderer;DrawList d;Font regular,bold;GLuint rt=0,bt=0,arabicTexture=0,arabicExtra=0;Json labels,catalog,state;std::map<std::string,Cover>covers;std::set<std::string>favorites;std::vector<int>visible;
 int availability=0,filterCursor=0;bool filterOpen=false;float navX=240,contentIn=1;std::vector<std::string>searchKeys;std::set<std::string>installedSet;std::map<std::string,Json>jobByKey;bool viewRestored=false,viewTouched=false;double viewSaveAt=0,viewStarted=0;std::string viewSignature;
 int tab=0,selected=0,sort=0,detail=-1,option=0,job=0,setting=0,key=0,frame=0;bool ar=true,navFocus=false,searching=false,exitDialog=false,cancelDialog=false;std::string query,toast;double toastUntil=0,lastStatus=0,now=0;float scroll=0,scrollTarget=0,modalAlpha=0,lastDt=.016f,focusX=72,focusY=409;std::map<std::string,float>bars;std::map<std::string,int> cardIndex;std::string cancelJobId,selectedJobId;std::string screenshotName="raff-preview.bmp";bool captureRequested=false;int keyboardRow=1,keyboardCol=0;bool pathEditing=false,pathUpper=false;std::string pathTarget,pathDraft;
 const char*tabKeys[31]={"ps2","switch","ps5","ps4","ps1","downloads","favorites","local","settings","xbox","xbox360","ps3","hub","nes","snes","n64","gb","gbc","gba","nds","3ds","psp","gamecube","wii","genesis","mastersystem","gamegear","saturn","segacd","dreamcast","arcade"};const char*tabNames[31]={"PlayStation 2","Switch","PlayStation 5","PlayStation 4","PlayStation 1","Downloads","Favorites","On console","Settings","Xbox","Xbox 360","PlayStation 3","Emulators","NES","SNES","Nintendo 64","Game Boy","Game Boy Color","Game Boy Advance","Nintendo DS","Nintendo 3DS","PSP","GameCube","Wii","Mega Drive","Master System","Game Gear","Saturn","Sega CD","Dreamcast","Arcade / FBNeo"};const char*sortKeys[5]={"featured","rating","popular","az","smallest"};const char*sortNames[5]={"Featured","Top rated","Most downloaded","A - Z","Smallest first"};
 const int tabOrder[12]={0,1,2,3,4,9,10,11,5,6,7,8};
 int nav_index(){for(int n=0;n<12;n++)if(tabOrder[n]==tab)return n;return 0;}
 int lastPlatformTab=0,headerCursor=0;bool headerFocus=false;
 const int platformOrder[26]={4,0,11,3,2,1,9,10,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30};
 static constexpr int platformCount=26;
 int platform_index(){for(int n=0;n<platformCount;n++)if(platformOrder[n]==(platform_tab()?tab:lastPlatformTab))return n;return 1;}
 int header_index(){return platformPicker?0:tab==12?4:tab==7?1:tab==5?3:tab==6?2:tab==8?5:0;}
 void choose_section(int n){headerCursor=n;headerFocus=false;navFocus=false;if(n==0){systems_open();}else change_tab(n==1?7:n==2?6:n==3?5:n==4?12:8);}
 void step_tab(int step){change_tab(platformOrder[(platform_index()+step+platformCount)%platformCount]);headerFocus=false;}
 bool platform_tab(){return tab<5||tab==9||tab==10||tab==11||tab>=13;}
 void text(const std::string&s,float x,float y,float size,Color c=Color::rgb(0xffffff),bool strong=false,float max=2000,Align align=Align::left){std::string clean=s;for(const auto&pair:std::initializer_list<std::pair<std::string,std::string>>{{"’","'"},{"‘","'"},{"–","-"},{"—","-"},{"™",""},{"®",""}}){size_t at=0;while((at=clean.find(pair.first,at))!=std::string::npos){clean.replace(at,pair.first.size(),pair.second);at+=pair.second.size();}}auto&f=strong?bold:regular;d.text(f,strong?bt:rt,f.fit(clean,size,max),x,y,size,c,align);}
 void label(const std::string&k,const std::string&en,float x,float y,float size=28,Color c=Color::rgb(0xffffff),float max=1000,Align align=Align::left){if(!ar||!arabicTexture||!labels.contains(k)){text(en,x,y,size,c,false,max,align);return;}auto a=labels[k];float factor=size/38.f,w=a["w"].get<float>()*factor,h=64*factor;if(w>max){h*=max/w;w=max;}if(align==Align::right)x-=w;else if(align==Align::center)x-=w/2;d.image(a.value("atlas",0)?arabicExtra:arabicTexture,{x,y-size*1.17f,w,h},{a.value("x",0.f)/1024,a["y"].get<float>()/a.value("atlasH",8192.f),a["w"].get<float>()/1024,64.f/a.value("atlasH",8192.f)},c);}
 void init(){hui::sys::log("Raff step renderer");if(!renderer.init())hui::sys::park();hui::sys::log("Raff step regular font");regular.load(read_file("/app0/assets/fonts/inter-regular.huifont"));hui::sys::log("Raff step bold font");bold.load(read_file("/app0/assets/fonts/inter-semibold.huifont"));hui::sys::log("Raff step font GPU upload");rt=renderer.batch().create_font_texture(regular);bt=renderer.batch().create_font_texture(bold);hui::sys::log("Raff step labels JSON");labels=load_json("/app0/assets/arabic.json");hui::sys::log("Raff step Arabic texture");arabicTexture=texture_file("/app0/assets/arabic.png");arabicExtra=texture_file("/app0/assets/arabic-extra.png");hui::sys::log("Raff Arabic atlas texture=%u labels=%zu",arabicTexture,labels.size());hui::sys::log("Raff step catalog JSON");log_heap("before catalog");catalog=Json::array();hui::sys::log("Raff step state");state=raff::status();init_cinema();init_systems();init_hub();rebuild();hui::sys::log("Raff step network start");raff::start_network();raff::request("restore-classic-view","/library/view",{{"id",browse_id()}});raff::start_artwork();hui::sys::log("Raff catalog: %zu native cards",catalog.size());}
 bool installed(const Json&g){return installedSet.count(g.value("key",""))>0;}
 Color accent(){return Color::rgb(0x70c8ff);}
 std::string platform_name(const Json&g){auto p=g.value("platform","");return p=="ps1"?"PS1":p=="ps2"?"PS2":p=="ps3"?"PS3":p=="ps4"?"PS4":p=="ps5"?"PS5":p=="xbox"?"XBOX":p=="xbox360"?"X360":p=="switch"?"SWITCH":platform_title(p);}
 std::string source_name(const std::string&s){return s=="minerva"?"Minerva":s=="nyaa"?"Nyaa":s=="blackbox"?"BLACKBOX":s=="orbit"?"Orbit Store":s=="pegasus"?"Pegasus DL":s=="pippo"?"Pippo / Spectrum":s=="archive"?"Internet Archive":s=="chdstationps1"?"CHD Station PS1":s=="chdstation"?"CHD Station":s=="ps2chd"?"PS2 CHD":s=="dlarchive"?"DLArchive":s;}
 void rebuild(){ensure_catalog();visible.clear();if(tab==12){filter_hub();return;}const auto search=search_key(query);for(int i=0;i<(int)catalog.size();i++){auto&g=catalog[i];if(platform_tab()&&g.value("platform","")!=tabKeys[tab])continue;if(tab==6&&!favorites.count(g.value("key","")))continue;if(tab==7&&!installed(g))continue;if(availability==1&&g["options"].empty()&&tab!=7)continue;if(availability==2&&!installed(g))continue;if(!search.empty()&&searchKeys[i].find(search)==std::string::npos)continue;visible.push_back(i);}std::stable_sort(visible.begin(),visible.end(),[&](int a,int b){auto&x=catalog[a];auto&y=catalog[b];if(sort==0){bool ax=!x["options"].empty(),ay=!y["options"].empty();if(ax!=ay)return ax;bool cx=!x.value("cover","").empty(),cy=!y.value("cover","").empty();if(cx!=cy)return cx;}if(sort==0||sort==1){int sx=x.value("score",-1),sy=y.value("score",-1);if(sx!=sy)return sx>sy;}else if(sort==2){auto sx=x.value("popularityScore",x.value("downloads",-1LL)),sy=y.value("popularityScore",y.value("downloads",-1LL));if(sx!=sy)return sx>sy;}else if(sort==4){auto sx=x.value("size",0LL),sy=y.value("size",0LL);if(sx!=sy)return(sx?sx:INT64_MAX)<(sy?sy:INT64_MAX);}return x.value("title","")<y.value("title","");});selected=std::min(selected,std::max(0,(int)visible.size()-1));rebuild_shelves(shelves.rows.empty()?"":shelves.rows[shelves.active].key);}
 void accept_artwork(){
  raff::Artwork pixels;if(!raff::take_artwork(pixels))return;
  auto i=covers.find(pixels.key);if(i==covers.end())return;auto&c=i->second;c.pending=false;
  if(pixels.pixels.empty())return;
  if(c.texture)glDeleteTextures(1,&c.texture);glGenTextures(1,&c.texture);glBindTexture(GL_TEXTURE_2D,c.texture);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  c.loadedFrame=frame;c.width=pixels.width;c.height=pixels.height;c.uvY=pixels.contentY/float(pixels.height);c.uvH=pixels.contentHeight/float(pixels.height);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,c.width,c.height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.pixels.data());
 }
 Cover& cover(const Json&g){
  const auto id=g.value("cover","");auto i=covers.find(id);
  if(i!=covers.end()){auto&c=i->second;c.used=frame;if(!id.empty()&&!c.texture&&!c.pending&&frame-c.attemptedFrame>(id.rfind("media-",0)==0?120:3600)){c.pending=true;c.attemptedFrame=frame;raff::request_artwork(id);}return c;}
  if(covers.size()>=112){auto old=std::min_element(covers.begin(),covers.end(),[](const auto&a,const auto&b){return a.second.used<b.second.used;});if(old->second.texture)glDeleteTextures(1,&old->second.texture);covers.erase(old);}
  auto&c=covers[id];c.used=frame;c.pending=!id.empty();c.attemptedFrame=frame;if(c.pending)raff::request_artwork(id);return c;
 }
 void poster(const Json&g,Rect r,float radius=18){
  auto&c=cover(g);d.rounded_rect(r,radius,Color::rgb(0x101116));
  if(c.texture){Rect uv=cover_uv(g,c);float naturalHeight=c.height*uv.h,naturalWidth=c.width*uv.w,scale=std::min(r.w/naturalWidth,r.h/naturalHeight),w=naturalWidth*scale,h=naturalHeight*scale;d.image(c.texture,{r.x+(r.w-w)/2,r.y+(r.h-h)/2,w,h},uv,Color::rgb(0xffffff,std::min(1.f,(frame-c.loadedFrame+1)*lastDt*7)),radius);}
  else {d.circle(r.cx(),r.y+r.h*.35f,26,Color::rgb(0x91ccb9,.10f));d.ring(r.cx(),r.y+r.h*.35f,17,2,Color::rgb(0xb8ead8,.5f));text(platform_name(g),r.cx(),r.y+r.h*.35f+5,12,Color::rgb(0xc8e3da),true,r.w,Align::center);auto lines=bold.wrap(g.value("title",""),23,r.w-30);for(int n=0;n<std::min(3,(int)lines.size());n++)text(lines[n],r.cx(),r.y+r.h*.59f+n*28,23,Color::rgb(0xe5edf8),true,r.w-26,Align::center);if(c.pending)d.arc(r.cx(),r.y+r.h*.35f,23,2,now*2,2,Color::rgb(0xb9f2d8));}
 }
 void button(const std::string&id,float x,float y,float size=24,Color color=Color::rgb(0xe0dcec)){
  float r=size*.38f;if(id=="circle")d.ring(x,y,r,2,color);
  else if(id=="cross"){d.line(x-r,y-r,x+r,y+r,2,color);d.line(x+r,y-r,x-r,y+r,2,color);}
  else if(id=="square")d.bordered_rect({x-r,y-r,2*r,2*r},2,Color::rgb(0,0),2,color);
  else if(id=="triangle")d.triangle({x-r-1,y-r-1,r*2+2,r*2+2},color,2);
  else{float w=id.size()>3?57:44;d.bordered_rect({x-w/2,y-13,w,26},7,Color::rgb(0x1b2a3b),1,Color::rgb(0x42536a));text(id,x,y+6,15,color,true,w-4,Align::center);}
 }
 void hint(const std::string&icon,const std::string&key,const std::string&en,float x,float y,float width=220){button(icon,x,y-8,29);label(key,en,x+31,y,24,Color::rgb(0xc2bdcf),width);}
 void footer(){
  d.rounded_rect({0,985,1920,95},0,Color::rgb(0x080c13,.94f));d.line(72,985,1848,985,1,Color::rgb(0xffffff,.10f));
  if(platformPicker){hint("cross","opensystem","Open library",102,1038,260);hint("L1/R1","manufacturer","Manufacturer",449,1038,268);hint("L2/R2","systempages","Pages",885,1038,240);hint("triangle","searchshort","Search",1260,1038,200);hint("circle","back","Back",1610,1038,200);return;}if(tab==12&&!headerFocus){if(hubOpened){hint("cross","download","Download",102,1038,350);hint("circle","back","Back",1500,1038,240);}else{hint("cross","select","Details",102,1038,240);hint("triangle","searchshort","Search",393,1038,205);hint("square","favorite","Favorite",659,1038,195);hint("R2","categories","Category",939,1038,223);hint("L2","hostfilter","Host filter",1255,1038,213);hint("touch","favorites","Favorites",1579,1038,235);}return;}if(headerFocus||navFocus){hint("cross","select","Select",102,1038,245);hint("R3","allplatforms","All platforms",625,1038,330);hint("L1/R1","emulator","Platforms",1080,1038,380);return;}
  if(tab==5){hint("R3","importtorrent","Import torrent",1410,974,370);hint("cross","pauseresume","Pause / resume",102,1038,310);hint("square","cancel","Cancel",560,1038,290);hint("circle","back","Back",1010,1038,170);hint("L1/R1","emulator","Platforms",1410,1038,230);}
  else if(tab==8){hint("cross","change","Change",102,1038,290);hint("L3","language","Language",560,1038,250);hint("circle","back","Back",1010,1038,200);hint("L1/R1","emulator","Platforms",1410,1038,230);}
  else{hint("cross","select","Details",102,1038,145);hint("triangle","searchshort","Search",363,1038,145);hint("square","sort","Sort / filter",624,1038,160);hint("R3",gridMode?"discover":"browsegrid",gridMode?"Discover":"Browse grid",911,1038,215);hint("circle","back","Back",1248,1038,170);hint("L1/R1","emulator","Platforms",1550,1038,210);}
 }

 Json download_jobs(){Json result=Json::array();for(const auto&j:state.value("jobs",Json::array()))if(j.value("phase","")!="done"&&j.value("phase","")!="complete"&&j.value("phase","")!="canceled")result.push_back(j);return result;}
 void sync_jobs(){auto jobs=download_jobs();int previous=job;bool found=false;for(int n=0;n<(int)jobs.size();n++)if(jobs[n].value("id","")==selectedJobId){job=n;found=true;break;}if(!found)job=std::clamp(previous,0,std::max(0,(int)jobs.size()-1));selectedJobId=jobs.empty()?"":jobs[job].value("id","");if(cancelDialog&&std::none_of(jobs.begin(),jobs.end(),[&](const auto&j){return j.value("id","")==cancelJobId;}))cancelDialog=false;}
 std::string game_path(const std::string&p,const std::string&key=""){auto settings=state.value("settings",Json::object());auto gamePaths=settings.value("gamePaths",Json::object()),paths=settings.value("paths",Json::object());if(!key.empty()&&gamePaths.contains(key))return gamePaths[key].get<std::string>();if(paths.contains(p))return paths[p].get<std::string>();if(platformDefaults.contains(p))return platformDefaults[p];return p=="ps1"?"/data/PSXS5/games":p=="xbox"?"/data/xemu/games":p=="xbox360"?"/data/xbox360":p=="ps3"?"/data/rpcs3/games":p=="ps2"?"/data/PCSX2/games":p=="switch"?"/data/prosperoeden/roms":"/data/etaHEN/games";}
 void open_path(const std::string&target,const std::string&platform){pathEditing=true;pathUpper=false;pathTarget=target;pathDraft=game_path(platform,target.find(':')!=std::string::npos?target:"");searching=true;keyboardRow=1;keyboardCol=0;}
 void apply_keyboard(){searching=false;if(pathTarget=="@magnet"&&pathEditing){magnetDraft=pathDraft;pathEditing=false;inspect_import({{"magnet",magnetDraft},{"platform",importPlatforms[importPlatform]}});return;}if(pathTarget=="@hub-query"&&pathEditing){query=pathDraft;pathEditing=false;hubCursor=0;filter_hub();return;}if(pathEditing){if(pathTarget.find(':')!=std::string::npos)dispatch("/settings",{{"gamePath",{{"key",pathTarget},{"path",pathDraft}}}});else dispatch("/settings",{{"paths",{{pathTarget,pathDraft}}}});pathEditing=false;}else{selected=0;rebuild();}}
 void notify(const std::string&s){toast=s;toastUntil=now+4;}
 void change_tab(int t){platformPicker=false;shelves.rows.clear();tab=(t+31)%31;hubOpened=false;if(platform_tab())lastPlatformTab=tab;contentIn=0;selected=0;job=0;detail=-1;query.clear();scroll=scrollTarget=0;focusY=409;rebuild();}
 std::string downloadNoticeId,downloadNoticeTitle;double downloadNoticeUntil=0;
 void download_added(const std::string&id,const std::string&title){downloadNoticeId=id;downloadNoticeTitle=title;downloadNoticeUntil=now+12;toastUntil=0;}
 bool download_notice_visible(){return now<downloadNoticeUntil&&!searching&&!exitDialog&&!cancelDialog&&!filterOpen&&!platformPicker&&transferModal.empty();}
 void open_downloads_notice(){const auto id=downloadNoticeId;downloadNoticeUntil=0;transferModal.clear();detail=-1;choose_section(3);state=raff::status();selectedJobId=id;sync_jobs();}
 void draw_download_notice(){if(!download_notice_visible())return;
  d.shadow({420,36,1080,128},20,24,Color::rgb(0,.38f));d.bordered_rect({420,36,1080,128},20,Color::rgb(0x163449),1,Color::rgb(0x70bba5));
  d.circle(469,100,24,Color::rgb(0x69dbb0,.14f));d.line(457,101,466,110,3,Color::rgb(0x8fe5bd));d.line(466,110,482,88,3,Color::rgb(0x8fe5bd));
  label("added","Added to downloads",516,85,29,Color::rgb(0xe8fff4),574);text(downloadNoticeTitle,516,127,22,Color::rgb(0xbdd7e5),false,574);
  d.bordered_rect({1127,60,342,80},14,Color::rgb(0x244c64),1,Color::rgb(0x74c8e1));hint("triangle","viewdownloads","View downloads",1158,109,252);
 }
 double nextClassicArt=0;std::map<std::string,int>metadataIndex;
 void classicArt(){
  Json result;if(raff::take_result("classic-art",result)&&result.contains("items"))for(auto&art:result["items"]){auto it=metadataIndex.find(art.value("id",""));if(it!=metadataIndex.end()&&it->second<(int)catalog.size()&&art.contains("cover"))catalog[it->second]["cover"]=art["cover"];}
  if(now<nextClassicArt||shelves.rows.empty())return;nextClassicArt=now+3;Json ids=Json::array();auto&r=shelves.rows[shelves.active];int first=std::max(0,r.cursor-2);
  const bool isGrid=gridMode||tab==6||tab==7;if(isGrid)first=std::max(0,selected-3);const int count=isGrid?visible.size():r.items.size();
  for(int n=first;n<std::min(first+8,count);n++){int at=visible[isGrid?n:r.items[n]];auto&g=catalog[at];auto art=covers.find(g.value("cover",""));bool missing=g.value("cover","").empty()||(art!=covers.end()&&!art->second.pending&&!art->second.texture);if(missing&&g.contains("metadataId")){ids.push_back(g["metadataId"]);metadataIndex[g["metadataId"].get<std::string>()]=at;}}
  if(!ids.empty())raff::request("classic-art","/library/media",{{"ids",ids}});
 }

 // Keep browsing position in the existing local user database; no per-frame disk writes.
 void browsing_state(){
  if(!viewStarted)viewStarted=now;
  Json result;if(raff::take_result("restore-classic-view",result)){
   if(result.is_object()&&!result.contains("error")&&!viewTouched){
    auto integer=[&](const char*k,int fallback,int max){return result.contains(k)&&result[k].is_number_integer()?std::clamp(result[k].get<int>(),0,max):fallback;};
    tab=integer("tab",0,30);sort=integer("sort",0,4);availability=integer("filter",0,2);if(!result.value("catalogPermissionMigration",false))availability=0;query=result.contains("query")&&result["query"].is_string()?result["query"].get<std::string>().substr(0,60):"";rebuild();
    auto k=result.contains("key")&&result["key"].is_string()?result["key"].get<std::string>():"";for(int n=0;n<(int)visible.size();n++)if(catalog[visible[n]].value("key","")==k){selected=n;break;}
    gridMode=result.value("layoutVersion",0)>=2?result.value("gridMode",true):true;reducedMotion=result.value("reducedMotion",false);lastPlatformTab=platform_tab()?tab:integer("lastPlatform",0,30);if(lastPlatformTab==5||lastPlatformTab==6||lastPlatformTab==7||lastPlatformTab==8)lastPlatformTab=0;rebuild_shelves(result.value("shelf",""));navX=72+platform_index()*224;
   }viewRestored=true;
  }
  if(!viewRestored&&now-viewStarted<10)return;viewRestored=true;
  Json value={{"layoutVersion",2},{"catalogPermissionMigration",true},{"lastPlatform",lastPlatformTab},{"gridMode",gridMode},{"reducedMotion",reducedMotion},{"shelf",shelves.rows.empty()?"":shelves.rows[shelves.active].key},{"tab",tab},{"sort",sort},{"filter",availability},{"query",query},{"key",visible.empty()?"":catalog[visible[selected]].value("key","")}};
  const auto signature=value.dump();if(signature!=viewSignature){viewSignature=signature;viewSaveAt=now+1.2;}
  if(viewSaveAt&&now>=viewSaveAt&&!searching&&!filterOpen){viewSaveAt=0;raff::request("save-classic-view","/library/view",{{"id",browse_id()},{"value",value}});}
  raff::take_result("save-classic-view",result);
 }
 void update(float dt){update_tick();accept_catalog();update_hub();browsing_state();if(platform_tab()||tab==6||tab==7)classicArt();lastDt=dt;frame++;accept_artwork();contentIn=reducedMotion?1:std::min(1.f,contentIn+dt*5);scroll+=(scrollTarget-scroll)*std::min(1.f,dt*14);if(now-lastStatus>.25){auto priorInstalled=installedSet;auto priorFavorites=favorites;state=raff::status();sync_jobs();lastStatus=now;installedSet.clear();for(auto&k:state.value("installedKeys",Json::array()))installedSet.insert(k.get<std::string>());jobByKey.clear();for(auto&j:download_jobs())jobByKey[j.value("key","")]=j;if(state.contains("settings")){auto&s=state["settings"];ar=s.value("language","ar")=="ar";favorites.clear();for(auto&f:s.value("favorites",Json::array()))favorites.insert(f.get<std::string>());}auto n=raff::take_notice();if(!n.empty()){if(n.find("Already")!=std::string::npos||n.find("اللعبة")!=std::string::npos)n="exists";else if(n!="added"&&n!="saved"&&n!="offline")n="error";notify(n);}if((priorFavorites!=favorites)||((tab==7||availability==2)&&priorInstalled!=installedSet))rebuild();}modalAlpha+=(float(detail>=0||searching||exitDialog||cancelDialog||filterOpen)-modalAlpha)*(reducedMotion?1.f:std::min(1.f,dt*18));sync_cinema(dt);}

 void input(const hui::InputFrame&i){auto press=[&](Action a){return i.is_pressed(a);};int dx=i.nav==Direction::right?1:i.nav==Direction::left?-1:0,dy=i.nav==Direction::down?1:i.nav==Direction::up?-1:0;
 if(dx||dy||press(Action::confirm)||press(Action::back)||press(Action::page_prev)||press(Action::page_next)||press(Action::north)||press(Action::west)||press(Action::menu))viewTouched=true;
 
 if(updateDialog){if(press(Action::back)){updateDialog=false;return;}if(press(Action::confirm)&&!updatePending){updatePending=true;updateDialog=false;raff::request("app-update","/updates/apply",Json::object());}return;}
 if(download_notice_visible()&&!transferPending&&press(Action::north)){open_downloads_notice();return;}
 if(platformPicker&&!exitDialog&&!searching){systems_input(i);return;}
 if(exitDialog){if(press(Action::back))exitDialog=false;if(press(Action::confirm))hui::sys::quit();return;}
 if(cancelDialog){if(press(Action::back))cancelDialog=false;if(press(Action::confirm)){auto jobs=download_jobs();if(!cancelJobId.empty())dispatch("/action",{{"id",cancelJobId},{"action","cancel"}});cancelDialog=false;}return;}
 if(filterOpen){if(press(Action::back)){filterOpen=false;return;}if(dx)filterCursor=filterCursor<5?5:0;if(dy)filterCursor=filterCursor<5?std::clamp(filterCursor+dy,0,4):std::clamp(filterCursor+dy,5,7);if(press(Action::confirm)){if(filterCursor<5)sort=filterCursor;else availability=filterCursor-5;selected=0;scroll=scrollTarget=0;rebuild();filterOpen=false;}return;}
 if(searching){
  std::string rows[]={"1234567890",pathEditing&&!pathUpper?"qwertyuiop":"QWERTYUIOP",pathEditing&&!pathUpper?"asdfghjkl":"ASDFGHJKL",pathEditing?(pathUpper?"ZXCVBNM/._-:?&=%":"zxcvbnm/._-:?&=%"):"ZXCVBNM","   "};auto&value=pathEditing?pathDraft:query;
  keyboardRow=std::clamp(keyboardRow+dy,0,4);keyboardCol=std::clamp(keyboardCol+dx,0,(int)rows[keyboardRow].size()-1);
  if(press(Action::back)){searching=false;if(!pathEditing){selected=0;rebuild();}pathEditing=false;return;}
  if(press(Action::l3)&&pathEditing)pathUpper=!pathUpper;
  if(pathEditing&&press(Action::jump_next))pathDraft.clear();
  if(pathEditing&&press(Action::jump_prev)&&pathTarget[0]!='@'){pathDraft.clear();apply_keyboard();return;}
  if(press(Action::west)&&!value.empty())value.pop_back();
  if(press(Action::confirm)){if(keyboardRow<4&&value.size()<(pathEditing?(pathTarget=="@magnet"?4096:240):60))value+=rows[keyboardRow][keyboardCol];else if(keyboardRow==4){if(keyboardCol==0&&value.size()<(pathEditing?(pathTarget=="@magnet"?4096:240):60))value+=' ';else if(keyboardCol==1&&!value.empty())value.pop_back();else if(keyboardCol==2)apply_keyboard();}}
  if(press(Action::north))apply_keyboard();return;
 }
 if(hub_input(i))return;
 if(detail>=0){auto&g=catalog[detail];if(press(Action::north)){open_path(g.value("key",""),g.value("platform","ps2"));return;}option=std::clamp(option+dy,0,std::max(0,(int)g["options"].size()-1));if(press(Action::back)){detail=-1;return;}if(press(Action::west)){auto k=g.value("key","");if(favorites.count(k))favorites.erase(k);else favorites.insert(k);dispatch("/settings",{{"favorites",favorites}});notify("saved");}if(press(Action::confirm)&&!g["options"].empty()){if(g["options"][option].value("method","")=="torrent")consent("/transfers/game",{{"id",g["options"][option]["id"]}});else{transfer_request("/queue",{{"id",g["options"][option]["id"]}},g.value("title",""));notify("wait");}}return;}
 if(tab==8&&!headerFocus&&!navFocus){settings_input(i);return;}
 if(press(Action::l3)){ar=!ar;dispatch("/settings",{{"language",ar?"ar":"en"}});}if(press(Action::page_prev))step_tab(-1);if(press(Action::page_next))step_tab(1);if(press(Action::menu)){choose_section(5);return;}if(press(Action::back)){if(headerFocus){headerFocus=false;navFocus=false;return;}if(navFocus){navFocus=false;return;}if(!query.empty()){query.clear();rebuild();}else if(tab!=lastPlatformTab||!gridMode){choose_section(0);}else systems_open();return;}
 if(press(Action::north)||press(Action::touch)){if(!platform_tab()&&tab!=6&&tab!=7)change_tab(lastPlatformTab);searching=true;pathEditing=false;keyboardRow=1;keyboardCol=0;return;}
 if(headerFocus){headerCursor=std::clamp(headerCursor+dx,0,5);if(press(Action::confirm)){choose_section(headerCursor);return;}if(dy>0){headerFocus=false;navFocus=platform_tab();}return;}
 if(navFocus){if(press(Action::r3)){systems_open();return;}if(dy<0||!platform_tab()){headerFocus=true;headerCursor=header_index();navFocus=false;return;}if(dx)step_tab(dx);if(dy>0||press(Action::confirm))navFocus=false;return;}
 if(tab==8){if(dy<0&&setting==0){headerFocus=true;headerCursor=header_index();return;}setting=std::clamp(setting+dy,0,15);if(press(Action::confirm)&&setting==15){transferModal="limits";transferSetting=0;return;}if(press(Action::confirm)&&setting==14){open_path("ps3","ps3");return;}if((press(Action::confirm)||dx)&&setting==13){reducedMotion=!reducedMotion;notify("saved");return;}if(press(Action::confirm)&&setting>=10&&setting<=12){const char*extra[]={"ps1","xbox","xbox360"};open_path(extra[setting-10],extra[setting-10]);return;}if((dx||press(Action::confirm))&&setting>=8&&setting<=9){const char*k=setting==8?"autoInstall":"removePackages";dispatch("/settings",{{k,!state["settings"].value(k,true)}});return;}if(press(Action::confirm)&&setting>=3&&setting<=7){if(setting==7){dispatch("/scan-paths",Json::object());notify("wait");}else{const char*platforms[]={"ps2","switch","ps5","ps4"};open_path(platforms[setting-3],platforms[setting-3]);}return;}if(dx||press(Action::confirm)){int step=dx?dx:1;if(setting==0){ar=!ar;dispatch("/settings",{{"language",ar?"ar":"en"}});}if(setting==1){int n=state["settings"].value("parallel",3);dispatch("/settings",{{"parallel",std::clamp(n+step,1,4)}});}if(setting==2){int n=state["settings"].value("connections",16);dispatch("/settings",{{"connections",std::clamp(n+step*2,1,16)}});}}return;}
 if(tab==5){auto jobs=download_jobs();if(dy<0&&job==0){headerFocus=true;headerCursor=header_index();return;}job=std::clamp(job+dy,0,std::max(0,(int)jobs.size()-1));if(!jobs.empty()){auto&j=jobs[job];selectedJobId=j.value("id","");if(press(Action::confirm)){auto phase=j.value("phase","");if(phase=="queued"||phase=="downloading"||phase=="resolving"||j.value("paused",false)||!j.value("error","").empty())dispatch("/action",{{"id",j["id"]},{"action",j.value("paused",false)||!j.value("error","").empty()?"resume":"pause"}});}if(press(Action::west)){cancelJobId=j.value("id","");cancelDialog=true;}}return;}
 if(press(Action::r3)){viewTouched=true;if(!platform_tab()){choose_section(0);return;}gridMode=!gridMode;gridOffset=raff::Grid::first(selected)/6*326.f;rebuild_shelves("allgames");return;}if(press(Action::north)||press(Action::touch)){searching=true;pathEditing=false;keyboardRow=1;keyboardCol=0;}if(press(Action::west)){filterOpen=true;filterCursor=sort;return;}if(gridMode||tab==7||tab==6){if(dy<0&&selected<6){if(platform_tab())navFocus=true;else{headerFocus=true;headerCursor=header_index();}}selected=raff::Grid::move(selected,visible.size(),dx,dy,press(Action::jump_next)?1:press(Action::jump_prev)?-1:0);}else{if(shelves.move(dx,dy,press(Action::jump_next)?1:press(Action::jump_prev)?-1:0))navFocus=true;if(shelves.selected()>=0)selected=shelves.selected();}if(press(Action::confirm)&&!visible.empty()){detail=visible[selected];option=0;modalAlpha=0;}}
 #include "catalogs.inc"
 #include "cinema.inc"
 #include "systems.inc"
 #include "hub.inc"
 #include "cinema-qa.inc"

 void downloads(){
  auto jobs=download_jobs();label("queue","Downloads",78,182,39,Color::rgb(0xffffff));text(bytes(state.value("speed",0.0))+"/s",1840,181,31,Color::rgb(0xbfb4ff),true,400,Align::right);label("transfers","Downloads continue after closing Raff",80,223,22,Color::rgb(0x9393a5),1450);
  if(jobs.empty()){d.circle(960,462,59,Color::rgb(0xa697ff,.085f));d.ring(960,462,35,2,Color::rgb(0xffa4b2));d.line(944,462,956,474,3,Color::rgb(0xffa4b2));d.line(956,474,979,450,3,Color::rgb(0xffa4b2));label("noactive","You're all caught up",960,582,38,Color::rgb(0xf0edf8),1500,Align::center);label("completedwhere","Completed games are in On console",960,632,26,Color::rgb(0xaaa5be),1500,Align::center);return;}
  job=std::min(job,(int)jobs.size()-1);int first=std::max(0,job-2);for(int n=first;n<std::min(first+3,(int)jobs.size());n++){auto&j=jobs[n];auto id=j.value("id","");float y=264+(n-first)*228;bool focus=n==job&&!navFocus;d.bordered_rect({72,y,1776,204},21,Color::rgb(focus?0x23364c:0x111c2b),focus?2:1,Color::rgb(focus?0x91ccff:0x26374a));auto card=cardIndex.find(j.value("key",""));if(card!=cardIndex.end())poster(catalog[card->second],{96,y+20,128,164},10);
   if(j.contains("kind")){text(std::to_string(j.value("peers",0))+" peers / up "+bytes(j.value("uploadSpeed",0.0))+"/s",266,y+192,16,accent(),false,1510);}text(j.value("title",""),266,y+48,29,Color::rgb(0xf8f5ff),true,1230);std::string phase=j.value("phase","queued");if(j.value("paused",false))phase="paused";else if(!j.value("error","").empty())phase=j.value("errorKind","error");else if(j.value("reconnecting",false))phase="reconnecting";else if(j.value("verifying",false))phase="torrentchecking";else if(phase=="resolving"||phase=="prepare")phase="queued";else if(phase=="scan"||phase=="move-ready"||phase=="moving")phase="installing";else if(phase=="extract-ready")phase="extracting";else if(phase=="pkg-cleanup"||phase=="pkg-deleting")phase="pkgcleanup";else if(phase.rfind("pkg-",0)==0)phase="pkginstall";const std::map<std::string,std::string> phaseNames={{"reconnecting","Reconnecting automatically"},{"torrentchecking","Checking torrent files"},{"queued","Waiting to download"},{"downloading","Downloading"},{"paused","Paused"},{"installing","Preparing game files"},{"extracting","Extracting"},{"pkginstall","Installing package on PS5"},{"pkgcleanup","Removing verified installer"},{"sourceerror","Source temporarily unavailable"},{"spaceerror","Not enough storage"},{"installerror","Installation unconfirmed; PKG kept"},{"networkerror","Connection interrupted; press X to resume"},{"fileerror","File operation failed"}};auto phaseName=phaseNames.find(phase);label(phase,phaseName==phaseNames.end()?phase:phaseName->second,268,y+83,22,Color::rgb(!j.value("error","").empty()?0xf1aaad:0xb7b0cc),1190);
   double total=j.value("total",0.0),received=j.value("received",0.0);bool installing=j.value("installing",false);float target=installing?0.f:total>0?std::clamp(float(received/total),0.f,1.f):0.f;auto&shown=bars[id];shown+=(target-shown)*std::min(1.f,lastDt*10);if(target==1&&shown>.999f)shown=1;text(installing?"...":number(target*100,1)+"%",1814,y+62,37,Color::rgb(0xa2d7ff),true,230,Align::right);
   Rect track{268,y+107,1544,12};d.rounded_rect(track,6,Color::rgb(0x23364a));if(installing){float w=180,x=track.x+float((std::sin(now*2)+1)*.5)*(track.w-w);d.gradient_rect_h({x,track.y,w,12},6,Color::rgb(0x368cff),Color::rgb(0x8bdcff));}else if(shown>0){float w=std::max(3.f,track.w*shown);d.gradient_rect_h({track.x,track.y,w,12},std::min(6.f,w/2),Color::rgb(0x368cff),Color::rgb(0x8bdcff));d.circle(track.x+w,track.y+6,5,Color::rgb(0xf3efff));}
   if(!j.value("error","").empty()){text(j.value("errorDetail","Press X to retry"),268,y+166,21,Color::rgb(0xf1aaad),false,1544);continue;}text(bytes(received)+" / "+bytes(total),268,y+166,23,Color::rgb(0xb9b4cc));text(bytes(j.value("speed",0.0))+"/s",1120,y+166,24,Color::rgb(0xd8d0ee),true,390,Align::right);label("remaining","Remaining",1380,y+166,22,Color::rgb(0xa29bb7),220);text(eta(j),1815,y+166,27,Color::rgb(0xeee8fa),true,220,Align::right);
  }
 }
 #include "settings.inc"

 void filters(){
  d.bordered_rect({380,220,1160,620},28,Color::rgb(0x182332),1,Color::rgb(0x66849d,.5f));label("filtertitle","Sort and filter",429,291,36,Color::rgb(0xf3f7ff),900);
  label("sort","Sort",433,350,23,accent(),500);label("showgames","Show games",1030,350,23,accent(),430);
  for(int n=0;n<8;n++){int row=n<5?n:n-5;float x=n<5?428:1028,y=377+row*66,w=n<5?554:464;bool focus=n==filterCursor;bool chosen=n<5?sort==n:availability==n-5;d.bordered_rect({x,y,w,56},13,Color::rgb(focus?0x344b62:0x202d3e),focus?2:0,accent());if(chosen){d.circle(x+25,y+28,5,accent());}const char*k=n<5?sortKeys[n]:n==5?"all":n==6?"available":"local";const char*en=n<5?sortNames[n]:n==5?"All games":n==6?"Ready to download":"On console";label(k,en,x+48,y+37,24,Color::rgb(0xe2ebf7),w-66);}
  hint("cross","applyfilter","Apply selection",470,796,340);hint("circle","back","Back",1140,796,240);
 }
 void details(){
  auto&g=catalog[detail];const auto&options=g["options"];bool linked=!options.empty();const std::string p=g.value("platform","");
  d.bordered_rect({52,94,1816,883},22,Color::rgb(0x0c1521),1,Color::rgb(0xffffff,.16f));
  d.push_clip({53,95,1814,880});backdrop(detail,.50f);d.gradient_rect_h({52,94,1816,882},0,Color::rgb(0x0b1420,.94f),Color::rgb(0x0b1420,.40f));d.gradient_rect({52,330,1816,647},0,Color::rgb(0x0b1420,.15f),Color::rgb(0x0b1420));d.pop_clip();
  button("circle",98,140,22);label("gamedetails","Game details",129,149,20,Color::rgb(0xd4e0ee),790);
  text(platform_name(g),1816,149,20,accent(),true,200,Align::right);
  poster(g,{84,194,352,468},13);
  auto lines=bold.wrap(g.value("title",""),43,1300);for(int n=0;n<std::min(2,(int)lines.size());n++)text(lines[n],480,236+n*49,43,Color::rgb(0xffffff),true,1300);
  text(g.value("genre","")+(g.value("date","").empty()?"":"  /  "+g.value("date","")),482,338,21,Color::rgb(0xc1cfdf),false,1240);
  d.bordered_rect({481,358,365,43},22,Color::rgb(0xffffff,.05f),1,Color::rgb(0xffffff,.22f));button("square",507,380,20);label(favorites.count(g.value("key",""))?"removefavorite":"favorite",favorites.count(g.value("key",""))?"Remove favorite":"Add to favorites",534,388,19,Color::rgb(0xe0e9f4),289);
  label("options","Download options",483,446,24,Color::rgb(0xe7eef8),624);text(linked?std::to_string(option+1)+" / "+std::to_string(options.size()):"",1195,446,18,Color::rgb(0x9ab2ca),false,120,Align::right);
  if(linked){int first=std::max(0,option-3);for(int n=first;n<std::min((int)options.size(),first+4);n++){auto&o=options[n];float y=463+(n-first)*77;bool focus=n==option;
   d.bordered_rect({480,y,735,67},11,Color::rgb(focus?0x29496a:0x162638,.95f),focus?2:1,Color::rgb(focus?0xc3e6ff:0x33475c));
   d.circle(504,y+32,8,Color::rgb(focus?0xedf6ff:0x546c83));if(focus)d.circle(504,y+32,3,Color::rgb(0x235b84));
   std::string source=o.value("method","")=="torrent"?"TORRENT / "+source_name(o.value("source","")):source_name(o.value("source",""));if(o.contains("catalogs"))source="BLACKBOX / Orbit - "+source;
   text(source,525,y+27,21,Color::rgb(0xf0f5fb),true,472);std::string variant=o.value("variant","");if(!o.value("collection","").empty())variant+=(variant.empty()?"":" / ")+o.value("collection","");if(variant.empty())variant="Standard edition";
   text(variant,525,y+51,15,Color::rgb(0xa6bdd2),false,468);text(o.value("format","CHD"),1193,y+25,17,accent(),true,170,Align::right);text(bytes(o.value("size",0.0)),1193,y+51,15,Color::rgb(0xd4e4f4),false,164,Align::right);
  }
  d.rounded_rect({480,797,735,57},28,Color::rgb(0xf1f6fd));button("cross",512,825,23,Color::rgb(0x152538));label("download","Add to downloads",546,835,25,Color::rgb(0x152538),639);
  }else{text(g.value("officialUrl","https://www.playstation.com/games/"),484,633,17,accent(),false,717);label("metadataonly","No verified download release linked",484,507,25,Color::rgb(0xe1ebf7),717);label("ps1import","Copy your game to the game folder",484,561,21,Color::rgb(0xa5bdd5),717);}
  d.bordered_rect({1243,421,574,433},14,Color::rgb(0x111f2f,.95f),1,Color::rgb(0x41596f,.7f));
  label("gamepath","Save game to",1271,459,23,Color::rgb(0xe0eafa),516);d.rounded_rect({1265,483,529,95},8,Color::rgb(0x091422));
  auto path=regular.wrap(game_path(p,g.value("key","")),19,482);for(int n=0;n<std::min(3,(int)path.size());n++)text(path[n],1281,513+n*24,19,Color::rgb(0xabc8e3),false,497);
  button("triangle",1287,610,20);label("change","Change folder",1317,619,20,Color::rgb(0xc6d9ee),460);
  d.line(1270,645,1790,645,1,Color::rgb(0xffffff,.10f));platform_icon(p,1284,676,48);text(emulator_name(p),1361,699,24,Color::rgb(0xffffff),true,418);label("emulator","Platform & emulator",1361,730,17,Color::rgb(0x96acc4),420);
  label(installed(g)?"installed":linked?"available":"metadataonly",installed(g)?"On your console":linked?"Ready to download":"No download source linked",1276,798,21,Color::rgb(installed(g)?0xa6e5c8:0xb4cce5),512);
  label("aboutgame","About this game",85,713,23,Color::rgb(0xe2edf9),350);text(platform_name(g)+"  /  "+emulator_name(p),87,752,18,Color::rgb(0xa8bed5),false,350);
  if(g.value("score",-1)>=0){d.star(99,787,8,Color::rgb(0xebcc96));text(g.contains("rating")?number(g["rating"].get<double>(),1)+" / 5  PS Store":std::to_string(g.value("score",0))+" / 100  Metacritic",123,796,17,Color::rgb(0xe6d7b9),true,314);}
  if(p=="ps5"||p=="ps4")label("consoleuntested","PKG installs automatically when enabled; compatibility depends on your console",481,891,18,Color::rgb(0xb4c4d7),1332);
  else if(p=="ps3")label("ps3notice","RPCS3 is experimental. Install PS3 PKG in RPCS3 with your game license.",481,891,18,Color::rgb(0xb4c4d7),1332);
  else if(p=="saturn"||p=="segacd"||p=="arcade")label("biosrequired","This core may need your BIOS files in RetroArch/system; set compatibility varies",481,891,18,Color::rgb(0xe1c397),1332);
  else label(linked?"sourcechecked":"ps1open",linked?"Source checked; availability can change":"Open the game from your emulator",481,891,18,Color::rgb(0xa6bdd5),1332);
  d.line(84,912,1817,912,1,Color::rgb(0xffffff,.10f));if(linked)hint("cross","download","Download",109,952,420);hint("square","favorite","Favorite",662,952,400);hint("circle","back","Back",1613,952,180);
 }

 void keyboard(){
  d.bordered_rect({338,123,1245,815},28,Color::rgb(0x121d2d),1,Color::rgb(0x496278));label(pathEditing?(pathTarget=="@magnet"?"magnet":pathTarget=="@hub-query"?"search":"gamepath"):"search",pathEditing?(pathTarget=="@magnet"?"Magnet link":pathTarget=="@hub-query"?"Search hub":"Game folder"):"Find your next game",386,188,34);text("QWERTY",1533,185,17,Color::rgb(0x7695ae),true,200,Align::right);
  d.bordered_rect({382,213,1154,76},15,Color::rgb(0x0d1b2d),1,Color::rgb(0x74616e));text((pathEditing?pathDraft.substr(pathDraft.size()>78?pathDraft.size()-78:0):query)+"|",412,263,pathEditing?24:32,Color::rgb(0xd5f4e5),true,1090);
  const std::string rows[]={"1234567890",pathEditing&&!pathUpper?"qwertyuiop":"QWERTYUIOP",pathEditing&&!pathUpper?"asdfghjkl":"ASDFGHJKL",pathEditing?(pathUpper?"ZXCVBNM/._-:?&=%":"zxcvbnm/._-:?&=%"):"ZXCVBNM"};
  for(int row=0;row<4;row++){float pitch=std::min(108.f,1130.f/rows[row].size()),width=rows[row].size()*pitch-8,x0=960-width/2;for(int col=0;col<(int)rows[row].size();col++){float x=x0+col*pitch,y=325+row*87;bool selectedKey=row==keyboardRow&&col==keyboardCol;d.bordered_rect({x,y,pitch-8,73},12,Color::rgb(selectedKey?0x285682:0x1d3045),selectedKey?2:1,Color::rgb(selectedKey?0xafdfff:0x354d64));text(std::string(1,rows[row][col]),x+(pitch-8)/2,y+47,27,Color::rgb(0xf3f8ff),true,95,Align::center);}}
  const char* keys[]={"space","backspace","apply"};const char* en[]={"Space","Delete","Search"};for(int n=0;n<3;n++){float x=415+n*367;bool selectedKey=keyboardRow==4&&keyboardCol==n;d.bordered_rect({x,691,355,72},14,Color::rgb(selectedKey?0x285682:0x1d3045),selectedKey?2:1,Color::rgb(selectedKey?0xafdfff:0x354d64));label(pathEditing&&n==2?"savepath":keys[n],pathEditing&&n==2?"Save":en[n],x+177,737,25,Color::rgb(0xe7f6ee),325,Align::center);}
  hint("cross","select","Type",422,873,160);hint("square","backspace","Delete",692,873,190);hint("triangle",pathEditing?"savepath":"apply",pathEditing?"Save":"Search",1000,873,190);if(pathEditing){hint("R2","clear","Clear",560,798,180);hint("L2","defaultpath","Default",950,798,230);hint("L3","lettercase","a / A",1398,798,130);}hint("circle","back","Back",1320,873,150);
 }

 void draw(){d.clear();chrome();if(platformPicker)draw_platform_picker();else if(tab==5)downloads();else if(tab==8)settings();else if(tab==12)hub_draw();else{d.push_opacity(.5f+.5f*contentIn);d.push_transform(1,0,0,0,(1-contentIn)*12);library();d.pop_transform();d.pop_opacity();}footer();if(modalAlpha>.02f)d.rounded_rect({0,0,1920,1080},0,Color::rgb(0x03070e,modalAlpha*.80f));if(detail>=0){d.push_transform(.97f+.03f*modalAlpha,960,540,0,(1-modalAlpha)*14);d.push_opacity(modalAlpha);details();d.pop_opacity();d.pop_transform();}if(filterOpen)filters();transfer_draw();if(searching)keyboard();if(exitDialog||cancelDialog){d.rounded_rect({500,360,920,332},28,Color::rgb(0x25202c));label(exitDialog?"exit":"confirmcancel",exitDialog?"Close Raff?":"Cancel this download?",960,464,38,Color::rgb(0xf0f5ff),840,Align::center);hint("cross","yes","Confirm",674,613,280);hint("circle","back","Back",1074,613,280);}if(now<toastUntil&&!download_notice_visible()){d.shadow({640,42,640,76},20,25,Color::rgb(0x000000,.4f));d.bordered_rect({640,42,640,76},20,Color::rgb(0x1b3550),1,Color::rgb(0x6c8e9d));label(toast,toast=="added"?"Added to downloads":toast=="exists"?"Already installed or queued":toast=="saved"?"Saved":toast=="wait"?"One moment...":toast=="offline"?"Service unavailable":"Could not complete the action",960,91,27,Color::rgb(0xd8f4e7),590,Align::center);}draw_download_notice();draw_update_overlay();label("developedby","Developed by Mohammed Al-Ruwaili",1460,1075,14,Color::rgb(0x8ba0b9),650,Align::right);text("X / Twitter  @MohamedFAlrwili",1842,1075,15,Color::rgb(0x8ba0b9),false,350,Align::right);renderer.begin();renderer.draw(d);}
};
void screenshot(int w,int h,const std::string&name){std::vector<unsigned char>p((size_t)w*h*4);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,p.data());for(size_t i=0;i<p.size();i+=4)std::swap(p[i],p[i+2]);unsigned char hdr[54]={0x42,0x4d};auto put=[&](int o,uint32_t n){memcpy(hdr+o,&n,4);};put(2,54+p.size());put(10,54);put(14,40);put(18,w);put(22,h);hdr[26]=1;hdr[28]=32;put(34,p.size());FILE*f=fopen(("/app0/"+name).c_str(),"wb");if(!f)f=fopen(("/download0/"+name).c_str(),"wb");if(f){fwrite(hdr,1,54,f);fwrite(p.data(),1,p.size(),f);fclose(f);hui::sys::log("Raff preview captured");}}
}
int main(){hui::sys::log("Raff 1.1.1 native library Games application");hui::ps5::Display display;if(!display.open(1920,1080))hui::sys::park();App app;app.init();hui::ps5::Pad pad;pad.open();pad.set_light_bar(80,166,255);hui::InputTracker tracker;auto previous=hui::sys::monotonic_us(),receipt=previous;int frames=0;bool captured=false;const auto opened=previous;hui::sys::hide_splash_screen();for(;;){auto current=hui::sys::monotonic_us();float dt=std::clamp((current-previous)/1000000.f,0.001f,0.1f);previous=current;app.now=current/1000000.;hui::PadSample samples[64];auto n=pad.read(samples);auto input=tracker.update({samples,n},current);float loadingProgress=(current-opened)/2200000.f;if(loadingProgress>=1)app.real_input(input);app.update(dt);app.qa_tick();if(loadingProgress<1)app.loading(loadingProgress);else app.draw();app.renderer.present(0,display.width(),display.height());if((!captured&&app.frame>180)||app.captureRequested){screenshot(display.width(),display.height(),app.screenshotName);app.screenshotName="raff-preview.bmp";captured=true;app.captureRequested=false;}display.swap();pad.tick(dt);frames++;if(current-receipt>5000000){hui::sys::log("Raff frame rate %.1f fps, %zu draw calls",frames*1000000.0/(current-receipt),app.renderer.last_draw_calls());log_heap("render");receipt=current;frames=0;}}}
