// Native storefront. Included inside App so the established download manager,

// controller keyboard, artwork cache and renderer remain shared.

int section=0,tile=0,pageOffset=0,order=0,platformTile=0,detailTab=0,releaseTile=0,shotTile=0;

std::vector<std::string> discIds;int discTile=0;

int fileTile=0,fileOffset=0,setupTile=0,coreTile=0;

bool storeStarted=false,storeBusy=true,fileOpen=false,gallery=false,reducedMotion=false;

bool restoring=true,storeNav=false,storeWasReady=false;

std::string platformFilter="ps2",gameId,browserPath="/data",message,editTarget;

Json page=Json::object(),platformList=Json::array(),game=Json::object(),setup=Json::object(),filePage=Json::object(),firmwareInfo=Json::object();

double messageUntil=0,nextScanPoll=0,nextMediaPoll=0;

std::string tr(const char*arabic,const char*english){return ar?arabic:english;}

std::string stringValue(const Json&j,const std::string&key,const std::string&fallback=""){auto it=j.find(key);if(it==j.end()||it->is_null())return fallback;return it->is_string()?it->get<std::string>():it->is_number()?it->dump():fallback;}

void say(const std::string&value){message=value;messageUntil=now+8;}

void ui(const std::string&s,float x,float y,float size=26,float width=1200,Color color=Color::rgb(0xe9edf7),bool strong=false){text(s,ar?x+width:x,y,size,color,strong,width,ar?Align::right:Align::left);}

void paragraph(const std::string&s,float x,float y,float width,int lines=4,float size=26,Color color=Color::rgb(0xb5bdd0)){auto wrapped=regular.wrap(s,size,width);for(int n=0;n<std::min(lines,(int)wrapped.size());n++)ui(wrapped[n],x,y+n*(size+12),size,width,color);}

void storeHint(const char*symbol,const std::string&s,float x,float width=300){button(symbol,x,1030,25);text(s,x+(strlen(symbol)>4?45:30),1038,22,Color::rgb(0xc4cbdd),false,width);}

Json cards(){return page.value("items",Json::array());}

std::string platformName(const std::string&id){for(const auto&p:platformList)if(p.value("id","")==id)return p.value("name",id);return id;}

void remember(){raff::command("/library/view",{{"value",{{"section",section},{"platform",platformFilter},{"query",query},{"offset",pageOffset},{"tile",tile},{"order",order},{"reducedMotion",reducedMotion}}}});}

void browse(bool reset=true){if(reset){pageOffset=0;tile=0;}storeBusy=true;const char*sorts[]={"az","rating","popular","year","recent"};if(section==2)raff::request("browse","/library/local",{{"offset",pageOffset}});else raff::request("browse","/library/browse",{{"query",query},{"platform",platformFilter},{"offset",pageOffset},{"limit",8},{"sort",sorts[order]},{"scope",section==3?"favorites":"discover"}});remember();}

void switchSection(int n){section=(n+6)%6;gameId.clear();fileOpen=false;gallery=false;storeNav=false;tile=pageOffset=0;tab=section==4?4:section==5?7:0;if(section==3)platformFilter.clear();if(section!=1&&section<4)browse();if(section==5)raff::request("setup","/library/state");remember();}

void openGame(const std::string&id){gameId=id;game=Json::object();detailTab=releaseTile=coreTile=discTile=shotTile=0;discIds.clear();gallery=false;raff::request("detail","/library/detail",{{"id",id}});remember();}

void browseFiles(const std::string&path,int offset=0){browserPath=path;fileOffset=offset;fileTile=0;fileOpen=true;filePage=Json::object();raff::request("files","/library/files",{{"path",path},{"offset",offset}});}

void storeTick(){

 bool serviceReady=state.value("ready",false);
 if(serviceReady&&!storeWasReady){storeStarted=false;storeBusy=true;}
 storeWasReady=serviceReady;
 if(!storeStarted&&serviceReady){storeStarted=true;raff::request("restore","/library/view");raff::request("platforms","/library/platforms");raff::request("setup","/library/state");}

 Json r;

 if(raff::take_result("restore",r)){restoring=false;if(!r.contains("error")){section=std::clamp(r.value("section",0),0,5);platformFilter=r.value("platform",std::string("ps2"));query=r.value("query",std::string());pageOffset=std::max(0,r.value("offset",0));tile=std::clamp(r.value("tile",0),0,7);order=std::clamp(r.value("order",0),0,4);reducedMotion=r.value("reducedMotion",false);}tab=section==4?4:section==5?7:0;browse(false);}

 if(raff::take_result("browse",r)){storeBusy=false;if(r.contains("error")){say(stringValue(r,"error")=="offline"?tr("جارٍ استعادة الاتصال بالمكتبة…","Reconnecting to your library…"):stringValue(r,"error"));}else{page=std::move(r);tile=std::clamp(tile,0,std::max(0,(int)cards().size()-1));}}

 if(raff::take_result("platforms",r)&&r.contains("items"))platformList=r["items"];

 if(raff::take_result("detail",r)){if(r.contains("error"))say(stringValue(r,"error"));else{game=std::move(r);raff::request("media","/library/media",{{"id",gameId}});}}

 if(raff::take_result("media",r)&&!r.contains("error")&&r.value("id","")==gameId){game["media"]=r;if(r.contains("cover"))game["cover"]=r["cover"];}

 if(raff::take_result("setup",r)&&!r.contains("error"))setup=std::move(r);

 if(raff::take_result("firmware",r))firmwareInfo=std::move(r);

 if(raff::take_result("files",r)){filePage=std::move(r);if(filePage.contains("error"))say(stringValue(filePage,"error"));}

 if(raff::take_result("action",r)){if(r.contains("error"))say(stringValue(r,"error"));else{say(r.contains("path")?stringValue(r,"path"):tr("تم حفظ التغيير","Saved"));if(!gameId.empty())raff::request("detail","/library/detail",{{"id",gameId},{"offset",game.value("releaseOffset",0)}});else if(section==2)browse(false);raff::request("setup","/library/state");}}

 if(serviceReady&&now>nextMediaPoll){nextMediaPoll=now+3;if(!gameId.empty())raff::request("media","/library/media",{{"id",gameId}});else if(section==0||section==3){Json ids=Json::array();for(auto&g:cards())ids.push_back(g["id"]);if(!ids.empty())raff::request("grid-art","/library/media",{{"ids",ids}});}}

 if(raff::take_result("grid-art",r)&&r.contains("items")){for(auto&art:r["items"])if(art.contains("cover"))for(auto&g:page["items"])if(g["id"]==art["id"])g["cover"]=art["cover"];}

 if(serviceReady&&section==5&&now>nextScanPoll){nextScanPoll=now+3;raff::request("setup","/library/state");}

}

void storeHeader(){

 d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x111725),Color::rgb(0x090c13));

 d.rounded_rect({0,0,1920,118},0,Color::rgb(0x111521));d.line(64,117,1856,117,1,Color::rgb(0x2b3244));

 text(ar?"رفّ":"RAFF",ar?1825:88,76,48,Color::rgb(0xf0f1ff),true,170,ar?Align::right:Align::left);

 const char*an[]={"اكتشف","المنصات","مكتبتي","المفضلة","التحميلات","الإعدادات"};

 const char*en[]={"Discover","Platforms","My library","Favorites","Downloads","Settings"};

 for(int n=0;n<6;n++){float x=ar?1462-n*211:285+n*211;bool active=section==n;d.rounded_rect({x,30,198,62},15,Color::rgb(active?(storeNav?0x6258b2:0x302b53):0x111521));text(ar?an[n]:en[n],x+99,70,26,Color::rgb(active?0xffffff:0xa9b0c3),active,184,Align::center);if(active)d.rounded_rect({x+77,103,44,3},2,Color::rgb(0xa398ff));}

}

void storeFooter(){

 d.rounded_rect({0,990,1920,90},0,Color::rgb(0x101521));d.line(64,990,1856,990,1,Color::rgb(0x2b3244));

 if(!fileOpen&&!gameId.empty()){storeHint("cross",detailTab==1?tr("اختيار النسخة","Choose version"):tr("فتح / تصدير","Open / export"),82);storeHint("square",tr("المفضلة","Favorite"),467);storeHint("triangle",tr("استيراد ملف","Import file"),820);storeHint("L1/R1",tr("تفاصيل اللعبة","Detail sections"),1160);storeHint("circle",tr("رجوع","Back"),1640,180);}

 else if(fileOpen){storeHint("cross",tr("فتح / استيراد","Open / import"),82);storeHint("triangle",tr("فحص هذا المجلد","Scan folder"),470);storeHint("L2/R2",tr("صفحات الملفات","File pages"),1030);storeHint("circle",tr("المجلد السابق","Parent folder"),1530);}

 else if(section==4){storeHint("cross",tr("إيقاف / استئناف","Pause / resume"),82);storeHint("square",tr("إلغاء التحميل","Cancel download"),580);storeHint("triangle",tr("رفع الأولوية","Raise priority"),1090);storeHint("L1/R1",tr("الأقسام","Sections"),1570);}

 else{storeHint("cross",tr("اختيار","Select"),82,170);storeHint("triangle",section==2?tr("استيراد","Import"):tr("بحث","Search"),362,180);storeHint("square",tr("الترتيب","Sort"),652,200);storeHint("L2/R2",tr("الصفحات","Pages"),1000,230);storeHint("L1/R1",tr("الأقسام","Sections"),1340,210);storeHint("circle",tr("رجوع","Back"),1690,140);}

}

void storeGrid(){

 if(section==2){ui(tr("ألعابك على الجهاز","Your games on this device"),76,175,38,1755,Color::rgb(0xffffff),true);ui(tr("استيراد ملفاتك أو فحص مجلد وربطه بـ RetroArch","Import files or scan a folder for RetroArch"),76,215,24,1755,Color::rgb(0x99a6bf));auto rows=cards();int first=std::max(0,tile-7);for(int n=first;n<std::min(first+8,(int)rows.size());n++){auto&r=rows[n];float y=258+(n-first)*83;d.bordered_rect({76,y,1768,72},15,Color::rgb(n==tile?0x302c51:0x192031),n==tile?2:0,Color::rgb(0xa69cff));text(stringValue(r.value("metadata",Json::object()),"title",stringValue(r,"path")),108,y+31,25,Color::rgb(0xf5f5ff),true,1600);text(stringValue(r,"path"),108,y+58,18,Color::rgb(0x9faec7),false,1600);}if(rows.empty())paragraph(tr("مكتبتك جاهزة لإضافة ألعابك. اضغط مثلث لاستيراد ملف، أو افحص مجلدًا من المنصات.","Your library is ready. Press Triangle to import a file, or scan a folder from Platforms."),360,420,1200,3,32);return;}

 const char*sortEn[]={"Title","Top rated","Popularity · source data","Release year","Recently cataloged"};const char*sortAr[]={"الاسم","الأعلى تقييمًا","الرواج · بيانات المصدر","سنة الإصدار","الأحدث في الفهرس"};

 ui(section==3?tr("ألعاب تستحق الرجوع لها","Saved for later"):tr("لعبتك التالية تبدأ هنا","Find your next game"),76,177,38,1755,Color::rgb(0xffffff),true);

 std::string context=platformFilter.empty()?tr("كل المنصات","All platforms"):platformName(platformFilter);context+="  /  "+number(page.value("total",0.0),0)+tr(" عنوان"," titles");if(!query.empty())context+="  /  "+query;

 ui(context,76,218,24,1755,Color::rgb(0x9fabc4));text(ar?sortAr[order]:sortEn[order],88,260,20,Color::rgb(0xc3bbf4),false,800);text(std::to_string(pageOffset+1)+" — "+std::to_string(pageOffset+(int)cards().size()),1830,260,20,Color::rgb(0x8f9eb9),false,250,Align::right);

 auto rows=cards();for(int n=0;n<std::min(8,(int)rows.size());n++){auto&g=rows[n];int col=ar?3-n%4:n%4;float x=76+col*448,y=284+(n/4)*342;bool focus=n==tile&&!storeNav;d.bordered_rect({x,y,422,321},20,Color::rgb(focus?0x242941:0x161c29),focus?3:1,Color::rgb(focus?0xaa9cf9:0x2b3244));poster(g,{x+14,y+13,164,250},13);auto lines=bold.wrap(g.value("title",""),24,202);for(int l=0;l<std::min(4,(int)lines.size());l++)text(lines[l],x+199,y+52+l*31,24,Color::rgb(0xf0f2fa),true,202);text(platformName(g.value("platform","")),x+199,y+207,18,Color::rgb(0xb6acd9),false,202);text(stringValue(g,"year",tr("غير مجرّبة","Untested")),x+199,y+242,20,Color::rgb(0x8fa1bb),false,202);text(g.value("favorite",false)?tr("في المفضلة","Favorite"):tr("عرض التفاصيل","View details"),x+24,y+297,20,Color::rgb(focus?0xd3caff:0x8997b0),false,365);}

 if(storeBusy){d.rounded_rect({800,472,320,90},22,Color::rgb(0x292d48));ui(tr("جاري البحث…","Loading…"),820,529,28,280);}

 else if(rows.empty())paragraph(tr("لا توجد نتائج. جرّب كلمة أقصر أو منصة أخرى.","No results. Try a shorter search or another platform."),380,470,1160,3,32);

}

void storePlatforms(){

 ui(tr("عالم ألعابك، منصة بمنصة","Explore every platform"),76,180,40,1768,Color::rgb(0xffffff),true);ui(tr("عدد العناوين من المصدر · توفر النواة على جهازك يظهر مستقلًا","Source catalog counts · core availability is checked on your target"),76,225,24,1768,Color::rgb(0x9fabc4));

 int first=(platformTile/12)*12;for(int n=first;n<std::min(first+12,(int)platformList.size());n++){auto&p=platformList[n];int c=(n-first)%3;if(ar)c=2-c;float x=76+c*601,y=273+((n-first)/3)*163;bool focus=n==platformTile;d.bordered_rect({x,y,567,139},18,Color::rgb(focus?0x302b51:0x192132),focus?2:1,Color::rgb(focus?0xa79af3:0x2a3244));text(p.value("name",""),x+25,y+44,25,Color::rgb(0xf4f5fb),true,515);std::string counts=stringValue(p,"game_count","0")+tr(" عنوان"," titles")+"   /   "+stringValue(p,"localCount","0")+tr(" محلي"," local");text(counts,x+25,y+80,22,Color::rgb(0xa9b4c9),false,515);std::string status=p.value("kind","")=="standalone"?tr("محاكي أو نظام مستقل","Standalone integration"):p.value("availableCores",Json::array()).empty()?tr("النواة غير مثبتة على الجهاز","Core not installed on target"):tr("النواة مثبتة · توافق الألعاب غير مجرّب","Core installed · games untested");text(status,x+25,y+114,19,Color::rgb(0xafa0df),false,515);}

}

Json currentCore(){auto ids=game.value("compatibility",Json::object()).value("coreAvailability",Json::array());if(ids.empty())return Json();return ids[std::clamp(coreTile,0,(int)ids.size()-1)];}

void storeDetails(){

 if(game.empty()){ui(tr("جاري تحميل التفاصيل…","Loading game details…"),300,420,36,1320);return;}

 auto metadata=game.value("metadata",Json::object()),legacy=metadata.value("legacy",Json::object());

 if(gallery){auto media=game.value("media",Json::object());auto shots=media.value("screenshots",Json::array());d.rounded_rect({0,118,1920,872},0,Color::rgb(0x070a10));if(!shots.empty()){Json imageGame={{"cover",shots[std::clamp(shotTile,0,(int)shots.size()-1)].value("key","")},{"title",game.value("title","")},{"platform",""}};poster(imageGame,{200,154,1520,748},12);ui(tr("لقطة لعب · Libretro · دائرة للرجوع","Gameplay · Libretro · Circle to return"),300,963,24,1320);}return;}

 poster(game,{78,156,290,405},22);text(game.value("title",""),412,202,40,Color::rgb(0xffffff),true,1420);text(game.value("platform",Json::object()).is_object()?game["platform"].value("name",""):stringValue(game,"platform"),414,250,25,Color::rgb(0xb6a5ea),false,1390);

 const char*aa[]={"عن اللعبة","النسخ والتحميل","التوافق والإعداد","لقطات اللعب","إدارة الأقراص"};const char*ee[]={"Overview","Versions & downloads","Compatibility & setup","Gameplay","Disc manager"};static_assert(sizeof(aa)==sizeof(ee),"Detail labels must match");for(int n=0;n<5;n++){float x=410+n*285;d.rounded_rect({x,278,271,53},12,Color::rgb(detailTab==n?0x39305e:0x1b2234));text(ar?aa[n]:ee[n],x+135,313,21,Color::rgb(0xe4e1f1),detailTab==n,258,Align::center);}

 text(game.value("favorite",false)?tr("محفوظة في المفضلة","In your favorites"):tr("مربع: أضف للمفضلة","Square: favorite"),90,603,23,Color::rgb(0xc6b7f2),false,286);

 text(tr("مثلث: استيراد ملف","Triangle: import file"),90,650,21,Color::rgb(0xa0b2ca),false,280);

 auto installs=game.value("installations",Json::array());text(installs.empty()?tr("غير مضافة محليًا","Not imported locally"):tr("مضافة إلى مكتبتك","In your library"),90,698,21,Color::rgb(0x9dcfbb),false,280);

 if(detailTab==0){std::string description=stringValue(metadata,ar?"descriptionAr":"description",tr("لم يتوفر وصف موثّق لهذه اللعبة بعد.","A sourced description is not available yet."));paragraph(description,416,395,1400,5,28);std::string developer=stringValue(metadata,"developer",stringValue(legacy,"developer",tr("غير متوفر","Unavailable")));std::string publisher=stringValue(metadata,"publisher",stringValue(legacy,"publisher",tr("غير متوفر","Unavailable")));ui(tr("المطور: ","Developer: ")+developer,416,625,24,1390);ui(tr("الناشر: ","Publisher: ")+publisher,416,669,24,1390);ui(tr("التصنيف: ","Genre: ")+stringValue(metadata,"genre",stringValue(legacy,"genre",tr("غير متوفر","Unavailable"))),416,713,24,1390);ui(tr("الإصدار: ","Release: ")+stringValue(metadata,"year",stringValue(legacy,"date",tr("غير متوفر","Unavailable"))),416,757,24,1390);auto critic=metadata.value("critic",Json());auto users=metadata.value("userRating",Json());std::string rating=tr("النقاد: غير مقيّمة","Critics: not rated");if(critic.is_object())rating=stringValue(critic,"score")+" / 100 · "+stringValue(critic,"source")+" · "+(critic.contains("count")&&!critic["count"].is_null()?stringValue(critic,"count")+tr(" مراجعة"," reviews"):tr("عدد المراجعات غير متوفر","Review count unavailable"));ui(rating,416,807,24,1390,Color::rgb(0xbacfa0));std::string userText=tr("المستخدمون: غير مقيّمة","Users: not rated");if(users.is_object())userText=stringValue(users,"score")+" / 5 · PS Store · "+(users.contains("count")&&!users["count"].is_null()?stringValue(users,"count")+tr(" تقييم"," ratings"):tr("عدد التقييمات غير متوفر","Review count unavailable"));ui(userText,416,845,24,1390,Color::rgb(0xbacfa0));ui(tr("دعم العربية: الواجهة / الترجمة / الصوت — غير موثّق لكل نسخة","Arabic: interface / subtitles / audio — not verified per release"),416,876,23,1390,Color::rgb(0x98a7c1));}

 if(detailTab==1){auto releases=game.value("releases",Json::array());int first=std::max(0,releaseTile-5);for(int n=first;n<std::min(first+6,(int)releases.size());n++){auto&r=releases[n];float y=357+(n-first)*86;d.bordered_rect({414,y,1415,77},12,Color::rgb(n==releaseTile?0x34314f:0x192031),n==releaseTile?2:0,Color::rgb(0xa89ce5));text(stringValue(r,"source_key"),435,y+30,23,Color::rgb(0xf1f2f8),true,1365);auto assets=r.value("assets",Json::array());std::string summary;for(auto&a:assets){summary=stringValue(a,"format")+"  ·  "+(a.contains("size")&&a["size"].is_number()?bytes(a["size"].get<double>()):tr("الحجم غير متوفر","Size unavailable"));break;}text(summary,435,y+59,20,Color::rgb(0xa6b3ce),false,1320);}ui(tr("اختر النسخة. تتاح الروابط فقط للمستودعات المصرّح لك باستخدامها.","Select a release. Downloads require an authorized repository."),414,916,22,1415,Color::rgb(0xb7a9cc));text(std::to_string(game.value("releaseOffset",0)+1)+" / "+std::to_string(game.value("releaseTotal",0)),90,756,22,Color::rgb(0x95a4be),false,280);}

 if(detailTab==2){auto comp=game.value("compatibility",Json::object());ui(comp.value("status","")=="unsupported"?tr("النواة غير مدعومة على هذا الجهاز","Core unsupported on this target"):tr("التوافق: غير مجرّبة","Compatibility: untested"),416,392,30,1410,Color::rgb(0xd2c3f2),true);bool available=!comp.value("coreAvailability",Json::array()).empty();ui(available?tr("نواة متوفرة على هذا السوني","A core is installed on this PS5"):tr("لم تُكتشف نواة متوافقة على هذا الجهاز","No matching core detected on this target"),416,446,26,1410);if(available){text(currentCore().get<std::string>(),438,497,26,Color::rgb(0xb5c9e8),true,1300);ui(tr("أعلى / أسفل لتغيير النواة · X لفحص ملفات النظام","Up / Down: core override · X: check firmware"),416,539,22,1410);}ui(tr("تشغيل محلي ناجح: لم يُسجّل · بدء العملية لا يثبت توافق اللعبة","Successful local gameplay: not recorded. Starting a process is not verification."),416,593,23,1410,Color::rgb(0x9caac1));if(!firmwareInfo.empty()){auto files=firmwareInfo.value("files",Json::array());int n=0;for(auto&f:files){if(n>=5)break;text((f.value("present",false)?"[+] ":"[ ] ")+stringValue(f,"path")+"  "+(f.value("optional",false)?tr("اختياري","optional"):tr("مطلوب","required"))+"  "+stringValue(f,"hashStatus"),438,650+n++*37,22,Color::rgb(f.value("present",false)?0xaacfb7:0xdfb3a3),false,1320);}}if(!installs.empty()){auto dep=installs[0].value("metadata",Json::object()).value("dependencies",Json::object());if(!dep.empty())ui(tr("اعتماديات اللعبة: ","Game dependencies: ")+stringValue(dep,"status"),416,851,23,1410,Color::rgb(0xb2c1da));}ui(tr("RetroArch: تصدير قائمة ثم فتحها داخل المحاكي. لا يُشغّل رفّ اللعبة تلقائيًا.","RetroArch: export a playlist and open it in the emulator."),416,899,24,1410,Color::rgb(0xb2c1da));}

 if(detailTab==3){auto media=game.value("media",Json::object());auto shots=media.value("screenshots",Json::array());if(!shots.empty()){Json imageGame={{"cover",shots[std::clamp(shotTile,0,(int)shots.size()-1)].value("key","")},{"title",game.value("title","")},{"platform",""}};poster(imageGame,{485,366,1270,500},15);ui(tr("لقطة لعب · يمين/يسار للصور · X للعرض الكامل","Gameplay capture · Left/Right: images · X: fullscreen"),416,923,24,1410);}else paragraph(tr("لا توجد لقطة لعب موثّقة متاحة لهذه النسخة حاليًا. تبقى بقية التفاصيل متاحة.","No verified gameplay capture is currently available for this release."),475,493,1300,4,30);}

 if(detailTab==4){ui(tr("اختر الأقراص بترتيب التشغيل ثم اضغط مثلث لإنشاء M3U","Select discs in play order, then Triangle creates an M3U"),416,386,25,1410);auto rows=game.value("installations",Json::array());int first=std::max(0,discTile-5);for(int n=first;n<std::min(first+6,(int)rows.size());n++){auto&r=rows[n];float y=423+(n-first)*70;auto found=std::find(discIds.begin(),discIds.end(),r.value("id",""));std::string prefix=found==discIds.end()?"[ ] ":"["+std::to_string(found-discIds.begin()+1)+"] ";d.bordered_rect({414,y,1415,60},11,Color::rgb(n==discTile?0x393054:0x192031),n==discTile?2:0,Color::rgb(0xa99ce5));text(prefix+stringValue(r,"path"),436,y+39,23,Color::rgb(0xe0e9f7),false,1360);}}

}

void storeFiles(){d.rounded_rect({130,150,1660,810},25,Color::rgb(0x151d2b));text(browserPath,166,204,27,Color::rgb(0xd4dff0),true,1570);auto entries=filePage.value("entries",Json::array());int first=std::max(0,fileTile-8);for(int n=first;n<std::min(first+9,(int)entries.size());n++){auto&r=entries[n];float y=236+(n-first)*69;d.bordered_rect({158,y,1603,60},11,Color::rgb(n==fileTile?0x383257:0x1d2639),n==fileTile?2:0,Color::rgb(0xab9cf1));text((r.value("type","")=="directory"?"[ + ]  ":"[   ]  ")+r.value("name",""),180,y+39,25,Color::rgb(0xe4eaf6),false,1500);}ui(tr("X: فتح أو استيراد · مثلث: فحص المجلد · مربع: إغلاق","X: open/import · Triangle: scan folder · Square: close"),165,914,24,1590);}

Json targetConfiguration(){auto profiles=setup.value("profiles",Json::array());return profiles.empty()?Json::object():profiles[0].value("configuration",Json::object());}

void storeSettings(){

 ui(tr("مصممة على مقاسك","Make it yours"),76,181,38,1750,Color::rgb(0xffffff),true);auto c=targetConfiguration();auto s=state.value("settings",Json::object());

 const char*arNames[]={"لغة الواجهة","تقليل الحركة","تحميلات متزامنة","حد سرعة التحميل","التثبيت التلقائي لحزم PKG","مسار RetroArch","مسار النوى","مسار الألعاب","مسار BIOS","مسار الحفظ","إعادة اكتشاف النوى","فحص مجلد ألعاب","تصدير مكتبتك لـ RetroArch","حالة فحص المجلدات","فهرس الألعاب والتغطية","مسار ألعاب PS2","مسار ألعاب Switch","مسار ألعاب PS5","مسار ألعاب PS4","مصادري الحالية مصرح لي بتنزيلها"};

 const char*enNames[]={"Interface language","Reduced motion","Simultaneous downloads","Bandwidth limit","Install completed PKG","RetroArch executable","Core directory","Content directory","Firmware directory","Save directory","Detect installed cores","Scan content folder","Export RetroArch playlist","Folder scan status","Catalog coverage","PS2 game folder","Switch game folder","PS5 game folder","PS4 game folder","My existing repositories are authorized"};

 std::vector<std::string> values={ar?"العربية":"English",reducedMotion?tr("مفعّل","On"):tr("متوقف","Off"),std::to_string(s.value("parallel",2)),s.value("bandwidthKiB",0)==0?tr("بلا حد","Unlimited"):std::to_string(s.value("bandwidthKiB",0))+" KiB/s",s.value("autoInstall",false)?tr("مفعّل","On"):tr("متوقف","Off"),stringValue(c,"executable"),stringValue(c,"coreDirectory"),stringValue(c,"contentDirectory"),stringValue(c,"firmwareDirectory"),stringValue(c,"saveDirectory"),std::to_string(c.value("installedCores",Json::array()).size())+tr(" نواة مثبتة"," installed cores"),platformName(platformFilter),tr("يحافظ على القوائم الحالية","Preserves existing playlists"),"",tr("مصدر: Libretro · المزامنة موثّقة","Source: Libretro · recorded synchronization"),game_path("ps2"),game_path("switch"),game_path("ps5"),game_path("ps4"),s.value("authorizedLegacy",false)?tr("مفعّل بإقرار المستخدم","Enabled by user"):tr("غير مفعّل","Not enabled")};

 auto scans=setup.value("scans",Json::array());if(!scans.empty()){auto&j=scans.back();values[13]=j.value("state","")+" · "+std::to_string(j.value("visited",0))+" / "+std::to_string(j.value("imported",0));}

 int first=std::max(0,setupTile-7);for(int n=first;n<std::min(first+8,20);n++){float y=231+(n-first)*87;d.bordered_rect({76,y,1768,76},15,Color::rgb(n==setupTile?0x302b4d:0x192132),n==setupTile?2:0,Color::rgb(0x9e94e4));ui(ar?arNames[n]:enNames[n],101,y+32,25,1713,Color::rgb(0xf3f3fa),true);text(values[n],101,y+61,20,Color::rgb(0xa9b7d0),false,1713);}

}

bool storeInput(const hui::InputFrame&i){

 auto press=[&](Action a){return i.is_pressed(a);};int dx=i.nav==Direction::right?1:i.nav==Direction::left?-1:0,dy=i.nav==Direction::down?1:i.nav==Direction::up?-1:0;

 if(fileOpen){auto entries=filePage.value("entries",Json::array());fileTile=std::clamp(fileTile+dy,0,std::max(0,(int)entries.size()-1));if(press(Action::west)){fileOpen=false;return true;}if(press(Action::back)){if(browserPath=="/data"||browserPath=="/mnt")fileOpen=false;else browseFiles(browserPath.substr(0,browserPath.find_last_of('/')));return true;}if(press(Action::jump_next)&&filePage.contains("next")&&!filePage["next"].is_null())browseFiles(browserPath,filePage["next"].get<int>());if(press(Action::jump_prev)&&fileOffset>0)browseFiles(browserPath,std::max(0,fileOffset-256));std::string platform=!gameId.empty()?game.value("platform",Json::object()).value("id",platformFilter):platformFilter;if(press(Action::north)){if(platform.empty())say(tr("اختر منصة أولًا من قسم المنصات","Select a platform first"));else{raff::request("action","/library/scan",{{"path",browserPath},{"platform",platform}});fileOpen=false;}}if(press(Action::confirm)&&!entries.empty()){auto&r=entries[fileTile];if(r.value("type","")=="directory")browseFiles(r.value("path","/data"));else if(platform.empty())say(tr("اختر منصة أولًا","Select a platform first"));else{Json v={{"path",r["path"]},{"platform",platform}};if(!gameId.empty())v["gameId"]=gameId;auto core=currentCore();if(!core.is_null())v["core"]=core;raff::request("action","/library/import",v);fileOpen=false;}}return true;}

 if(!gameId.empty()){

  if(press(Action::back)){if(gallery){gallery=false;return true;}gameId.clear();firmwareInfo=Json::object();return true;}if(game.empty())return true;if(!gallery&&(press(Action::page_prev)||press(Action::page_next))){detailTab=(detailTab+(press(Action::page_next)?1:4))%5;return true;}

  if(press(Action::west)&&detailTab!=4){bool enabled=!game.value("favorite",false);game["favorite"]=enabled;raff::request("action","/library/favorite",{{"id",gameId},{"enabled",enabled}});}if(press(Action::north)&&detailTab!=4)browseFiles(game_path(game.value("platform",Json::object()).value("id",platformFilter)));

  if(detailTab==1){auto releases=game.value("releases",Json::array());releaseTile=std::clamp(releaseTile+dy,0,std::max(0,(int)releases.size()-1));int offset=game.value("releaseOffset",0);if(press(Action::jump_next)&&offset+(int)releases.size()<game.value("releaseTotal",0)){releaseTile=0;raff::request("detail","/library/detail",{{"id",gameId},{"offset",offset+60}});}if(press(Action::jump_prev)&&offset>0){releaseTile=0;raff::request("detail","/library/detail",{{"id",gameId},{"offset",std::max(0,offset-60)}});}if(press(Action::confirm)&&!releases.empty())raff::request("action","/library/download",{{"release",releases[releaseTile]["id"]}});}

  else if(detailTab==2){auto cores=game.value("compatibility",Json::object()).value("coreAvailability",Json::array());coreTile=std::clamp(coreTile+dy,0,std::max(0,(int)cores.size()-1));if(press(Action::confirm)&&!cores.empty())raff::request("firmware","/library/firmware",{{"core",currentCore()},{"region",game.value("releases",Json::array()).empty()?"":game["releases"][std::clamp(releaseTile,0,(int)game["releases"].size()-1)]["region"].dump()}});}

  else if(detailTab==3){auto shots=game.value("media",Json::object()).value("screenshots",Json::array());shotTile=std::clamp(shotTile+dx,0,std::max(0,(int)shots.size()-1));if(press(Action::confirm))gallery=!gallery;}

  else if(detailTab==4){auto rows=game.value("installations",Json::array());discTile=std::clamp(discTile+dy,0,std::max(0,(int)rows.size()-1));if(press(Action::confirm)&&!rows.empty()){auto id=rows[discTile].value("id","");auto found=std::find(discIds.begin(),discIds.end(),id);if(found==discIds.end())discIds.push_back(id);else discIds.erase(found);}if(press(Action::north))raff::request("action","/library/multidisc",{{"ids",discIds}});}

  else if(press(Action::confirm)){auto installs=game.value("installations",Json::array());if(installs.empty()){detailTab=1;}else{Json v={{"installationId",installs[0]["id"]}};auto core=currentCore();if(!core.is_null())v["core"]=core;raff::request("action","/library/export",v);}}

  return true;

 }

 if(press(Action::l3)){ar=!ar;raff::command("/settings",{{"language",ar?"ar":"en"}});}if(press(Action::page_prev)){switchSection(section-1);return true;}if(press(Action::page_next)){switchSection(section+1);return true;}if(press(Action::menu)){switchSection(5);return true;}

 if(press(Action::back)){if(!query.empty()){query.clear();browse();}else if(section!=0)switchSection(0);else if(!platformFilter.empty()){platformFilter.clear();browse();}else exitDialog=true;return true;}

 if(section==4){auto jobs=download_jobs();job=std::clamp(job+dy,0,std::max(0,(int)jobs.size()-1));if(jobs.empty())return true;auto&j=jobs[job];selectedJobId=j.value("id","");if(press(Action::jump_next)&&!j.value("error","").empty())raff::command("/action",{{"id",j["id"]},{"action","restart"}});if(press(Action::north))raff::command("/action",{{"id",j["id"]},{"action","priority"}});if(press(Action::confirm))raff::command("/action",{{"id",j["id"]},{"action",j.value("paused",false)||!j.value("error","").empty()?"resume":"pause"}});if(press(Action::west)){cancelJobId=j.value("id","");cancelDialog=true;}return true;}

 if(section==5){setupTile=std::clamp(setupTile+dy,0,19);if(press(Action::confirm)||dx){auto s=state.value("settings",Json::object()),c=targetConfiguration();int step=dx?dx:1;if(setupTile==0){ar=!ar;raff::command("/settings",{{"language",ar?"ar":"en"}});}else if(setupTile==1){reducedMotion=!reducedMotion;remember();}else if(setupTile==2)raff::command("/settings",{{"parallel",std::clamp(s.value("parallel",2)+step,1,4)}});else if(setupTile==3)raff::command("/settings",{{"bandwidthKiB",std::max(0,s.value("bandwidthKiB",0)+step*1024)}});else if(setupTile==4)raff::command("/settings",{{"autoInstall",!s.value("autoInstall",false)}});else if(setupTile>=5&&setupTile<=9){const char*keys[]={"executable","coreDirectory","contentDirectory","firmwareDirectory","saveDirectory"};editTarget=keys[setupTile-5];pathDraft=stringValue(c,editTarget);searching=pathEditing=true;keyboardRow=1;keyboardCol=0;}else if(setupTile==10){raff::request("action","/library/detect");raff::request("platforms","/library/platforms");}else if(setupTile==11)browseFiles(stringValue(c,"contentDirectory","/data"));else if(setupTile==12)raff::request("action","/library/export");else if(setupTile==13){auto scans=setup.value("scans",Json::array());if(!scans.empty()){auto j=scans.back();raff::request("action","/library/scan-action",{{"id",j["id"]},{"action",j.value("state","")=="running"?"pause":"resume"}});}}else if(setupTile==14){say(tr("الفهرس محفوظ محليًا. تفاصيل المصدر والتغطية مرفقة مع الإصدار.","The catalog is stored locally. Source coverage is included with the release."));}else if(setupTile==19){raff::command("/settings",{{"authorizedLegacy",!s.value("authorizedLegacy",false)}});}else if(setupTile>=15&&setupTile<=18){const char*p[]={"ps2","switch","ps5","ps4"};open_path(p[setupTile-15],p[setupTile-15]);}}if(press(Action::west)&&setupTile==13){auto scans=setup.value("scans",Json::array());if(!scans.empty())raff::request("action","/library/scan-action",{{"id",scans.back()["id"]},{"action","cancel"}});}return true;}

 if(section==1){platformTile=std::clamp(platformTile+(ar?-dx:dx)+dy*3,0,std::max(0,(int)platformList.size()-1));if(press(Action::jump_next))platformTile=std::min(platformTile+12,std::max(0,(int)platformList.size()-1));if(press(Action::jump_prev))platformTile=std::max(0,platformTile-12);if(press(Action::confirm)&&!platformList.empty()){platformFilter=platformList[platformTile].value("id","");switchSection(0);}if(press(Action::north)&&!platformList.empty()){platformFilter=platformList[platformTile].value("id","");browseFiles("/data");}return true;}

 if(press(Action::north)||press(Action::touch)){if(section==2)browseFiles("/data");else{searching=true;pathEditing=false;keyboardRow=1;keyboardCol=0;}}if(press(Action::west)&&section!=2){order=(order+1)%5;browse();}

 auto rows=cards();int step=section==2?dy:(ar?-dx:dx)+dy*4;tile=std::clamp(tile+step,0,std::max(0,(int)rows.size()-1));if(step)remember();int size=section==2?60:8;if(press(Action::jump_next)&&pageOffset+size<page.value("total",0)){pageOffset+=size;tile=0;browse(false);}if(press(Action::jump_prev)&&pageOffset){pageOffset=std::max(0,pageOffset-size);tile=0;browse(false);}if(press(Action::confirm)&&!rows.empty()){auto&r=rows[tile];if(section==2){if(r.contains("game_id")&&r["game_id"].is_string())openGame(r["game_id"].get<std::string>());else raff::request("action","/library/export",{{"installationId",r["id"]}});}else openGame(r.value("id",""));}return true;

}

void storeDraw(){storeHeader();if(!gameId.empty())storeDetails();else if(section==1)storePlatforms();else if(section==4)downloads();else if(section==5)storeSettings();else storeGrid();if(fileOpen)storeFiles();storeFooter();if(now<messageUntil){d.bordered_rect({275,870,1370,101},17,Color::rgb(0x332d4e),1,Color::rgb(0xafa3e7));paragraph(message,300,910,1320,2,22,Color::rgb(0xf1eaff));}}

