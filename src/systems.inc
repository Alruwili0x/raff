// The home screen is a small platform index; game catalogs remain lazy-loaded.
Json systems=Json::array();int makerFilter=0;float systemFocus=1;int lastSystemCursor=-1;
const int homeOrder[26]={2,3,11,0,4,21,1,22,23,15,14,13,16,17,18,19,20,9,10,29,27,28,24,25,26,30};
int maker(int t){return t==0||t==2||t==3||t==4||t==11||t==21?1:t==9||t==10?3:t>=24&&t<=29?4:t==30?5:2;}
std::vector<int> system_tabs(){std::vector<int>result;for(int t:homeOrder)if(!makerFilter||maker(t)==makerFilter)result.push_back(t);return result;}
void init_systems(){systems=load_json("/app0/assets/systems.json");if(!systems.is_array())systems=Json::array();}
Json system_info(int t){for(const auto&s:systems)if(s.value("platform","")==tabKeys[t])return s;return Json{{"platform",tabKeys[t]},{"title",tabNames[t]},{"count",0}};}
void systems_open(){platformPicker=true;headerFocus=navFocus=false;detail=-1;filterOpen=false;auto tabs=system_tabs();auto it=std::find(tabs.begin(),tabs.end(),lastPlatformTab);platformCursor=it==tabs.end()?0:int(it-tabs.begin());}
void systems_input(const hui::InputFrame&i){
 auto press=[&](Action a){return i.is_pressed(a);};int dx=i.nav==Direction::right?1:i.nav==Direction::left?-1:0,dy=i.nav==Direction::down?1:i.nav==Direction::up?-1:0;
 if(press(Action::l3)){ar=!ar;dispatch("/settings",{{"language",ar?"ar":"en"}});}
 if(press(Action::menu)){choose_section(5);return;}
 if(headerFocus){headerCursor=std::clamp(headerCursor+dx,0,5);if(press(Action::confirm)){choose_section(headerCursor);return;}if(dy>0||press(Action::back))headerFocus=false;return;}
 if(press(Action::back)){exitDialog=true;return;}
 int filterStep=press(Action::page_next)?1:press(Action::page_prev)?-1:0;if(filterStep){makerFilter=(makerFilter+filterStep+6)%6;platformCursor=0;}
 auto tabs=system_tabs();if(tabs.empty())return;int page=press(Action::jump_next)?10:press(Action::jump_prev)?-10:0;
 if(dy<0&&platformCursor<5){headerFocus=true;headerCursor=0;return;}
 platformCursor=std::clamp(platformCursor+dx+dy*5+page,0,int(tabs.size())-1);
 if(press(Action::confirm)){int t=tabs[platformCursor];platformPicker=false;change_tab(t);gridMode=true;navFocus=false;return;}
 if(press(Action::north)){platformPicker=false;change_tab(tabs[platformCursor]);searching=true;pathEditing=false;keyboardRow=1;keyboardCol=0;}
}
void draw_platform_picker(){
 auto tabs=system_tabs();if(tabs.empty())return;platformCursor=std::clamp(platformCursor,0,int(tabs.size())-1);const int page=platformCursor/10;
 if(lastSystemCursor!=platformCursor){lastSystemCursor=platformCursor;systemFocus=0;}systemFocus=std::min(1.f,systemFocus+lastDt*8);
 d.rounded_rect({0,108,1920,877},0,Color::rgb(0x070a12));
 d.glow({95,223,520,2},30,140,Color::rgb(0x07cdea,.055f));d.glow({1470,813,300,2},30,160,Color::rgb(0xff087b,.065f));
 label("systemshome","Your worlds. One library.",86,181,37,Color::rgb(0xf5f4fc),1230);
 text(std::to_string(tabs.size()),1828,169,31,Color::rgb(0x59d4e8),true,100,Align::right);label("systemslabel","systems",1737,169,18,Color::rgb(0x8d9bb5),200,Align::right);
 const char*names[]={"All systems","PlayStation","Nintendo","Xbox","SEGA","Arcade"};const char*keys[]={"allsystems","makerps","makernintendo","makerxbox","makersega","makerarcade"};
 for(int n=0;n<6;n++){float x=86+n*294;bool active=n==makerFilter;d.bordered_rect({x,211,278,51},15,Color::rgb(active?0x075b78:0x111724),1,Color::rgb(active?0x26bddc:0x263043));label(keys[n],names[n],x+139,245,21,Color::rgb(active?0xf3fbff:0xa4b0c7),250,Align::center);}
 const unsigned colors[]={0x58bbfa,0xfe398f,0x47dc94,0x7d84ff,0xf2ae5d};
 for(int n=page*10;n<std::min(int(tabs.size()),page*10+10);n++){
  int t=tabs[n],slot=n-page*10;float x=86+(slot%5)*354,y=296+(slot/5)*264;bool focus=n==platformCursor&&!headerFocus;auto g=system_info(t);unsigned hue=colors[maker(t)-1];
  if(focus)d.glow({x+5,y+7,322,236},22,14,Color::rgb(0xff087b,reducedMotion?.21f:.12f+systemFocus*.09f));
  d.bordered_rect({x,y,332,243},22,Color::rgb(0x141c2b),1,Color::rgb(0x2a3548));
  auto&art=cover(g);if(art.texture){Rect uv=cover_uv(g,art);float sourceRatio=art.width*uv.w/(art.height*uv.h),targetRatio=332.f/243.f;if(sourceRatio>targetRatio){float w=uv.w*targetRatio/sourceRatio;uv.x+=(uv.w-w)/2;uv.w=w;}else{float h=uv.h*sourceRatio/targetRatio;uv.y+=(uv.h-h)*.28f;uv.h=h;}d.image(art.texture,{x,y,332,243},uv,Color::rgb(0xffffff,.66f),22);}
  d.rounded_rect({x,y,332,243},22,Color::rgb(0x030813,.42f));d.gradient_rect_h({x,y,332,243},22,Color::rgb(0x070a11,.13f),Color::rgb(0x070a11,.77f));
  d.rounded_rect({x+15,y+14,123,25},8,Color::rgb(0x080e18,.92f));d.rounded_rect({x+15,y+20,3,13},1,Color::rgb(hue));text(names[maker(t)],x+27,y+31,12,Color::rgb(0xe7f4ff),true,101);
  auto logo=emulatorIcons.find(tabKeys[t]);if(logo!=emulatorIcons.end()&&logo->second)platform_icon(tabKeys[t],x+122,y+64,88);else text(g.value("mark",tabNames[t]),x+166,y+118,42,Color::rgb(0xf6f9ff),true,278,Align::center);
  d.rounded_rect({x+1,y+166,330,76},20,Color::rgb(0x070b13,.92f));label(tabKeys[t],tabNames[t],x+19,y+199,23,Color::rgb(0xf9f7ff),295);
  text(g.value("year",""),x+20,y+225,15,Color::rgb(0x929eb5),true,100);text(std::to_string(g.value("count",0)),x+310,y+225,16,Color::rgb(0x67d6e9),true,115,Align::right);
  if(focus)d.bordered_rect({x-3,y-3,338,249},25,Color::rgb(0,0),3,Color::rgb(0xff1687));
 }
 const int t=tabs[platformCursor];auto g=system_info(t);
 d.bordered_rect({86,837,1748,131},21,Color::rgb(0x101622),1,Color::rgb(0x2b3449));platform_icon(tabKeys[t],113,867,62);
 label(tabKeys[t],tabNames[t],211,887,29,Color::rgb(0xf4f7ff),1000);
 text(emulator_name(tabKeys[t]),211,927,19,Color::rgb(0x67d6e9),true,650);label("systemshint","Choose a system to explore its games",716,925,20,Color::rgb(0x9caac3),730);
 d.rounded_rect({1465,875,339,57},16,Color::rgb(0xf90876));button("cross",1492,904,24);label("opensystem","Open library",1530,912,22,Color::rgb(0xffffff),254);
 text(std::to_string(page+1)+" / "+std::to_string((tabs.size()+9)/10),1813,823,13,Color::rgb(0x8a97af),false,140,Align::right);
}
