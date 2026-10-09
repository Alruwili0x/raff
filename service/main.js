const queue=new RaffQueue.Queue({catalog:JSON.parse(nativeRead('catalog')),read:n=>nativeRead(n),write:(n,v)=>nativeWrite(n,v)});

queue.discovery=JSON.parse(nativeRead("cards"));
const sources=new RaffSources.Sources((q,p)=>nativeSQL(q,p));queue.sources=sources;

const library=new RaffLibrary.Library({sql:(q,p)=>nativeSQL(q,p),fs:(op,p,v)=>nativeFS(op,p,v),registry:JSON.parse(nativeRead('registry')),settings:queue.settings,saveSettings:()=>nativeWrite('settings',JSON.stringify(queue.settings))});

const media=new RaffMedia.Media(library,queue);
const psxs5=new RaffPSXS5.Sync((op,p,v)=>nativeFS(op,p,v),queue);
let hub=new RaffHub.Hub(JSON.parse(nativeRead('hub')));
const transfers=new RaffGameTransfers.GameTransfers({gameCatalog:JSON.parse(nativeRead('game-torrents')),gameQueue:queue,sql:(q,p)=>nativeSQL(q,p),fs:(op,p,v)=>nativeFS(op,p,v),client:queue.clientFactory(),catalog:hub.catalog,legacyActive:()=>queue.jobs.filter(j=>!j.paused&&!j.error&&!['done','canceled'].includes(j.phase)).length});
transfers.sources=sources;

let updateConfig={repository:''};try{updateConfig=JSON.parse(nativeRead('update-config')||'{}');}catch{}
const updater=new RaffUpdates.Updates({config:updateConfig,client:queue.clientFactory(),fs:nativeFS,
 read:nativeRead,write:nativeWrite,id:()=>Array.from(new Uint8Array(nativeRandom(8)),v=>v.toString(16).padStart(2,'0')).join(''),
 busy:async()=>{if(queue.jobs.some(j=>!['done','canceled'].includes(j.phase)&&(!j.paused||['extracting','moving','extract-request','move-request'].includes(j.phase)||j.phase.startsWith('pkg-')))||Array.from(queue.engines.values()).some(e=>e.running)||transfers.jobs.some(j=>!['complete','selecting','paused','stalled','error','canceled'].includes(j.state))||media.running||transfers.busy)return true;for(const j of [...queue.jobs,...transfers.jobs])if(j.gid&&!['done','complete','canceled'].includes(j.phase||j.state)){let s;try{s=await queue.clientFactory().rpc('tellStatus',[j.gid]);}catch(e){if(/not found/i.test(e.message))continue;return true;}if(!['paused','error','removed','complete'].includes(s.status))return true;}return false;},
 native:(op,v)=>{if(op==='apply'){nativeUpdateFiles('arm',{});nativeLaunchUpdater();return;}return nativeUpdateFiles(op,v);}});
function updateTick(){updater.tick().catch(e=>console.error(e.message));if(updater.state.phase==='applying'){try{const r=JSON.parse(nativeRead('update-result')||'{}');if(r.phase==='waiting-close'||r.phase==='preserving-art'){queue.persist();nativeStop();}else if(r.phase==='error')updater.fail(Error(r.error));}catch{}}setTimeout(updateTick,1500);}setTimeout(updateTick,15000);

let transferReady=false;
function transferTick(){if(transferReady&&!['preparing','applying'].includes(updater.state.phase))transfers.tick().catch(e=>console.error(e.message));setTimeout(transferTick,800);}setTimeout(transferTick,800);

function mediaTick(){if(!['preparing','applying'].includes(updater.state.phase))media.tick().catch(()=>{});setTimeout(mediaTick,250);}setTimeout(mediaTick,250);

queue.init().then(async()=>{await transfers.init();transferReady=true;library.init();for(const j of queue.jobs)if(j.phase==='done')for(const file of j.moved||[])try{library.importFile({path:file,platform:j.game.platform||'ps2',gameId:'legacy:'+RaffIdentity.identity(j.game)});}catch(_){}return queue.scanPaths();}).catch(e=>console.error(e.stack));

function scanTick(){try{library.tick();}catch(e){console.error(e.message);}setTimeout(scanTick,200);}setTimeout(scanTick,200);
function psxs5Tick(){try{if(queue.ready)psxs5.tick();}catch(e){console.error('PSXS5: '+e.message);}setTimeout(psxs5Tick,5000);}setTimeout(psxs5Tick,5000);

const registeredFiles=new Set();
async function poll(){try{if(['preparing','applying'].includes(updater.state.phase)){setTimeout(poll,500);return;}await queue.pump();if(queue.ready)for(const j of queue.jobs)if(j.phase==='done')for(const file of j.moved||[])if(!registeredFiles.has(file)){try{library.importFile({path:file,platform:j.game.platform||'ps2',gameId:'legacy:'+RaffIdentity.identity(j.game)});}catch(_){}registeredFiles.add(file);}}catch(e){console.error(e.stack);}setTimeout(poll,500);}setTimeout(poll,500);

globalThis.route=async(method,path,body)=>{

 try{let v=body?JSON.parse(body):{};

 if(method==='GET'&&path==='/health')return JSON.stringify({app:'raff-native-service',version:'1.1.1',pid:nativePid});

 if(method==='POST'&&path.startsWith('/updates/')){if(path==='/updates/check')return JSON.stringify(await updater.check(true));if(path==='/updates/download')return JSON.stringify(await updater.download());if(path==='/updates/apply')return JSON.stringify(await updater.apply());if(path==='/updates/state')return JSON.stringify(updater.status());}
 if(['preparing','applying'].includes(updater.state.phase)&&method==='POST')throw Error('Update is waiting for Raff to close');
 if(method==='POST'&&path==='/catalog/selected')return JSON.stringify(sources.selected(v,queue));
 if(method==='POST'&&path==='/hub/list'){const result=hub.list({...v,favoriteIds:v.favoritesOnly?(queue.settings.favorites||[]):null});for(const e of result.items)e.favorite=(queue.settings.favorites||[]).includes('hub:'+e.id);return JSON.stringify(result);}
 if(method==='POST'&&path==='/hub/detail')return JSON.stringify(hub.detail(v.id));
 if(method==='POST'&&path==='/hub/reload'){const updated=new RaffHub.Hub(JSON.parse(nativeRead('hub')));hub=updated;transfers.catalog=hub.catalog;return JSON.stringify({entries:hub.catalog.entries.length,releases:hub.catalog.releases.length});}
 if(method==='POST'&&path==='/hub/sources')return JSON.stringify({items:hub.catalog.sources||[],notice:'External indexes do not prove redistribution permission or emulator compatibility.'});
 if(method==='POST'&&path==='/transfers/imports'){await queue.clientFactory().ensureDir('/data/raff/imports');const files=nativeFS('list','/data/raff/imports',v.offset||0);files.entries=files.entries.filter(e=>e.type==='file'&&/\.(torrent|magnet|txt)$/i.test(e.name));return JSON.stringify(files);}
 if(method==='POST'&&path==='/transfers/inspect'){
  if(v.path&&/\.(magnet|txt)$/i.test(v.path)){if(!/^\/data\/raff\/imports\/[^/]+\.(magnet|txt)$/i.test(v.path))throw Error('Import path rejected');const stat=nativeFS('stat',v.path);if(stat.type!=='file'||stat.size>4096)throw Error('Magnet file exceeds 4096 bytes');v.magnet=new TextDecoder().decode(new Uint8Array(nativeFS('slice',v.path,{offset:0,length:stat.size}))).trim();delete v.path;}
  return JSON.stringify(await transfers.inspect(v));
 }
 if(method==='POST'&&path==='/transfers/detail')return JSON.stringify(transfers.detail(v.id,v.offset));
 if(method==='POST'&&path==='/transfers/start')return JSON.stringify(await transfers.start(v.id,v));
 if(method==='POST'&&path==='/transfers/game')return JSON.stringify(await transfers.addGame(v.id,v));
 if(method==='POST'&&path==='/transfers/release')return JSON.stringify(await transfers.addRelease(v.id,v));
 if(method==='POST'&&path==='/transfers/action')return JSON.stringify(await transfers.action(v.id,v.action)||{ok:true});
 if(method==='POST'&&path==='/transfers/settings')return JSON.stringify(await transfers.configure(v));
 if(method==='POST'&&path==='/transfers/clear')return JSON.stringify(transfers.clear());
 if((method==='GET'||method==='POST')&&path==='/transfers/state')return JSON.stringify(transfers.state());
 if((method==='GET'||method==='POST')&&path==='/library/state')return JSON.stringify(library.state());

 if(method==='POST'&&path==='/library/browse'){const result=library.browse(v);for(const g of result.items){const art=media.request(g.id);if(art.cover)g.cover=art.cover;}return JSON.stringify(result);}

 if(method==='POST'&&path==='/library/media')return JSON.stringify(v.ids?{items:v.ids.slice(0,8).map(id=>media.request(id))}:media.request(v.id,true));

 if(method==='POST'&&path==='/library/download'){const a=library.rows('SELECT download_option FROM catalog.assets WHERE release_id=? AND download_option IS NOT NULL LIMIT 1',[v.release])[0];if(!a)throw Error('No download linked');return JSON.stringify(queue.add(a.download_option));}

 if(method==='POST'&&path==='/library/platforms')return JSON.stringify({items:library.platforms(v.target)});

 if(method==='POST'&&path==='/library/detail')return JSON.stringify(library.detail(v.id,v.offset));

 if(method==='POST'&&path==='/library/favorite')return JSON.stringify(library.favorite(v.id,v.enabled));

 if(method==='POST'&&path==='/library/view')return JSON.stringify(library.view(v));

 if(method==='POST'&&path==='/library/local')return JSON.stringify(library.local(v));

 if(method==='POST'&&path==='/library/import')return JSON.stringify(library.importFile(v));

 if(method==='POST'&&path==='/library/scan')return JSON.stringify(library.scan(v));

 if(method==='POST'&&path==='/library/scan-action')return JSON.stringify(library.scanAction(v.id,v.action));

 if(method==='POST'&&path==='/library/scan-status')return JSON.stringify({items:library.scanStatus()});

 if(method==='POST'&&path==='/library/target')return JSON.stringify(library.configureTarget(v));

 if(method==='POST'&&path==='/library/detect')return JSON.stringify(library.detectTarget(v.id||'ps5-local'));

 if(method==='POST'&&path==='/library/firmware')return JSON.stringify(library.firmware(v.core,v.target,v.region));

 if(method==='POST'&&path==='/library/export')return JSON.stringify(library.exportPlaylist(v));

 if(method==='POST'&&path==='/library/multidisc')return JSON.stringify(library.multidisc(v));

 if(method==='POST'&&path==='/library/remove')return JSON.stringify(library.remove(v.id));

 if(method==='POST'&&path==='/library/sources')return JSON.stringify({items:library.rows('SELECT * FROM catalog.sources WHERE metadata_only=0'),authorized:queue.settings.authorizedSources||[]});

 if(method==='POST'&&path==='/library/files')return JSON.stringify(nativeFS('list',RaffLibrary.path(v.path),v.offset||0));


 if(method==='GET'&&path==='/status'){const status=queue.status(),extra=transfers.state();status.updates=updater.status();status.jobs.push(...extra.jobs);status.transfers={...extra,jobs:undefined};status.speed=(status.speed||0)+extra.jobs.reduce((n,j)=>n+j.speed,0);return JSON.stringify(status);}

 if(method==='POST'&&path==='/queue')return JSON.stringify(queue.add(v.id));

 if(method==='POST'&&path==='/action'){if(queue.jobs.some(j=>j.id===v.id))return JSON.stringify(await queue.action(v.id,v.action));return JSON.stringify(await transfers.action(v.id,v.action)||{ok:true});}

 if(method==='POST'&&path==='/settings')return JSON.stringify(await queue.configure(v));

 if(method==='POST'&&path==='/scan-paths'){queue.scanPaths().catch(e=>console.error(e.message));return'{"ok":true}';}

 if(method==='POST'&&path==='/refresh'){queue.refreshInstalled().catch(e=>console.error(e.message));return'{"ok":true}';}

 if(method==='POST'&&path==='/shutdown'){

  if(queue.jobs.some(j=>!['done','canceled'].includes(j.phase)&&!((j.paused||j.error)&&['queued','downloading','resolving','extract-ready','scan','prepare','ready','move-ready'].includes(j.phase)))||Array.from(queue.engines.values()).some(e=>e.running))throw Error('Pause downloads before service shutdown');

  for(const j of queue.jobs.filter(j=>['downloading','resolving'].includes(j.phase))){const s=await queue.clientFactory().rpc('tellStatus',[j.gid]);if(!['paused','error','removed','complete'].includes(s.status))throw Error('Download is not safely stopped');}

  if(transfers.jobs.some(j=>['downloading','metadata','seeding'].includes(j.state)))throw Error('Pause torrent and hub downloads before shutdown');
  queue.persist();nativeStop();return'{"ok":true}';

 }

 return'{"error":"Unknown route"}';

 }catch(e){return JSON.stringify({error:e.message});}

};

