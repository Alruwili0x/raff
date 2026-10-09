// Native cinema storefront. Included inside App; all artwork stays asynchronous.
raff::Shelves shelves;
std::map<std::string,GLuint> emulatorIcons;
bool reducedMotion=false;
bool gridMode=true;
float gridOffset=0;
int heroIndex=-1,previousHero=-1,requestedHero=-1;
float heroFade=1,selectionAge=1;
double heroRequestedAt=0;
// Wide PS2 box scans contain the back, spine and front. Show the front panel.
Rect cover_uv(const Json&g,const Cover&c){
 Rect uv{0,c.uvY,1,c.uvH};float ratio=c.width/(c.height*c.uvH);
 if(g.value("platform","")=="ps2"&&ratio>=1.20f&&ratio<=1.85f){uv.x=.53f;uv.w=.47f;}
 return uv;
}

std::string emulator_name(const std::string&p){
 return p=="ps1"?"PSXS5":p=="ps3"?"RPCS3":p=="ps2"?"PS5SX2":p=="switch"?"ProsperoEden":p=="xbox"?"XPSemu":p=="xbox360"?"PS5X360":p=="dreamcast"?"PSFlycast":platformDefaults.contains(p)?"RetroArch":"PlayStation";
}
void init_cinema(){
 for(const auto*p:{"ps1","ps2","ps3","ps4","ps5","switch","xbox","xbox360"})emulatorIcons[p]=texture_file(std::string("/app0/assets/platforms/")+p+".png");
}
void rebuild_shelves(std::string preferred=""){
 std::vector<raff::Shelf> rows;std::vector<int> all;
 for(int n=0;n<(int)visible.size();n++)all.push_back(n);
 if(sort!=0||!query.empty()||tab==6||tab==7){rows.push_back({!query.empty()?"searchresults":sortKeys[sort],!query.empty()?"Search results":sortNames[sort],all});}
 else if(!all.empty()){
  std::vector<int> featured,rated,popular,saved;
  for(int n:all){auto&g=catalog[visible[n]];if(featured.size()<18&&!g.value("cover","").empty())featured.push_back(n);if(g.value("score",-1)>=0)rated.push_back(n);if(g.value("popularityScore",g.value("downloads",-1LL))>0)popular.push_back(n);if(favorites.count(g.value("key","")))saved.push_back(n);}
  if(featured.empty())featured.assign(all.begin(),all.begin()+std::min(size_t(18),all.size()));
  std::stable_sort(popular.begin(),popular.end(),[&](int a,int b){auto&x=catalog[visible[a]];auto&y=catalog[visible[b]];return x.value("popularityScore",x.value("downloads",-1LL))>y.value("popularityScore",y.value("downloads",-1LL));});
  if(rated.size()>24)rated.resize(24);if(popular.size()>24)popular.resize(24);
  rows.push_back({"featured","Featured",featured});
  if(!saved.empty())rows.push_back({"favorites","Your favorites",saved});
  if(!popular.empty())rows.push_back({"popular","Most downloaded",popular});
  if(!rated.empty())rows.push_back({"rating","Top rated",rated});
  rows.push_back({"allgames","Explore all games",all});
 }
 shelves.reset(std::move(rows),selected,preferred);
 if(shelves.selected()>=0)selected=shelves.selected();
}
void sync_cinema(float dt){
 shelves.animate(dt,reducedMotion);
 const float target=raff::Grid::first(selected)/6*326.f;gridOffset+=(target-gridOffset)*(reducedMotion?1.f:1.f-std::exp(-18*dt));
 selectionAge=std::min(1.f,selectionAge+dt*7);
 if(visible.empty()){heroIndex=previousHero=requestedHero=-1;return;}
 int next=visible[selected];
 if(requestedHero!=next){requestedHero=next;heroRequestedAt=now;selectionAge=0;cover(catalog[next]);}
 // A short dwell prevents large artwork uploads while holding a direction.
 if(heroIndex<0||(heroIndex!=next&&(reducedMotion||now-heroRequestedAt>.10))){previousHero=heroIndex;heroIndex=next;heroFade=reducedMotion?1:0;}
 heroFade=std::min(1.f,heroFade+dt*4.5f);
}
void brand(float x,float y,float s,float reveal=1){
 const unsigned colors[]={0xb4e6ff,0x65bdf8,0x348af0};
 for(int n=0;n<3;n++){float a=std::clamp(reveal*2-n*.20f,0.f,1.f);float h=(n==1?35.f:27.f)*s;d.push_opacity(a);d.rounded_rect({x+n*11*s,y+36*s-h+(1-a)*14*s,8*s,h},2*s,Color::rgb(colors[n]));d.pop_opacity();}
 d.rounded_rect({x-3*s,y+41*s,38*s,3*s},1.5*s,Color::rgb(0xffffff,reveal));
}
void platform_icon(const std::string&p,float x,float y,float size,Color tint=Color::rgb(0xffffff)){
 auto it=emulatorIcons.find(p);
 if(it!=emulatorIcons.end()&&it->second){const float scale=p=="ps2"||p=="ps3"||p=="ps4"||p=="ps5"?1.6f:1.f;const float extra=size*(scale-1)/2;d.image(it->second,{x-extra,y-extra,size*scale,size*scale},kFullUv,tint,0);return;}
 float cx=x+size/2,cy=y+size/2;
 if(p=="downloads"){d.line(cx,y+size*.15f,cx,y+size*.65f,2.5f,tint);d.line(cx-size*.17f,cy+size*.02f,cx,cy+size*.19f,2.5f,tint);d.line(cx+size*.17f,cy+size*.02f,cx,cy+size*.19f,2.5f,tint);d.line(x+size*.16f,y+size*.83f,x+size*.84f,y+size*.83f,2.5f,tint);}
 else if(p=="favorites")d.star(cx,cy,size*.38f,tint,2);
 else if(p=="settings"){d.ring(cx,cy,size*.29f,2,tint);d.ring(cx,cy,size*.1f,2,tint);for(int n=0;n<8;n++){float a=n*3.1415926f/4;d.line(cx+cosf(a)*size*.30f,cy+sinf(a)*size*.30f,cx+cosf(a)*size*.42f,cy+sinf(a)*size*.42f,3,tint);}}
 else if(p=="local"){d.bordered_rect({x+3,y+5,size-6,size-10},3,Color::rgb(0,0),2,tint);d.line(x+9,y+size*.36f,x+size-9,y+size*.36f,2,tint);d.circle(x+size-10,y+size-11,2,tint);}
 else {d.bordered_rect({x+2,y+size*.23f,size-4,size*.55f},size*.13f,Color::rgb(0,0),2,tint);d.line(x+size*.22f,cy,x+size*.42f,cy,2,tint);d.line(x+size*.32f,cy-size*.1f,x+size*.32f,cy+size*.1f,2,tint);d.circle(x+size*.70f,cy-size*.06f,2,tint);d.circle(x+size*.80f,cy+size*.06f,2,tint);}
}
void chrome(){
 d.rounded_rect({0,0,1920,1080},0,Color::rgb(0x080d16));
 if(!platformPicker&&!gridMode&&platform_tab()&&!visible.empty()){backdrop(previousHero,1-heroFade);backdrop(heroIndex,heroFade);d.gradient_rect_h({0,100,1550,880},0,Color::rgb(0x080d16,.98f),Color::rgb(0x080d16,.08f));}
 d.rounded_rect({0,0,1920,108},0,Color::rgb(0x0c1320));
 brand(78,31,1);text("raff",127,72,42,Color::rgb(0xf4f8ff),true);
 const char*keys[]={"allgames","library","favorites","downloads","emulators","settings"};
 const char*names[]={"Games","My library","Favorites","Downloads","Emulators","Settings"};
 for(int n=0;n<6;n++){float x=338+n*174;bool active=n==header_index(),focus=headerFocus&&headerCursor==n;
  if(active||focus)d.rounded_rect({x,30,160,48},12,Color::rgb(focus?0xf0f7ff:0xcd1268));
  label(keys[n],names[n],x+80,63,20,Color::rgb(focus?0x0a1424:active?0xf5faff:0xa8b7ca),148,Align::center);
 }
 d.bordered_rect({1634,32,112,44},12,Color::rgb(0x111f30),1,Color::rgb(0x31435b));text(ar?"AR":"EN",1651,61,20,Color::rgb(0xe7eff9),true,38);text("L3",1715,58,13,Color::rgb(0x91a6c0),true,28,Align::center);
 d.circle(1790,52,4,Color::rgb(state.value("ready",false)?0x75d3b0:0xe9b170));text("1.1.1",1806,58,13,Color::rgb(0x93a8c1),false,65);
 auto jobs=download_jobs();if(!jobs.empty()){d.circle(992,35,9,accent());text(std::to_string(jobs.size()),992,39,11,Color::rgb(0x071624),true,16,Align::center);}
 if(platform_tab()&&!platformPicker){
  const int start=std::clamp(platform_index()-3,0,platformCount-8);const float target=76+(platform_index()-start)*224.f;navX+=(target-navX)*(reducedMotion?1.f:1.f-std::exp(-18*lastDt));
  for(int n=0;n<8;n++){int t=platformOrder[start+n];float x=76+n*224;bool active=t==tab;
   if(active)d.rounded_rect({x,132,208,58},12,Color::rgb(0x1d3047));
   platform_icon(tabKeys[t],x+15,146,28,Color::rgb(0xffffff,active?1.f:.60f));
   label(tabKeys[t],tabNames[t],x+64,169,19,Color::rgb(active?0xf4f9ff:0x98aac0),137);
   if(active)d.rounded_rect({x+62,187,127,3},1,accent());
  }
  text("L1 / R1  -  "+std::to_string(platform_index()+1)+" / "+std::to_string(platformCount),1840,219,13,Color::rgb(0x7890a9),false,400,Align::right);
  if(navFocus)d.bordered_rect({navX-2,130,212,62},14,Color::rgb(0,0),2,Color::rgb(0xe0f2ff));
 }
}

void backdrop(int index,float alpha){
 if(index<0||index>=(int)catalog.size()||alpha<=0)return;
 auto art=catalog[index];bool wide=!art.value("hero","").empty();if(wide)art["cover"]=art["hero"];auto&c=cover(art);if(!c.texture)return;
 const float drift=reducedMotion?0:std::sin(float(now*.08))*5;
 Rect box{0,90,1926,982};Rect uv=wide?Rect{0,0,1,1}:cover_uv(art,c);float ratio=box.w/box.h,sourceRatio=(c.width*uv.w)/(c.height*uv.h);
 if(sourceRatio>ratio){float w=uv.w*ratio/sourceRatio;uv.x+=(uv.w-w)*.65f;uv.w=w;}else{float h=uv.h*sourceRatio/ratio;uv.y+=(uv.h-h)*.25f;uv.h=h;}
 box.x+=drift;d.image(c.texture,box,uv,Color::rgb(0xffffff,alpha*(gridMode?.25f:.84f)));
}
void cinema_hero(){
 if(visible.empty())return;auto&g=catalog[visible[selected]];const std::string p=g.value("platform","");
 label(tab==6?"favorites":tab==7?"library":"storetitle",tab==6?"Your favorites":tab==7?"Your game library":"DISCOVER YOUR NEXT GAME",76,233,18,Color::rgb(0xb8c7d8),770);
 auto lines=bold.wrap(g.value("title",""),62,1020);float baseline=317;
 for(int n=0;n<std::min(2,(int)lines.size());n++)text(lines[n],73,baseline+n*66,62,Color::rgb(0xffffff),true,1030);
 float metaY=lines.size()>1?421:360;
 d.bordered_rect({76,metaY-21,64,28},6,Color::rgb(0xffffff,.09f),1,Color::rgb(0xffffff,.22f));text(platform_name(g),108,metaY-1,14,Color::rgb(0xdae5f0),true,58,Align::center);
 std::string meta=g.value("genre","");if(!g.value("date","").empty())meta+=(meta.empty()?"":"  /  ")+g.value("date","").substr(0,4);
 if(meta.empty())meta=emulator_name(p);text(meta,157,metaY,21,Color::rgb(0xbfcad9),false,710);
 if(g.value("score",-1)>=0){d.star(900,metaY-8,8,Color::rgb(0xedd4a4));text(g.contains("rating")?number(g["rating"].get<double>(),1)+" / 5":std::to_string(g.value("score",0))+" / 100",923,metaY,18,Color::rgb(0xf1e3c8),true,125);}
 d.rounded_rect({76,464,270,55},28,Color::rgb(0xf4f6fa));button("cross",108,491,22,Color::rgb(0x111823));label("gamedetails","View game",138,500,23,Color::rgb(0x111823),188);
 d.bordered_rect({365,464,195,55},28,Color::rgb(0x121e2d,.83f),1,Color::rgb(0xffffff,.27f));button("R3",398,491,21);label("browsegrid","Browse",429,500,21,Color::rgb(0xf2f6fc),113);
 if(favorites.count(g.value("key",""))){d.circle(601,491,26,Color::rgb(0x121e2d,.86f));d.star(601,491,11,Color::rgb(0xefd094));}
 if(!query.empty()){d.bordered_rect({1300,218,544,51},10,Color::rgb(0x0b1521,.85f),1,Color::rgb(0xffffff,.12f));text(query,1323,251,22,Color::rgb(0xffffff),true,496);}
 // The emulator badge stays low contrast so game artwork remains the focal point.
 d.rounded_rect({1517,448,327,73},13,Color::rgb(0x0c1420,.76f));platform_icon(p,1535,467,36);
 text(emulator_name(p),1597,479,22,Color::rgb(0xf1f6fd),true,226);label("emulator","Platform & emulator",1597,503,15,Color::rgb(0xa6b7cc),227);
}
void store_card(int index,float x,float y,bool focused,bool compact=false){
 auto&g=catalog[visible[index]];const auto k=g.value("key","");const float ph=compact?238:300,whole=ph+58;
 d.push_transform(focused?1+.012f*(reducedMotion?1:selectionAge):1,x+136,y+whole/2,0,focused?-2:0);
 if(focused){d.shadow({x-3,y-3,278,ph+6},11,16,Color::rgb(0,.6f));d.bordered_rect({x-4,y-4,280,ph+8},12,Color::rgb(0x243950),2.5f,Color::rgb(0xffffff));}
 poster(g,{x,y,272,ph},8);
 if(installed(g)){d.rounded_rect({x+10,y+10,30,27},7,Color::rgb(0x0c221c,.93f));d.line(x+17,y+23,x+23,y+29,2,Color::rgb(0x9bdfbf));d.line(x+23,y+29,x+34,y+16,2,Color::rgb(0x9bdfbf));}
 if(favorites.count(k)){d.circle(x+249,y+23,13,Color::rgb(0x101923,.9f));d.star(x+249,y+23,7,Color::rgb(0xf6d192));}
 text(g.value("title",""),x+2,y+ph+27,20,Color::rgb(focused?0xffffff:0xd4deeb),true,268);
 text(platform_name(g),x+2,y+ph+49,12,Color::rgb(0x96a9bf),true,80);
 const auto transfer=jobByKey.find(k);
 if(transfer!=jobByKey.end()){auto&j=transfer->second;float progress=j.value("total",0.0)>0?std::clamp(float(j.value("received",0.0)/j.value("total",0.0)),0.f,1.f):0;d.rounded_rect({x,y+ph-4,272,4},0,Color::rgb(0x304154));if(progress>0)d.rounded_rect({x,y+ph-4,272*progress,4},0,accent());text(number(progress*100,0)+"%",x+270,y+ph+49,13,accent(),true,76,Align::right);}
 else if(g.value("size",0LL)>0)text(bytes(g.value("size",0.0)),x+270,y+ph+49,13,Color::rgb(0x96a9bf),false,100,Align::right);
 d.pop_transform();
}
void library(){
 if(catalogPending){label("loading","Loading your library",960,520,36,Color::rgb(0xd2e8fa),1500,Align::center);return;}
 if(visible.empty()){platform_icon(platform_tab()?tabKeys[tab]:"local",925,376,70,Color::rgb(0xffffff,.6f));label("empty","No games found",960,530,38,Color::rgb(0xf5f1f6),1400,Align::center);label("filterhint","Change filters or try another search",960,585,26,Color::rgb(0xaaa4b4),1400,Align::center);return;}
 if(gridMode||tab==7||tab==6){
  d.rounded_rect({0,208,1920,773},0,Color::rgb(0x080d16));
  const char*titleKey=tab==7?"library":tab==6?"favorites":!query.empty()?"searchresults":tabKeys[tab];
  const char*titleName=tab==7?"My game library":tab==6?"Favorites":!query.empty()?"Search results":tabNames[tab];
  label(titleKey,titleName,78,254,34,Color::rgb(0xf4f8ff),1020);
  text(!query.empty()?query:platform_tab()?emulator_name(tabKeys[tab]):"",80,290,17,Color::rgb(0x829bb8),false,530);
  text(std::to_string(selected+1)+" / "+std::to_string(visible.size()),1060,289,17,Color::rgb(0x94a9c2),false,260,Align::right);
  d.bordered_rect({1198,226,338,62},14,Color::rgb(0x111f30),1,Color::rgb(0x2a3d56));button("triangle",1228,256,21);label("searchshort","Search",1258,264,21,Color::rgb(0xcbd8e8),244);
  d.bordered_rect({1556,226,287,62},14,Color::rgb(0x111f30),1,Color::rgb(0x2a3d56));button("square",1586,256,21);label("sort","Sort / filter",1616,264,21,Color::rgb(0xcbd8e8),209);
  d.push_clip({59,314,1810,653});int first=std::max(0,int(gridOffset/326)-1)*6,last=std::min((int)visible.size(),first+24);
  for(int n=first;n<last;n++){float x=76+n%6*292,y=326+n/6*326-gridOffset;if(y+304<314||y>967)continue;store_card(n,x,y,n==selected&&!navFocus&&!headerFocus,true);}d.pop_clip();
 }else{
  cinema_hero();d.push_clip({52,540,1816,437});
  for(int row=0;row<(int)shelves.rows.size();row++){
   float y=573+row*raff::Shelves::rowHeight-shelves.vertical;if(y+398<540||y>977)continue;auto&r=shelves.rows[row];
   label(r.key,r.name,76,y,27,Color::rgb(0xf5f8fd),1120);
   text(std::to_string(r.cursor+1)+" / "+std::to_string(r.items.size()),1842,y,18,Color::rgb(0xa1b3c9),false,235,Align::right);
   int start=std::max(0,int(r.offset/raff::Shelves::pitch)-1),end=std::min(int(r.items.size()),start+8);
   for(int pos=start;pos<end;pos++){float x=76+pos*raff::Shelves::pitch-r.offset;if(x+280<52||x>1868)continue;store_card(r.items[pos],x,y+28,row==shelves.active&&pos==r.cursor&&!navFocus&&!headerFocus);}
  }d.pop_clip();
 }
 d.gradient_rect({52,960,1816,21},0,Color::rgb(0x080d16,0),Color::rgb(0x080d16));
}

void loading(float progress){
 d.clear();d.rounded_rect({0,0,1920,1080},0,Color::rgb(0x090a0e));
 float p=std::clamp(progress,0.f,1.f);d.glow({745,280,430,85},55,180,Color::rgb(0x66baff,.13f));
 brand(900,242,3.8f,reducedMotion?1:std::min(1.f,p*2));
 label("rafftitle","Raff - Game Library",960,532,58,Color::rgb(0xf9f5f3),1300,Align::center);
 label("splashfeature","Your games, one beautiful library",960,598,30,Color::rgb(0xbeb5c5),1430,Align::center);
 label("splashfeature2","Multiple downloads / Custom folders / Arabic & English",960,647,25,Color::rgb(0x978c9f),1580,Align::center);
 d.rounded_rect({738,715,444,4},2,Color::rgb(0x2b242f));if(p>0)d.gradient_rect_h({738,715,444*p,4},2,Color::rgb(0x429aff),Color::rgb(0x9cdcff));
 label("loading","Preparing your library",960,765,23,Color::rgb(0xb2a6ba),1100,Align::center);
 label("developer","Developed by Mohammed Al-Ruwaili",960,927,26,Color::rgb(0xd9ccd9),1600,Align::center);text("X / Twitter  @MohamedFAlrwili",960,971,23,Color::rgb(0x8bc8f5),false,1500,Align::center);renderer.begin();renderer.draw(d);
}
