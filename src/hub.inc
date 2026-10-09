// Native, controller-driven catalog and transfer UI. Network work stays off the render thread.
Json hubCatalog,hubItems=Json::array(),hubEntry=Json::object(),transferDetail=Json::object(),importFiles=Json::array();
int hubKind=0,hubCursor=0,hubRelease=0,sourceCursor=0,importCursor=0,importPlatform=0,fileCursor=0,fileOffset=0,transferSetting=0;
bool hubOpened=false,hubPS5Only=true,hubFavorites=false,hubRecent=false,transferPending=false;
std::string transferModal,transferMessage,consentAction,magnetDraft,transferRoute,transferTitle;Json consentBody;
std::set<int> selectedFiles;
const char* importPlatforms[10]={"files","ps1","ps2","ps3","ps4","ps5","switch","xbox","xbox360","files"};
void paragraph(const std::string&s,float x,float y,float width,int lines=4,float size=23,Color color=Color::rgb(0xb9c9dc)){
 auto rows=regular.wrap(s,size,width);for(int n=0;n<std::min(lines,(int)rows.size());n++)text(rows[n],x,y+n*(size+8),size,color,false,width);
}
std::string hub_labels(const Json&items){std::string out;for(const auto&v:items){if(!v.is_string())continue;std::string name=v.get<std::string>();if(name=="ps5")name="PlayStation 5";else if(name=="windows")name="Windows";else if(name=="linux")name="Linux";else if(name=="ps1")name="PS1";else if(name=="ps2")name="PS2";else if(name=="ps3")name="PS3";if(!out.empty())out+=" / ";out+=name;}return out;}
void init_hub(){hubCatalog=load_json("/app0/assets/hub.json");if(!hubCatalog.is_object())hubCatalog={{"entries",Json::array()},{"releases",Json::array()},{"sources",Json::array()}};filter_hub();}
void filter_hub(){hubItems=Json::array();const auto q=lower(query);const auto source=hubCatalog.value("entries",Json::array());
 for(auto&e:source){if(e.value("kind","")!=(hubKind==0?"emulator":"game"))continue;if(!q.empty()&&lower(e.value("title","")+" "+e.value("description","")).find(q)==std::string::npos)continue;
  if(hubPS5Only){auto hosts=e.value("hosts",Json::array());if(std::find(hosts.begin(),hosts.end(),Json("ps5"))==hosts.end())continue;}if(hubFavorites&&!favorites.count("hub:"+e.value("id","")))continue;hubItems.push_back(e);}
 std::stable_sort(hubItems.begin(),hubItems.end(),[&](const Json&a,const Json&b){return hubRecent?a.value("addedAt","")>b.value("addedAt",""):a.value("title","")<b.value("title","");});hubCursor=std::clamp(hubCursor,0,std::max(0,(int)hubItems.size()-1));
}
void transfer_request(const std::string&path,const Json&body,const std::string&title=""){if(transferPending)return;transferPending=true;transferRoute=path;transferTitle=title;transferMessage.clear();raff::request("transfer-ui",path,body);}
void imports_open(){transferModal="imports";importCursor=0;transfer_request("/transfers/imports",Json::object());}
void consent(const std::string&path,Json body){consentAction=path;consentBody=std::move(body);transferModal="consent";}
void inspect_import(const Json&body){if(body.contains("magnet")||body.value("path","").find(".torrent")==std::string::npos)consent("/transfers/inspect",body);else transfer_request("/transfers/inspect",body);}
void accept_transfer_result(const Json&r){transferPending=false;const auto route=transferRoute;transferRoute.clear();
 if(r.contains("error")&&!r.value("error","").empty()){transferMessage=r.value("error","");notify("error");return;}
 if((route=="/queue"||route=="/transfers/game"||route=="/transfers/release"||route=="/transfers/start")&&r.contains("id")&&r.value("state","")!="selecting"){
  download_added(r.value("id",""),r.value("title",transferTitle));transferModal.clear();transferMessage.clear();return;
 }
 if(r.contains("entries")){importFiles=r["entries"];return;}
 if(r.value("catalogGame",false)){transferDetail=r;transferModal="receipt";transferMessage.clear();return;}
 if(r.contains("files")&&r.contains("id")){const bool same=transferDetail.value("id","")==r.value("id","");transferDetail=r;if(!same){selectedFiles.clear();fileCursor=0;fileOffset=0;}for(auto&f:r["files"])if(f.value("selected",false))selectedFiles.insert(f.value("index",0));transferModal="files";return;}
 if(r.contains("settings"))state["transfers"]["settings"]=r["settings"];else if(r.contains("uploadKiB"))state["transfers"]["settings"]=r;
 transferMessage="Saved";
}
void update_hub(){Json r;if(raff::take_result("transfer-ui",r))accept_transfer_result(r);}
bool hub_input(const hui::InputFrame&i){auto press=[&](Action a){return i.is_pressed(a);};int dx=i.nav==Direction::right?1:i.nav==Direction::left?-1:0,dy=i.nav==Direction::down?1:i.nav==Direction::up?-1:0;
 if(!transferModal.empty()){
  if(press(Action::back)){if(!transferPending){transferModal.clear();transferMessage.clear();}return true;}if(transferPending)return true;
  if(transferModal=="consent"){if(press(Action::confirm)){consentBody["uploadAccepted"]=true;transfer_request(consentAction,consentBody);}return true;}
  if(transferModal=="imports"){
   importCursor=std::clamp(importCursor+dy,0,(int)importFiles.size()+2);if(importCursor==1&&dx)importPlatform=(importPlatform+dx+9)%9;
   if(press(Action::confirm)){if(importCursor==0){pathEditing=true;pathTarget="@magnet";pathDraft=magnetDraft.empty()?"magnet:?xt=urn:btih:":magnetDraft;pathUpper=false;searching=true;keyboardRow=1;keyboardCol=0;}else if(importCursor==1)importPlatform=(importPlatform+1)%9;else if(importCursor==2){transferModal="history";importCursor=0;}else inspect_import({{"path",importFiles[importCursor-3].value("path","")},{"platform",importPlatforms[importPlatform]}});}
   if(press(Action::west))transfer_request("/transfers/imports",Json::object());return true;
  }
  if(transferModal=="history"){Json jobs=Json::array();for(auto&j:state.value("jobs",Json::array()))if(j.contains("kind")&&j.value("phase","")=="complete")jobs.push_back(j);importCursor=std::clamp(importCursor+dy,0,std::max(0,(int)jobs.size()-1));if(press(Action::confirm)&&!jobs.empty())transfer_request("/transfers/detail",{{"id",jobs[importCursor]["id"]}});return true;}
  if(transferModal=="files"){
   auto files=transferDetail.value("files",Json::array());fileCursor=std::clamp(fileCursor+dy,0,std::max(0,(int)files.size()-1));const bool choosing=transferDetail.value("state","")=="selecting";
   if(press(Action::confirm)&&choosing&&!files.empty()){int n=files[fileCursor].value("index",0);if(!files[fileCursor].value("padding",false)){if(selectedFiles.count(n))selectedFiles.erase(n);else selectedFiles.insert(n);}}
   int step=press(Action::jump_next)?128:press(Action::jump_prev)?-128:0;if(step){fileOffset=std::clamp(fileOffset+step,0,std::max(0,(transferDetail.value("fileCount",0)-1)/128*128));fileCursor=0;transfer_request("/transfers/detail",{{"id",transferDetail["id"]},{"offset",fileOffset}});}
   if(press(Action::north)&&choosing){if(selectedFiles.empty())transferMessage="Select at least one file";else consent("/transfers/start",{{"id",transferDetail["id"]},{"selected",selectedFiles}});}return true;
  }
  if(transferModal=="limits"){
   transferSetting=std::clamp(transferSetting+dy,0,4);if(dx||press(Action::confirm)){auto settings=state.value("transfers",Json::object()).value("settings",Json::object());const char*names[]={"downloadKiB","uploadKiB","seedMinutes","parallel"};
    if(transferSetting==4){transfer_request("/transfers/clear",Json::object());return true;}std::vector<int>values=transferSetting==0?std::vector<int>{0,512,1024,4096,8192,16384,32768}:transferSetting==1?std::vector<int>{16,32,64,128,256,512,1024}:transferSetting==2?std::vector<int>{0,5,15,30,60,120}:std::vector<int>{1,2,3,4};int value=settings.value(names[transferSetting],transferSetting==1?64:transferSetting==3?2:0),at=0;for(int n=0;n<(int)values.size();n++)if(values[n]==value)at=n;at=(at+(dx?dx:1)+values.size())%values.size();transfer_request("/transfers/settings",{{names[transferSetting],values[at]}});
   }return true;
  }return true;
 }
 if(tab==5&&!headerFocus&&!navFocus){if(press(Action::r3)){imports_open();return true;}auto jobs=download_jobs();if(press(Action::confirm)&&!jobs.empty()&&jobs[job].contains("kind")){auto j=jobs[job];if(j.value("state","")=="selecting")transfer_request("/transfers/detail",{{"id",j["id"]}});else transfer_request("/transfers/action",{{"id",j["id"]},{"action",j.value("state","")=="downloading"||j.value("state","")=="metadata"||j.value("state","")=="seeding"||j.value("state","")=="queued"||j.value("state","")=="metadata-queued"?"pause":"resume"}});return true;}return false;}
 if(tab!=12||headerFocus)return false;
 if(press(Action::l3)){ar=!ar;dispatch("/settings",{{"language",ar?"ar":"en"}});return true;}
 if(hubOpened){if(press(Action::back)){hubOpened=false;return true;}
  Json releases=Json::array();for(auto&r:hubCatalog.value("releases",Json::array()))if(r.value("entryId","")==hubEntry.value("id",""))releases.push_back(r);hubRelease=std::clamp(hubRelease+dy,0,std::max(0,(int)releases.size()-1));
  if(press(Action::confirm)&&!releases.empty()){auto&r=releases[hubRelease];if(r.contains("magnet"))consent("/transfers/release",{{"id",r["id"]}});else{transferModal="receipt";transfer_request("/transfers/release",{{"id",r["id"]}});}}return true;
 }
 if(press(Action::menu))return false;
 if(press(Action::back)||dy<0&&hubCursor<2){headerFocus=true;headerCursor=6;return true;}
 if(press(Action::jump_next)||press(Action::page_next)){hubKind=(hubKind+1)%2;hubCursor=0;filter_hub();return true;}
 if(press(Action::page_prev)){hubKind=(hubKind+1)%2;hubCursor=0;filter_hub();return true;}
 if(press(Action::jump_prev)){hubPS5Only=!hubPS5Only;hubCursor=0;filter_hub();return true;}
 if(press(Action::r3)){hubRecent=!hubRecent;filter_hub();return true;}
 if(press(Action::touch)){hubFavorites=!hubFavorites;hubCursor=0;filter_hub();return true;}
 if(press(Action::north)){searching=true;pathEditing=true;pathTarget="@hub-query";pathDraft=query;keyboardRow=1;keyboardCol=0;return true;}
 hubCursor=std::clamp(hubCursor+dx+dy*2,0,std::max(0,(int)hubItems.size()-1));
 if(!hubItems.empty()&&press(Action::west)){auto key="hub:"+hubItems[hubCursor].value("id","");if(favorites.count(key))favorites.erase(key);else favorites.insert(key);dispatch("/settings",{{"favorites",favorites}});filter_hub();}
 if(press(Action::confirm)&&!hubItems.empty()){hubEntry=hubItems[hubCursor];hubOpened=true;hubRelease=sourceCursor=0;}return true;
}
void hub_draw(){
 label("hub","Emulators & homebrew",76,172,38);const char*names[]={"Emulators","Free games"},*keys[]={"emulators","freegames"};
 for(int n=0;n<2;n++){float x=76+n*266;d.bordered_rect({x,208,250,49},24,Color::rgb(n==hubKind?0xe6f2ff:0x172737),0,accent());label(keys[n],names[n],x+125,242,23,Color::rgb(n==hubKind?0x13283b:0xd3dfec),226,Align::center);}
 text((hubPS5Only?"PS5 HOST":"ALL HOSTS")+std::string(hubFavorites?" / FAVORITES":"")+" / "+(hubRecent?"RECENT":"A-Z"),1825,239,18,accent(),true,900,Align::right);
 if(hubItems.empty())label("noresults","No matching entries",960,512,34,Color::rgb(0xa9bace),1500,Align::center);
 int first=std::max(0,hubCursor/2-2)*2;for(int n=first;n<std::min(first+6,(int)hubItems.size());n++){auto&e=hubItems[n];float x=76+(n%2)*894,y=287+(n-first)/2*205;bool focus=n==hubCursor&&!headerFocus;
  d.bordered_rect({x,y,868,182},19,Color::rgb(focus?0x263e55:0x122030),focus?2:1,Color::rgb(focus?0xbce0ff:0x304356));std::string sys=e.value("systems",Json::array()).empty()?"ps2":e["systems"][0].get<std::string>();platform_icon(sys,x+24,y+29,54,accent());
  text(e.value("title",""),x+106,y+48,28,Color::rgb(0xf3f7ff),true,729);std::string hosts=hub_labels(e.value("hosts",Json::array()));text(hosts,x+106,y+82,17,accent(),true,720);paragraph(e.value("description",""),x+26,y+120,815,2,19);
 }text(std::to_string(hubItems.empty()?0:hubCursor+1)+" / "+std::to_string(hubItems.size()),1817,951,20,accent(),false,190,Align::right);
 if(!hubOpened)return;
 d.rounded_rect({52,103,1816,868},22,Color::rgb(0x101c2a));text(hubEntry.value("title",""),91,171,37,Color::rgb(0xf1f7ff),true,1700);
 const auto hosts=hub_labels(hubEntry.value("hosts",Json::array())),systems=hub_labels(hubEntry.value("systems",Json::array()));text("HOST: "+hosts+"    SYSTEM: "+systems,93,218,21,accent(),true,1700);paragraph(hubEntry.value("description",""),93,269,1660,2,24);
 text(hubEntry.value("version","")+"  /  "+hubEntry.value("license",""),93,349,23,Color::rgb(0xe1ca99),true,1710);paragraph(hubEntry.value("setup",""),93,397,1690,3,23);
 Json releases=Json::array();for(auto&r:hubCatalog.value("releases",Json::array()))if(r.value("entryId","")==hubEntry.value("id",""))releases.push_back(r);
 if(releases.empty())label("nobuild","Source reference only; no verified binary release",93,557,27,Color::rgb(0xe0bd9a),1690);
 for(int n=0;n<std::min(2,(int)releases.size());n++){auto&r=releases[n];float y=524+n*87;d.bordered_rect({90,y,1737,76},13,Color::rgb(n==hubRelease?0x294d6b:0x192d40),n==hubRelease?2:0,accent());text(r.value("filename",""),114,y+32,24,Color::rgb(0xf3f8ff),true,1240);text(bytes(r.value("size",0.0))+" / "+r.value("host",""),1807,y+32,21,accent(),true,425,Align::right);text("HTTPS / SHA-256  |  /data/raff/downloads/Raff-<id>",114,y+61,18,Color::rgb(0xc4d3e2),false,1660);}
 label("downloadonly","Download only. Install and launch are not verified for this build.",93,734,25,Color::rgb(0xe0bd9a),1700);
}
void transfer_draw(){if(transferModal.empty())return;d.rounded_rect({0,0,1920,1080},0,Color::rgb(0x03070e,.75f));d.bordered_rect({114,116,1692,848},24,Color::rgb(0x102033),1,Color::rgb(0x65819d));label("transfercenter","Transfer center",151,177,33);
 if(transferModal=="consent"){label("torrentnotice","Torrents upload pieces to other people while downloading",160,275,31,Color::rgb(0xe4d2a8),1550);label("torrentnotice2","Peers can see your IP. Continue only with files you are authorized to use.",160,334,25,Color::rgb(0xccd7e5),1550);auto s=state.value("transfers",Json::object()).value("settings",Json::object());text("UPLOAD: "+std::to_string(s.value("uploadKiB",64))+" KiB/s    SEED AFTER FINISH: "+std::to_string(s.value("seedMinutes",0))+" min",160,417,29,accent(),true,1540);label("seedzero","0 minutes stops seeding after completion; uploading during download remains enabled.",160,482,24,Color::rgb(0xa9bfd4),1540);hint("cross","agree","Agree and continue",205,838,760);hint("circle","back","Back",1235,838,300);}
 else if(transferModal=="imports"){label("importfolder","Put .torrent or .magnet files in /data/raff/imports",157,223,23,Color::rgb(0xa8c5e3),1550);const int first=std::max(0,importCursor-6);for(int n=first;n<std::min(first+7,(int)importFiles.size()+3);n++){float y=259+(n-first)*73;bool focus=n==importCursor;d.bordered_rect({151,y,1619,64},12,Color::rgb(focus?0x2b5273:0x1a3046),focus?2:0,accent());if(n==0)label("magnet","Enter magnet link",176,y+42,26,Color::rgb(0xedf5ff),1560);else if(n==1){label("destination","Game destination",176,y+42,25,Color::rgb(0xf0f5ff),750);text(importPlatforms[importPlatform],1735,y+42,25,accent(),true,630,Align::right);}else if(n==2)label("completedfiles","Completed files",176,y+42,25,Color::rgb(0xf0f5ff),1560);else text(importFiles[n-3].value("name",""),176,y+42,25,Color::rgb(0xf0f5ff),false,1560);}hint("cross","select","Select",198,861,390);hint("square","refresh","Refresh folder",761,861,480);hint("circle","back","Back",1440,861,260);}
 else if(transferModal=="history"){Json jobs=Json::array();for(auto&j:state.value("jobs",Json::array()))if(j.contains("kind")&&j.value("phase","")=="complete")jobs.push_back(j);const int first=std::max(0,importCursor-5);for(int n=first;n<std::min(first+6,(int)jobs.size());n++){float y=239+(n-first)*89;d.bordered_rect({153,y,1617,79},12,Color::rgb(n==importCursor?0x294e6c:0x1b3043),n==importCursor?2:0,accent());text(jobs[n].value("title",""),177,y+31,24,Color::rgb(0xf1f7ff),true,1550);text(jobs[n].value("destination",""),177,y+62,20,accent(),false,1550);}if(jobs.empty())label("noresults","No completed files yet",160,294,28);}
 else if(transferModal=="files"){text(transferDetail.value("title",""),155,232,27,Color::rgb(0xf1f6ff),true,1580);text(transferDetail.value("destination",""),155,269,20,accent(),false,1600);text(transferDetail.value("state","")+" / "+std::to_string(selectedFiles.size())+" selected / "+bytes(transferDetail.value("total",0.0)),155,305,21,Color::rgb(0xb8cbdc),true,1600);auto files=transferDetail.value("files",Json::array());int first=std::max(0,fileCursor-5);for(int n=first;n<std::min(first+6,(int)files.size());n++){auto&f=files[n];float y=329+(n-first)*67;bool focus=n==fileCursor,checked=selectedFiles.count(f.value("index",0));d.bordered_rect({152,y,1618,60},10,Color::rgb(focus?0x294f6c:0x1b3043),focus?2:0,accent());text(checked?"[X]":"[ ]",171,y+38,22,accent(),true,63);text(f.value("path",""),242,y+37,22,Color::rgb(0xf1f5fa),false,1278);text(bytes(f.value("length",0.0)),1745,y+38,19,Color::rgb(0xb4cce0),false,213,Align::right);}hint("cross","selectfiles","Select files",199,821,415);hint("triangle","startdownload","Start selected",730,821,450);hint("L2/R2","pages","File pages",1355,821,341);text(transferDetail.value("integrity","pending"),156,883,19,accent(),false,1450);}
 else if(transferModal=="receipt"&&transferDetail.value("catalogGame",false)){label("gamequeued","Game added to downloads",160,261,35,accent(),1550);text(transferDetail.value("title",""),160,325,30,Color::rgb(0xf0f6ff),true,1550);label("gameselected","Only this edition's files will download",160,383,25,Color::rgb(0xb9cede),1550);text(bytes(transferDetail.value("total",0.0))+" / "+std::to_string(transferDetail.value("fileCount",0))+" files",160,429,24,accent(),true,1550);label("destination","Game destination",160,501,24);text(transferDetail.value("gameDestination",""),160,544,24,accent(),false,1550);auto selected=transferDetail.value("files",Json::array());for(int n=0;n<std::min(4,(int)selected.size());n++){auto path=selected[n].value("path","");text(path.substr(path.find_last_of('/')+1),160,610+n*43,21,Color::rgb(0xc3d6e6),false,1540);}hint("circle","back","Back",230,849,420);}
 else if(transferModal=="limits"){const char*keys[]={"downloadlimit","uploadlimit","seedtime","parallel","clearhistory"},*names[]={"Download limit (KiB/s; 0 = unlimited)","Upload limit (KiB/s)","Seeding after completion (minutes)","Concurrent transfers","Clear completed history (keep files)"};auto s=state.value("transfers",Json::object()).value("settings",Json::object());const char*fields[]={"downloadKiB","uploadKiB","seedMinutes","parallel"};for(int n=0;n<5;n++){float y=244+n*105;d.bordered_rect({154,y,1612,88},14,Color::rgb(n==transferSetting?0x294e6b:0x1c3043),n==transferSetting?2:0,accent());label(keys[n],names[n],180,y+55,26,Color::rgb(0xf2f6ff),1280);if(n<4)text("< "+std::to_string(s.value(fields[n],n==1?64:n==3?2:0))+" >",1735,y+56,28,accent(),true,260,Align::right);}}
 if(transferPending){d.arc(1734,166,16,3,now*3,4,accent());label("wait","Working...",154,927,23,accent(),1200);}else if(!transferMessage.empty())paragraph(transferMessage,154,909,1590,2,20,Color::rgb(0xf3c7a6));
}
