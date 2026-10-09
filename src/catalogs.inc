// Load only the visible platform. Favorites and installed games come from the disk index.
std::string loadedCatalog,specialStamp,specialSection;
Json specialItems=Json::array();
bool platformPicker=true,catalogPending=false;int platformCursor=0;
std::map<std::string,std::string> platformDefaults={{"nes","/data/homebrew/PPSA99169/content/NES"},{"snes","/data/homebrew/PPSA99169/content/SNES"},{"n64","/data/homebrew/PPSA99169/content/N64"},{"gb","/data/homebrew/PPSA99169/content/GB"},{"gbc","/data/homebrew/PPSA99169/content/GBC"},{"gba","/data/homebrew/PPSA99169/content/GBA"},{"nds","/data/homebrew/PPSA99169/content/NDS"},{"3ds","/data/homebrew/PPSA99169/content/3DS"},{"psp","/data/homebrew/PPSA99169/content/PSP"},{"gamecube","/data/homebrew/PPSA99169/content/GAMECUBE"},{"wii","/data/homebrew/PPSA99169/content/WII"},{"genesis","/data/homebrew/PPSA99169/content/GENESIS"},{"mastersystem","/data/homebrew/PPSA99169/content/MASTERSYSTEM"},{"gamegear","/data/homebrew/PPSA99169/content/GAMEGEAR"},{"saturn","/data/homebrew/PPSA99169/content/SATURN"},{"segacd","/data/homebrew/PPSA99169/content/SEGACD"},{"dreamcast","/data/homebrew/PPSA99247/games"},{"arcade","/data/homebrew/PPSA99169/content/ARCADE"}};
std::string platform_title(const std::string&p){for(int t:platformOrder)if(p==tabKeys[t])return tabNames[t];return p;}
void index_catalog(){cardIndex.clear();searchKeys.clear();metadataIndex.clear();heroIndex=previousHero=requestedHero=-1;shelves.rows.clear();for(int n=0;n<(int)catalog.size();n++){cardIndex[catalog[n].value("key","")]=n;searchKeys.push_back(search_key(catalog[n].value("title","")));}selected=0;gridOffset=0;}
void ensure_catalog(){
 if(platform_tab()){
  const std::string p=tabKeys[tab];if(loadedCatalog==p)return;loadedCatalog=p;specialStamp.clear();catalogPending=false;specialItems=Json::array();
  catalog=Json::array();catalog=load_json((std::string("/app0/assets/catalog-")+p+".json").c_str());if(!catalog.is_array())catalog=Json::array();index_catalog();log_heap(p.c_str());return;
 }
 if(tab!=6&&tab!=7)return;
 const std::string section=tab==6?"favorites":"local";std::string stamp=section;
 for(const auto&k:(tab==6?favorites:installedSet))stamp+='|'+k;
 if(stamp==specialStamp&&loadedCatalog==section)return;
 loadedCatalog=section;specialStamp=stamp;specialSection=section;specialItems=Json::array();catalogPending=true;
 catalog=Json::array();index_catalog();raff::request("catalog-selected","/catalog/selected",{{"section",section},{"offset",0}});
}
void accept_catalog(){Json r;if(!raff::take_result("catalog-selected",r))return;if(!catalogPending||r.value("section","")!=loadedCatalog)return;if(r.contains("error")){catalogPending=false;notify("offline");return;}
 for(auto&c:r.value("items",Json::array()))specialItems.push_back(c);
 if(r.contains("next")&&!r["next"].is_null()){raff::request("catalog-selected","/catalog/selected",{{"section",loadedCatalog},{"offset",r["next"]}});return;}
 catalogPending=false;catalog=std::move(specialItems);index_catalog();rebuild();
}
