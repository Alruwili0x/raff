// Settings use four stable groups. The right panel describes the selected
// action; long paths are kept away from labels and controller focus borders.
int settingsGroup=0,settingsRow=0;bool settingsNav=false,updateDialog=false,updatePending=false;
std::string noticedVersion,updateError;double updateNoticeUntil=0;
std::vector<int> settings_items(){if(settingsGroup==0)return{0,13};if(settingsGroup==1)return{1,2,15,8,9};if(settingsGroup==2)return{3,4,14,5,6,10,11,12,7};return{16};}
Json update_state(){return state.value("updates",Json::object());}
void update_tick(){
 Json r;if(raff::take_result("app-update",r)){updatePending=false;if(r.contains("error"))updateError=r.value("error","");else{updateError.clear();state["updates"]=r;if(r.value("phase","")=="applying")hui::sys::quit();}}
 auto u=update_state();auto v=u.value("release",Json::object());if(!v.is_null()&&u.value("phase","")=="available"&&v.value("version","")!=noticedVersion){noticedVersion=v.value("version","");updateNoticeUntil=now+16;}
}
void update_action(){
 if(updatePending)return;auto p=update_state().value("phase","");updateError.clear();
 if(p=="ready"){updateDialog=true;return;}
 if(p=="unconfigured"||p=="checking"||p=="downloading"||p=="verifying")return;
 auto r=update_state().value("release",Json::object());updatePending=true;
 raff::request("app-update",(p=="available"||p=="error"&&!r.is_null()&&!r.empty())?"/updates/download":"/updates/check",Json::object());
}
void settings_input(const hui::InputFrame&i){
 auto press=[&](Action a){return i.is_pressed(a);};int dx=i.nav==Direction::right?1:i.nav==Direction::left?-1:0,dy=i.nav==Direction::down?1:i.nav==Direction::up?-1:0;
 if(press(Action::l3)){ar=!ar;dispatch("/settings",{{"language",ar?"ar":"en"}});return;}
 if(press(Action::back)){if(!settingsNav){settingsNav=true;return;}choose_section(0);return;}
 if(press(Action::jump_prev)||press(Action::jump_next)){settingsGroup=(settingsGroup+(press(Action::jump_next)?1:3))%4;settingsRow=0;return;}
 if(settingsNav){if(dy<0&&settingsGroup==0){headerFocus=true;headerCursor=5;return;}settingsGroup=std::clamp(settingsGroup+dy,0,3);settingsRow=0;if(dx>0||press(Action::confirm))settingsNav=false;return;}
 auto items=settings_items();if(dy<0&&settingsRow==0){settingsNav=true;return;}settingsRow=std::clamp(settingsRow+dy,0,(int)items.size()-1);setting=items[settingsRow];
 if(settingsGroup==3){if(press(Action::confirm))update_action();if(press(Action::north)&&!updatePending){updatePending=true;raff::request("app-update","/updates/check",Json::object());}return;}
 if(setting==16)return;
 if(press(Action::confirm)&&setting==15){transferModal="limits";transferSetting=0;return;}
 if((press(Action::confirm)||dx)&&setting==13){reducedMotion=!reducedMotion;viewTouched=true;notify("saved");return;}
 if((dx||press(Action::confirm))&&(setting==8||setting==9)){const char*k=setting==8?"autoInstall":"removePackages";dispatch("/settings",{{k,!state["settings"].value(k,false)}});return;}
 if(press(Action::confirm)&&setting>=3){if(setting==7){dispatch("/scan-paths",Json::object());notify("wait");return;}const char*p=setting==3?"ps2":setting==4?"switch":setting==5?"ps5":setting==6?"ps4":setting==10?"ps1":setting==11?"xbox":setting==12?"xbox360":"ps3";open_path(p,p);return;}
 if(dx||press(Action::confirm)){int step=dx?dx:1;if(setting==0){ar=!ar;dispatch("/settings",{{"language",ar?"ar":"en"}});}if(setting==1)dispatch("/settings",{{"parallel",std::clamp(state["settings"].value("parallel",3)+step,1,4)}});if(setting==2)dispatch("/settings",{{"connections",std::clamp(state["settings"].value("connections",16)+step*2,1,16)}});}
}
void settings(){
 label("settings","Settings",78,181,38);label("settingsintro","Make Raff yours",78,222,21,Color::rgb(0x92a9c4),1100);
 const char*gkeys[]={"appearancegroup","downloadgroup","pathsgroup","updatesgroup"},*gnames[]={"Appearance","Downloads","Game folders","About & updates"};
 for(int n=0;n<4;n++){float y=277+n*82;bool active=settingsGroup==n;d.bordered_rect({76,y,286,65},13,Color::rgb(active?0x223952:0x101d2d),settingsNav&&active?2:1,Color::rgb(settingsNav&&active?0xd6edff:0x27384d));label(gkeys[n],gnames[n],100,y+41,22,Color::rgb(active?0xf1f7ff:0x9bb1cc),242);}
 label("settingsgroupshint","L2 / R2 changes group",86,649,17,Color::rgb(0x91a7c0),265);
 if(settingsGroup==3){draw_update_page();return;}
 const char*keys[]={"language","parallel","connections","ps2path","switchpath","ps5path","ps4path","scanpaths","autoInstall","removePackages","ps1path","xboxpath","xbox360path","reducedmotion","ps3path","torrentsettings"};
 const char*names[]={"Language","Simultaneous games","Smart connection ceiling","PS2 folder","Switch folder","PS5 folder","PS4 folder","Find game folders","Install PKG automatically","Delete verified installed PKG","PS1 folder","Xbox folder","Xbox 360 folder","Reduced motion","PS3 folder","Torrent bandwidth & seeding"};
 auto items=settings_items();for(int row=0;row<(int)items.size();row++){int n=items[row];float y=277+row*69;bool focus=row==settingsRow&&!settingsNav;
  d.bordered_rect({399,y,913,60},12,Color::rgb(focus?0x223c58:0x111e2e),focus?2:1,Color::rgb(focus?0xc8e7ff:0x26384c));label(keys[n],names[n],424,y+38,22,Color::rgb(0xeaf3ff),650);
  std::string v;if(n<3)v=n==0?(ar?"AR":"EN"):std::to_string(state["settings"].value(n==1?"parallel":"connections",n==1?3:16));if(!v.empty())text(v,1285,y+39,23,accent(),true,180,Align::right);
  else if(n==8||n==9||n==13){bool on=n==13?reducedMotion:state["settings"].value(keys[n],false);d.rounded_rect({1233,y+17,51,27},14,Color::rgb(on?0x65bafa:0x34465d));d.circle(on?1270:1247,y+30,9,Color::rgb(on?0x102038:0xa7b6c8));}
  else text(">",1280,y+38,24,Color::rgb(0x90acc8),false,40,Align::right);
 }
 int n=items[std::clamp(settingsRow,0,(int)items.size()-1)];d.bordered_rect({1350,277,494,535},18,Color::rgb(0x101e30),1,Color::rgb(0x2a405a));label(keys[n],names[n],1380,326,25,Color::rgb(0xf0f6ff),434);
 std::string path;if(n>=3&&n<=6)path=game_path(n==3?"ps2":n==4?"switch":n==5?"ps5":"ps4");else if(n==10||n==11||n==12||n==14)path=game_path(n==10?"ps1":n==11?"xbox":n==12?"xbox360":"ps3");
 if(!path.empty()){label("currentfolder","Current folder",1380,388,20,Color::rgb(0x92a9c4),420);auto lines=regular.wrap(path,21,426);for(int j=0;j<std::min(5,(int)lines.size());j++)text(lines[j],1380,435+j*32,21,Color::rgb(0xcce8ff),false,434);label("folderhint","X to choose a different folder",1380,734,20,Color::rgb(0xa7bbd3),434);}
 else{const char*k=n==0?"languagehint":n==13?"motionhint":n==8||n==9?"installhint":"downloadhint";const char*t=n==0?"Arabic and English, at any time":n==13?"Keep navigation calm and responsive":n==8||n==9?"Your files stay until installation is verified":"Connections adapt to the source server";label(k,t,1380,414,22,Color::rgb(0xa7bbd3),434);}
 label("localdatahint","Settings are stored on this console",1380,778,17,Color::rgb(0x7b97b6),434);
}
void draw_update_page(){
 auto u=update_state();auto r=u.value("release",Json::object());if(r.is_null())r=Json::object();auto phase=u.value("phase","unconfigured");
 d.bordered_rect({399,277,1445,592},20,Color::rgb(0x111f31),1,Color::rgb(0x2e4662));brand(439,322,1.7f);text("raff",520,381,56,Color::rgb(0xf3f8ff),true);text("1.1.1",1799,378,28,accent(),true,270,Align::right);
 label("developedby","Developed by Mohammed Al-Ruwaili",440,435,26,Color::rgb(0xb7cbe1),1300);text("X / Twitter  @MohamedFAlrwili",440,471,22,accent(),true,1300);
 std::string key=phase=="unconfigured"?"updatenorepo":phase=="available"?"updateavailable":phase=="downloading"?"updatedownloading":phase=="verifying"?"updateverifying":phase=="ready"?"updateready":phase=="current"?"updatecurrent":phase=="error"?"updateerror":"updatecheck";
 const char*english=phase=="unconfigured"?"Official repository will be linked before publication":phase=="available"?"A new version of Raff is available":phase=="downloading"?"Downloading the update":phase=="verifying"?"Verifying files before installation":phase=="ready"?"Update verified and ready to install":phase=="current"?"You have the latest version":phase=="error"?"Update could not be completed":"Check for a new version";
 label(key,english,440,523,31,Color::rgb(0xf2f7ff),1300);if(!r.empty())text("v"+r.value("version","")+"  /  "+bytes(r.value("size",0.0)),440,568,23,accent(),true,1250);
 if(phase=="downloading"||phase=="verifying"){double total=u.value("total",0.0),received=u.value(phase=="verifying"?"verified":"received",0.0);float p=total>0?std::clamp(float(received/total),0.f,1.f):0;d.rounded_rect({440,608,1315,7},4,Color::rgb(0x2d425b));if(p>0)d.rounded_rect({440,608,1315*p,7},4,accent());text(number(p*100,0)+"%",1755,658,23,accent(),true,180,Align::right);}
 else{d.bordered_rect({440,612,630,64},16,Color::rgb(settingsNav?0x24394f:0xeff7ff),2,Color::rgb(0xb9ddfa));button("cross",476,644,22,Color::rgb(settingsNav?0xeaf5ff:0x102135));label(phase=="ready"?"updateinstall":phase=="available"||phase=="error"&&!r.empty()?"updatedownload":"updatecheck",phase=="ready"?"Install and close Raff":phase=="available"||phase=="error"&&!r.empty()?"Download update":"Check for updates",509,652,23,Color::rgb(settingsNav?0xeaf5ff:0x102135),522);}
 label("updatepreserve","Your games, settings and downloads are kept",440,735,22,Color::rgb(0xa6bfda),1270);text(u.value("repository",""),440,788,19,Color::rgb(0x7898b9),false,1300);
 std::string err=updateError.empty()?u.value("error",""):updateError;if(!err.empty())text(err,440,839,19,Color::rgb(0xf5c1b7),false,1320);
}
void draw_update_overlay(){
 if(updateDialog){d.rounded_rect({0,0,1920,1080},0,Color::rgb(0x030912,.88f));d.bordered_rect({390,338,1140,363},24,Color::rgb(0x14243a),1,Color::rgb(0x426283));label("updateconfirm","Install update and close Raff?",960,416,35,Color::rgb(0xf0f7ff),1040,Align::center);label("updateclosehint","Pause downloads first. Reopen Raff when installation finishes.",960,491,24,Color::rgb(0xb1c8e1),1040,Align::center);hint("cross","updateinstall","Install and close",505,626,575);hint("circle","back","Back",1145,626,260);return;}
 if(now<updateNoticeUntil&&!download_notice_visible()&&tab!=8&&detail<0&&!searching){d.bordered_rect({1120,855,715,107},18,Color::rgb(0x163550),1,Color::rgb(0x609bc9));label("updateavailable","A new version of Raff is available",1150,899,23,Color::rgb(0xf2f8ff),651);label("updatenoticehint","Options > About & updates",1150,936,19,Color::rgb(0xb2d7f5),651);}
}
