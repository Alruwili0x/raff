(function(root){
'use strict';
const L=root.PS2Library||(typeof require==='function'?require('../shared/core.js'):null);
const I=root.RaffIdentity||(typeof require==='function'?require('../shared/identity.js'):null);
const P=root.RaffPackages||(typeof require==='function'?require('./packages.js'):null);
const T=root.RaffTransport||(typeof require==='function'?require('./transport.js'):null);
function errorInfo(j){const error=j.error||'';if(!error)return{errorKind:'',errorDetail:''};const http=error.match(/(?:status[=: ]+|HTTP )(\d{3})/i);if(http)return{errorKind:'sourceerror',errorDetail:'HTTP '+http[1]+' - source server failed. Press X to retry.'};if(/space|المساحة/i.test(error))return{errorKind:'spaceerror',errorDetail:'Not enough free storage. The downloaded file is kept.'};if(j.phase.startsWith('pkg-')||(['ps4','ps5'].includes(j.game?.platform)&&/install|PKG/i.test(error)))return{errorKind:'installerror',errorDetail:'Installation is not confirmed. PKG is kept. Check PS5 downloads.'};if(/HTTP|connect|network|GID|التنزيل|الاتصال/i.test(error))return{errorKind:'networkerror',errorDetail:'Connection interrupted. Press X to resume the partial download.'};return{errorKind:'fileerror',errorDetail:error.replace(/[^\x20-\x7e]/g,' ').trim().slice(0,180)||'File operation failed. The existing files are kept.'};}
const active=j=>!['done','canceled'].includes(j.phase);
class FileGate{
 constructor(){this.owner=null;this.waiting=[];this.sticky=null;}
 async take(owner){if(this.owner===owner)return;if(!this.owner){this.owner=owner;return;}await new Promise(resolve=>this.waiting.push({owner,resolve}));}
 release(owner){if(this.owner!==owner)return;this.sticky=null;const next=this.waiting.shift();this.owner=next?.owner||null;if(next)next.resolve();}
 async call(owner,fn,sticky=false){await this.take(owner);if(sticky)this.sticky=owner;try{return await fn();}finally{if(this.sticky!==owner)this.release(owner);}}
}
class Queue{
 constructor({catalog,read,write,clientFactory,clock=()=>Date.now()}){this.catalog=catalog;this.discovery=catalog.games;this.read=read;this.write=write;this.clock=clock;this.clientFactory=clientFactory||(()=>new L.Client({wfm:'http://127.0.0.1:8888',aria:'http://127.0.0.1:6800',secret:this.read('rpc-token')||''}));this.gate=new FileGate();this.engines=new Map();this.jobs=[];this.systemTitleIds=[];this.lastInstalledRefresh=0;this.installed=Object.fromEntries(Object.keys(L.PLATFORMS).map(p=>[p,[]]));this.settings={parallel:3,connections:16,language:'ar',favorites:[],autoInstall:false,removePackages:false};this.settings.paths={};this.settings.gamePaths={};this.pathScan={running:false,results:{}};this.lastSave=0;this.polling=false;this.ready=false;this.error='';}
 persist(){this.write('jobs',JSON.stringify(this.jobs.map(j=>({...j,speed:0,eta:null}))));this.lastSave=this.clock();}
 async init(){try{Object.assign(this.settings,JSON.parse(this.read('settings')||'{}'));this.settings.parallel=Math.min(4,Math.max(1,Number(this.settings.parallel)||3));this.settings.connections=Math.min(16,Math.max(1,Number(this.settings.connections)||16));}catch(_){}
 for(const k of ['paths','gamePaths'])if(!this.settings[k]||typeof this.settings[k]!=='object'||Array.isArray(this.settings[k]))this.settings[k]={};
 if(this.settings.speedProfile!==2){this.settings.connections=16;this.settings.speedProfile=2;this.write('settings',JSON.stringify(this.settings));}
 try{this.jobs=JSON.parse(this.read('jobs')||'[]');if(!Array.isArray(this.jobs))this.jobs=[];}catch(_){this.jobs=[];}
 // A lost WFM completion must be re-extracted into a fresh directory, never trusted as complete.
 let liveTasks=null;try{liveTasks=(await this.clientFactory().api('tasks')).tasks||[];}catch{}
 for(const j of this.jobs){if(j.phase==='extracting'&&j.error&&/لم يحتفظ|completion|نتيجة العملية/.test(j.error)&&liveTasks&&!liveTasks.some(t=>t.id===j.taskId&&['running','queued'].includes(t.state))){j.previousExtractDir=j.extractDir;j.extractDir=j.dir+'/extracted-retry-'+this.clock();j.phase='extract-ready';j.archiveIndex=0;j.paused=true;j.recovered=true;j.error='Extraction confirmation was lost. Resume to verify the archive again; existing files are kept.';}}
 for(const j of this.jobs){j.speed=0;j.eta=null;if(active(j)){j.paused=true;j.recovered=true;if(!j.installOptions)j.installOptions={enabled:this.settings.autoInstall===true&&['ps4','ps5'].includes(j.game.platform),remove:this.settings.removePackages===true};}if(j.phase!=='queued')this.attach(j);}
 // Finish observing a previously-started file operation before letting other file work in.
 const fileJobs=this.jobs.filter(j=>(['extracting','moving','extract-request','move-request'].includes(j.phase)||j.phase.startsWith('pkg-')));
 if(fileJobs.length){await this.gate.take(fileJobs[0].id);this.gate.sticky=fileJobs[0].id;}
 try{const c=this.clientFactory();await c.rpc('changeGlobalOption',[{'max-concurrent-downloads':String(this.settings.parallel),'max-overall-download-limit':'0'}]);this.ready=true;}catch(e){this.error=e.message;}
 if(!fileJobs.length)await this.refreshInstalled();this.persist();}
 attach(j){
 // Refresh only locations of the same catalogued file; never change a partial file's identity.
 const fresh=this.catalog.games.find(g=>g.id===j.game.id&&g.filename===j.game.filename&&g.titleId===j.game.titleId&&g.sizeBytes===j.game.sizeBytes);
 if(fresh?.url&&fresh.mirrors){j.game.url=fresh.url;j.game.mirrors=fresh.mirrors;}
 const c=this.clientFactory(),owner=j.id,api=c.api.bind(c),bytes=c.bytes.bind(c);c.api=(route,v,form)=>this.gate.call(owner,()=>api(route,v,form),route==='extract'||route==='move'||route==='install-pkg'||route==='delete');c.bytes=(path,max,start)=>this.gate.call(owner,()=>bytes(path,max,start));
 const add=c.add.bind(c);c.add=async(url,dir,name,gid,checksum)=>{const plan=T.plan(this,j,url,name);c.connections=plan.connections;if(!plan.metadata)j.transferConnections=plan.connections;return add(url,dir,name,gid,checksum,url===j.game.url?j.game.mirrors||[]:[]);};
 const rpc=c.rpc.bind(c);c.rpc=async(method,args)=>{try{return await rpc(method,args);}catch(error){if(method!=='tellStatus'||args[0]!==j.gid||!/GID.*not found|not found.*GID/i.test(error.message)||!['downloading','resolving'].includes(j.phase))throw error;
 const resolving=j.phase==='resolving',url=resolving?(j.game.source==='dlarchive'?'https://uptobox.dlarchive.my.id/files_api.php?action=file-url&id='+encodeURIComponent(j.game.fileId):j.game.sourcePage+'/download'):j.game.url;
 await c.add(url,j.dir,resolving?'source.html':j.game.filename,j.gid,resolving?undefined:j.game.checksum);return rpc(method,args);}};
 const available=c.available.bind(c);c.available=async path=>Math.max(0,await available(path)-this.jobs.filter(other=>other.id!==j.id&&active(other)).reduce((n,other)=>n+Math.max(0,(other.game.sizeBytes||64*1048576)-(other.received||0))+Number(other.game.unpackedBytes||0),0));
 const list=c.list.bind(c);c.list=async path=>{const entries=await list(path);if(path===L.destinationFor(j.game)&&['prepare','ready'].includes(j.phase)&&entries.some(e=>e.type==='-'&&I.sameInstalled(j.game,e)))throw Error('اللعبة موجودة بالفعل / Already installed');return entries;};
 c.installPackages=(...args)=>new P.Packages(c).run(...args);
 const engine=new L.Engine(c,job=>{Object.assign(j,job);this.persist();},()=>this.update(j));engine.job=j;this.engines.set(j.id,engine);return engine;}
 update(j){const now=this.clock();if(j.phase==='downloading'&&j.speed>0){j.smoothSpeed=j.smoothSpeed?j.smoothSpeed*0.7+j.speed*0.3:j.speed;j.eta=j.total>j.received?Math.ceil((j.total-j.received)/j.smoothSpeed):0;}else j.eta=null;
 if(j.phase!=='downloading')j.speed=0;
 if(!active(j)){this.gate.release(j.id);if(j.phase==='done'){for(const r of j.packageReceipts||[])if(r.confirmedAt&&!this.systemTitleIds.includes(r.titleId))this.systemTitleIds.push(r.titleId);const p=j.game.platform||'ps2';for(const path of j.moved||[])if(!(j.packageReceipts||[]).some(r=>r.path===path&&r.removedAt)&&!this.installed[p].some(e=>e.path===path))this.installed[p].push({name:path.split('/').pop(),path,type:'-'});}}
 if(now-this.lastSave>5000)this.persist();}
 destination(game){return L.destinationPath(this.settings.gamePaths[I.identity(game)]||this.settings.paths[game.platform||'ps2']||L.platformFor(game.platform).games);}
 async scanPaths(){if(this.pathScan.running)return;this.pathScan={running:true,results:{}};const c=this.clientFactory();
 let mounts=[];try{mounts=(await this.gate.call('scan-paths',()=>c.list('/mnt'))).filter(e=>e.type==='d'&&/^usb[0-7]$/.test(e.name)).map(e=>'/mnt/'+e.name);}catch(_){}const scanned=new Map();
 try{for(const p of Object.keys(L.PLATFORMS)){const def=L.platformFor(p).games,defaults=L.PLATFORMS[p].core?[def]:p==='ps3'?['/data/rpcs3/games']:p==='xbox'?['/data/xemu/games']:p==='xbox360'?['/data/xbox360']:p==='ps1'?['/data/PSXS5/games','/data/homebrew/PPSA99169/content/PS1','/data/homebrew/PPSA99169/content','/data/RetroArch/roms/ps1','/data/homebrew/PPSA99169/content/psx']:p==='ps2'?['/data/PCSX2/games','/data/PS5SX2/games']:p==='switch'?['/data/prosperoeden/roms','/data/eden/roms']:['/data/etaHEN/games','/data/games'];
 const candidates=[...new Set([this.settings.paths[p],...defaults,...mounts.flatMap(m=>[m+'/games',m+'/'+(p==='ps3'?'PS3':p==='xbox'?'xbox':p==='xbox360'?'xbox360':p==='ps1'?'PS1':p==='ps2'?'PS2':p==='switch'?'roms':p==='ps4'?'PS4':'PS5')])].filter(Boolean))];let found=[];
 for(const path of candidates)try{let rows=scanned.get(path);if(!rows){rows=await this.gate.call('scan-paths',()=>c.list(path));scanned.set(path,rows);}const count=rows.filter(e=>e.type==='-'&&L.platformFor(p).formats.includes((e.name.split('.').pop()||'').toLowerCase())).length;found.push({path,count});}catch(_){}
 found.sort((a,b)=>b.count-a.count||(a.path===def?-1:b.path===def?1:0));const chosen=found[0];this.pathScan.results[p]={found:!!chosen,path:chosen?.path||def,count:chosen?.count||0};if(chosen&&!this.settings.paths[p])this.settings.paths[p]=chosen.path;
 }this.write('settings',JSON.stringify(this.settings));await this.refreshInstalled();}finally{this.pathScan.running=false;}}
 async refreshInstalled(){const c=this.clientFactory(),cache=new Map();try{const dirs=await this.gate.call('refresh',()=>c.list('/user/app'));this.systemTitleIds=dirs.filter(e=>e.type==='d'&&/^(?:CUSA|PPSA)\d{5}$/.test(e.name)).map(e=>e.name);}catch(_){}this.lastInstalledRefresh=this.clock();for(const p of Object.keys(L.PLATFORMS))try{const games=this.discovery.filter(g=>(g.platform||'ps2')===p),paths=new Set([L.platformFor(p).games,this.settings.paths[p],...Object.entries(this.settings.gamePaths).filter(([k])=>k.startsWith(p+':')).map(([,v])=>v),...this.jobs.filter(j=>(j.game.platform||'ps2')===p&&j.game.destination).map(j=>j.game.destination)].filter(Boolean)),all=[];
 for(const path of paths){let rows=cache.get(path);if(!rows)try{rows=await this.gate.call('refresh',()=>c.list(path));cache.set(path,rows);}catch(_){continue;}
 for(const entry of rows){if(entry.type!=='-'||!L.platformFor(p).formats.includes((entry.name||'').split('.').pop().toLowerCase()))continue;const named=entry.name||'',id=named.match(/(?:PPSA|CUSA)\d{5}/i),known=games.some(g=>g.filename===named)||this.jobs.some(j=>(j.game.platform||'ps2')===p&&(j.moved||[]).includes(entry.path));if(['ps4','ps5'].includes(p)&&!(id?(p==='ps4'?id[0].toUpperCase().startsWith('CUSA'):id[0].toUpperCase().startsWith('PPSA')):known))continue;all.push({...entry,platform:p});}}
 this.installed[p]=Array.from(new Map(all.map(e=>[e.path||e.name,e])).values());}catch(e){this.error=e.message;}}

 add(optionId){const game=this.catalog.games.find(g=>g.id===optionId)||this.sources?.direct(optionId);if(!game)throw Error('Unknown game');L.validateGame(game);const key=I.identity(game),disc=I.titleParts(game.filename||game.title).disc;
 if(this.jobs.some(j=>active(j)&&I.identity(j.game)===key&&I.titleParts(j.game.filename||j.game.title).disc===disc))throw Error('اللعبة في قائمة التحميل / Already queued');
 if(this.systemTitleIds.includes(game.titleId)||(this.installed[game.platform||'ps2']||[]).some(e=>I.sameInstalled(game,e)))throw Error('اللعبة مثبتة بالفعل / Already installed');
 const b=new Uint8Array(8);root.crypto.getRandomValues(b);const id=Array.from(b,x=>x.toString(16).padStart(2,'0')).join('');const j={id,installOptions:{enabled:this.settings.autoInstall===true&&['ps4','ps5'].includes(game.platform),remove:this.settings.removePackages===true},game:{...game,destination:this.destination(game)},phase:'queued',created:new Date().toISOString(),progress:0,paused:false};this.jobs.push(j);this.persist();return{id};}
 acceptTorrent(v){
 if(this.jobs.some(j=>j.id===v.id))return;
 const game={id:v.optionId,title:v.title,platform:v.platform,filename:v.inputs[0].name,sizeBytes:v.total,destination:v.destination,unpackedBytes:0};
 const archives=L.platformFor(v.platform).keepArchive?[]:v.inputs.filter(f=>/\.(zip|7z|rar)$/i.test(f.name));
 const j={id:v.id,gameKey:v.key,game,dir:v.dir,inputs:v.inputs,archives,archiveIndex:0,phase:archives.length?'extract-ready':'scan',endpoint:'http://127.0.0.1:8888',rpcURL:'http://127.0.0.1:6800/jsonrpc',created:new Date().toISOString(),moved:[],progress:0,received:v.total,total:v.total,paused:false};
 this.jobs.push(j);this.attach(j);this.persist();return j;
 }
 async pump(){if(this.polling)return;this.polling=true;try{if(!this.ready){try{await this.clientFactory().rpc('getVersion',[]);this.ready=true;this.error='';}catch(e){this.error=e.message;return;}}
 if(!this.gate.owner&&this.clock()-this.lastInstalledRefresh>60000)await this.refreshInstalled();
 let count=Array.from(this.engines.values()).filter(e=>e.running).length;
 for(const j of this.jobs){if(count>=this.settings.parallel)break;if(!active(j)||j.paused||j.error||this.engines.get(j.id)?.running)continue;count++;if(j.phase==='queued'){Object.assign(j,{dir:L.platformFor(j.game.platform).staging+'/'+j.id,phase:'prepare',endpoint:'http://127.0.0.1:8888',rpcURL:'http://127.0.0.1:6800/jsonrpc',moved:[]});this.attach(j);}
 const e=this.engines.get(j.id);e.resume().catch(err=>{if(j.paused)delete j.error;else j.error=err.message;}).finally(async()=>{j.speed=0;j.eta=null;const uncertain=['extract-request','move-request','extracting','moving'].includes(j.phase);if(!uncertain)this.gate.release(j.id);this.persist();});}
 }finally{this.polling=false;}}
 async action(id,action){const j=this.jobs.find(j=>j.id===id);if(!j)throw Error('Unknown task');const e=this.engines.get(id);
 if(action==='pause'){if(j.phase==='queued'){j.paused=true;}else{await e.pause();j.paused=true;j.speed=0;j.eta=null;}}
 else if(action==='resume'){if(e?.running)throw Error('انتظر توقف المهمة / Wait for task to pause');j.paused=false;delete j.error;}
 else if(action==='cancel'){if(j.phase==='queued')j.phase='canceled';else await e.cancel();this.gate.release(id);}
 else throw Error('Unknown action');this.persist();return{ok:true};}
 async configure(changes){
 const paths={...this.settings.paths},gamePaths={...this.settings.gamePaths};const requested=[];
 if(changes.paths)for(const[p,v]of Object.entries(changes.paths)){L.platformFor(p);if(v)requested.push([paths,p,L.destinationPath(v)]);else delete paths[p];}
 if(changes.gamePath){const{key,path}=changes.gamePath;if(!this.discovery.some(g=>I.identity(g)===key)&&!this.sources?.card(key))throw Error('Unknown game');if(path)requested.push([gamePaths,key,L.destinationPath(path)]);else delete gamePaths[key];}
 for(const[map,key,path]of requested){await this.gate.call('settings-path',()=>this.clientFactory().ensureDir(path));map[key]=path;}
 this.settings.paths=paths;this.settings.gamePaths=gamePaths;
 for(const key of ['autoInstall','removePackages'])if(typeof changes[key]==='boolean')this.settings[key]=changes[key];
 if(changes.parallel!==undefined)this.settings.parallel=Math.min(4,Math.max(1,Number(changes.parallel)||3));if(changes.connections!==undefined)this.settings.connections=Math.min(16,Math.max(1,Number(changes.connections)||16));if(changes.language)this.settings.language=changes.language==='en'?'en':'ar';if(Array.isArray(changes.favorites))this.settings.favorites=changes.favorites.filter(x=>typeof x==='string').slice(0,10000);this.write('settings',JSON.stringify(this.settings));await this.clientFactory().rpc('changeGlobalOption',[{'max-concurrent-downloads':String(this.settings.parallel)}]);if(changes.paths||changes.gamePath)this.refreshInstalled().catch(e=>{this.error=e.message;});return this.settings;}
 status(){const signature=JSON.stringify(Object.values(this.installed).map(a=>a.map(e=>e.name)));if(signature!==this.installedSignature){this.installedSignature=signature;this.installedKeys=[...I.installedKeys(this.discovery,this.installed),...(this.sources?.installed(this.installed)||[])];}const installedKeys=Array.from(new Set([...(this.installedKeys||[]),...this.catalog.games.filter(g=>this.systemTitleIds.includes(g.titleId)).map(g=>I.identity(g))]));return{version:'1.1.1',updatedAt:this.clock(),ready:this.ready,error:this.error,settings:this.settings,pathScan:this.pathScan,installed:this.installed,installedKeys,jobs:this.jobs.map(j=>({id:j.id,key:j.gameKey||I.identity(j.game),title:j.game.title,platform:j.game.platform||'ps2',destination:L.destinationFor(j.game),phase:j.phase,installing:j.phase.startsWith('pkg-'),packageRemoved:(j.packageReceipts||[]).some(r=>r.removedAt),paused:!!j.paused,error:j.error||'',...errorInfo(j),progress:j.phase==='done'?1:j.phase==='downloading'&&j.total>0?Math.min(1,(j.received||0)/j.total):j.progress||0,received:j.received||0,total:j.total||j.game.sizeBytes||0,speed:j.speed||0,eta:j.eta??null})),speed:this.jobs.reduce((n,j)=>n+(j.speed||0),0)};}
}
const api={Queue,FileGate};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.RaffQueue=api;
})(globalThis);
