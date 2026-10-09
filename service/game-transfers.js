(function(root){'use strict';
const T=root.RaffTransfers||(typeof require==='function'?require('./transfers.js'):null),BT=root.RaffBT||(typeof require==='function'?require('./bittorrent.js'):null),L=root.PS2Library||(typeof require==='function'?require('../shared/core.js'):null);
const I=root.RaffIdentity||(typeof require==='function'?require('../shared/identity.js'):null);
const done=j=>['complete','canceled'].includes(j.state);
const missingGid=e=>/GID.*not found|not found.*GID|No such download for GID/i.test(e.message);
const freshGid=()=>Array.from(root.crypto.getRandomValues(new Uint8Array(8)),x=>x.toString(16).padStart(2,'0')).join('');
const fields=['status','totalLength','completedLength','downloadSpeed','uploadSpeed','uploadLength','connections','numSeeders','seeder','bitfield','infoHash','pieceLength','numPieces','errorCode','errorMessage','followedBy','dir','verifiedLength','verifyIntegrityPending'];
function selection(s){const out=[];for(const p of String(s).split(',')){if(!/^\d+(?:-\d+)?$/.test(p))throw Error('Invalid engine file selection');const[a,b=a]=p.split('-').map(Number);if(a<1||b<a||b-a>128)throw Error('Invalid engine selection range');for(let n=a;n<=b;n++)out.push(n);}return [...new Set(out)].sort((a,b)=>a-b);}
function validateRelease(g,c){
 if(!g||!c||!L.PLATFORMS[g.platform])throw Error('Unknown game torrent');
 BT.url(c.url);if(!/^[a-f0-9]{40}$/.test(c.infoHash)||!/^sha-256=[a-f0-9]{64}$/.test(c.checksum)||!Number.isSafeInteger(c.metadataBytes)||c.metadataBytes<16||c.metadataBytes>64*1048576)throw Error('Torrent source identity is missing');
 if(!Number.isSafeInteger(c.total)||c.total<=0||c.total>64*1024**4||!Number.isSafeInteger(c.pieceLength)||c.pieceLength<16384||c.pieceLength>67108864||c.pieceLength&(c.pieceLength-1))throw Error('Invalid collection size');
 if(!Array.isArray(g.files)||!g.files.length||g.files.length>32)throw Error('A game must have a bounded selection');
 const indices=new Set(),names=new Set();for(const f of g.files){
  if(!Number.isSafeInteger(f.index)||f.index<1||f.index>c.fileCount||indices.has(f.index)||!Number.isSafeInteger(f.length)||f.length<=0||!Number.isSafeInteger(f.offset)||f.offset<0||f.offset+f.length>c.total)throw Error('Invalid selected file');
  if(typeof f.path!=='string'||f.path.length>900)throw Error('Invalid file path');f.path.split('/').forEach(BT.segment);if(names.has(f.path.toLowerCase()))throw Error('Duplicate selected path');indices.add(f.index);names.add(f.path.toLowerCase());
 }
}
class GameTransfers extends T.Transfers{
 constructor(o){super(o);this.gameCatalog=o.gameCatalog||{collections:{},games:[]};this.gameQueue=o.gameQueue;this.gameById=new Map(this.gameCatalog.games.map(g=>[g.id,g]));}
 async addGame(id,v={}){
  const g=this.gameById.get(id)||this.sources?.torrent(id),c=this.gameCatalog.collections[g?.collection]||this.sources?.collection(g?.collection);validateRelease(g,c);
  if(v.uploadAccepted!==true)throw Error('Torrent downloads upload pieces to peers; accept the notice first');
  if(!this.features.includes('BitTorrent'))throw Error('BitTorrent is not available');
  if(this.jobs.some(j=>j.gameKey===g.key&&!done(j))||this.gameQueue?.jobs.some(j=>!['done','canceled'].includes(j.phase)&&(j.gameKey||I.identity(j.game))===g.key))throw Error('This game is already queued');
  if((this.gameQueue?.installed?.[g.platform]||[]).some(e=>I.sameInstalled(g,e))||this.gameQueue?.sources&&this.gameQueue.status().installedKeys.includes(g.key))throw Error('This game is already installed');
  const j=this.base('torrent',g.platform);Object.assign(j,{catalogGame:true,gameKey:g.key,optionId:id,name:g.title,sourceTorrent:{...c},infoHash:c.infoHash,pieceLength:c.pieceLength,files:g.files.map(f=>({...f})),selected:g.files.map(f=>f.index),total:g.files.reduce((n,f)=>n+f.length,0),collectionLength:c.total,destination:L.platformFor(g.platform).staging+'/'+j.id,gameDestination:this.gameQueue?.destination(g)||this.destination(g.platform),uploadAccepted:true,state:'queued',stage:'metadata'});
  await this.reserve(j);this.jobs.unshift(j);this.save(j);return this.detail(j.id);
 }
 async launchGame(j){
  const o=this.options(j);Object.assign(o,{out:j.infoHash+'.torrent',checksum:j.sourceTorrent.checksum,'follow-torrent':'true','pause-metadata':'true','bt-metadata-only':'false','bt-save-metadata':'false','select-file':j.selected.join(','),split:'2','max-connection-per-server':'2'});
  // Persist the requested GID before RPC: the engine may accept it even if its reply times out.
  j.gid=o.gid;j.stage='metadata';this.save(j);
  j.gid=await this.client.rpc('addUri',[[j.sourceTorrent.url],o]);j.state='metadata';j.lastProgress=this.clock();this.save(j);
 }
 async queryGame(j){return this.client.rpc('tellStatus',[j.gid,fields]);}
 resetMetadata(j){delete j.gid;delete j.pendingPayload;delete j.metadataGid;j.requestedGid=freshGid();j.stage='metadata';j.metadataReceived=0;j.verifying=false;delete j.verifiedLength;this.save(j);}
 clearControlRetry(j){delete j.controlRetryAt;delete j.controlFailures;delete j.connectionNote;}
 retryControl(j,e){
  if(e.rpcTransport!==true)return false;
  j.controlFailures=(j.controlFailures||0)+1;j.speed=0;j.eta=null;
  if(j.controlFailures>6)return false;
  j.controlRetryAt=this.clock()+Math.min(20000,2000*2**(j.controlFailures-1));j.state='queued';
  j.connectionNote='Reconnecting to the download engine; partial files and selection are kept.';j.error='';this.save(j);return true;
 }
 verifyPayload(j,s){if(s.infoHash!==j.infoHash||Number(s.pieceLength)!==j.pieceLength||Number(s.numPieces)!==Math.ceil(j.collectionLength/j.pieceLength)||s.dir!==j.destination)throw Error('Torrent metadata differs from the indexed game');}
 async verifySelection(j){const o=await this.client.rpc('getOption',[j.gid]);if(JSON.stringify(selection(o['select-file']))!==JSON.stringify([...j.selected].sort((a,b)=>a-b)))throw Error('Engine did not retain the game-only file selection');}
 async attachPayload(j,s){
  if(!Array.isArray(s.followedBy)||s.followedBy.length!==1)throw Error('Torrent metadata did not create one paused download');
  const parent=j.gid,child=s.followedBy[0];j.pendingPayload=child;this.save(j);
  let next;try{next=await this.client.rpc('tellStatus',[child,fields]);}catch(e){
   if(!missingGid(e))throw e;
   // A previous failed payload can disappear while its completed metadata receipt survives.
   try{await this.client.rpc('removeDownloadResult',[parent]);}catch(e){if(!missingGid(e))throw e;}
   this.resetMetadata(j);await this.launchGame(j);return;
  }
  if(next.status!=='paused'){await this.client.rpc('forcePause',[child]);throw Error('Torrent payload was not paused for verification');}
  if(next.infoHash!==j.infoHash||Number(next.pieceLength)!==j.pieceLength||Number(next.numPieces)!==Math.ceil(j.collectionLength/j.pieceLength)||next.dir!==j.destination)throw Error('Torrent metadata differs from the indexed game');
  await this.client.rpc('changeOption',[child,{'select-file':j.selected.join(','),'max-upload-limit':String(this.settings.uploadKiB*1024),'max-download-limit':String(this.settings.downloadKiB*1024)}]);
  const options=await this.client.rpc('getOption',[child]);if(JSON.stringify(selection(options['select-file']))!==JSON.stringify([...j.selected].sort((a,b)=>a-b)))throw Error('Engine did not retain the game-only file selection');
  j.metadataGid=parent;j.gid=child;delete j.pendingPayload;j.stage='payload';j.state='downloading';j.lastProgress=this.clock();this.save(j);
  await this.client.rpc('unpause',[child]);try{await this.client.rpc('removeDownloadResult',[parent]);}catch{}
 }
 async prepareGame(j){
  if(!this.gameQueue)throw Error('Game preparation is unavailable');
  const inputs=j.files.map(f=>({name:f.path.split('/').pop(),path:j.destination+'/'+f.path,size:f.length}));
  for(const f of inputs){const actual=this.fs('stat',f.path);if(!actual||actual.type!=='file'||actual.size!==f.size)throw Error('Selected game file is incomplete');}
  await this.client.ensureDir(j.gameDestination);
  // Only selected files enter preparation. Boundary pieces of other games are never scanned.
  this.gameQueue.acceptTorrent({id:j.id,key:j.gameKey,title:j.name,platform:j.platform,destination:j.gameDestination,dir:j.destination,inputs,total:j.total,optionId:j.optionId});
  j.integrity='torrent-pieces';j.state='complete';j.completedAt=this.clock();j.speed=0;j.uploadSpeed=0;this.save(j);
  try{await this.client.rpc('removeDownloadResult',[j.gid]);}catch{}
 }
 async tickGame(j,canStart){
  if(done(j)||j.state==='paused'||j.state==='error'||j.state==='stalled')return 0;
  if(j.controlRetryAt>this.clock())return 0;
  if(j.state==='queued'){
   if(!canStart)return 0;
   // aria2 permits one live payload for an infohash. Keep separate game jobs and serialize the collection.
   if(this.jobs.some(other=>other!==j&&other.infoHash===j.infoHash&&other.gid&&!done(other))){j.waitingForCollection=true;return 0;}delete j.waitingForCollection;
   try{await this.reserve(j);if(j.pendingPayload){await this.attachPayload(j,{followedBy:[j.pendingPayload]});this.clearControlRetry(j);return 1;}if(j.gid){let s;try{s=await this.queryGame(j);}catch(e){if(!missingGid(e))throw e;this.resetMetadata(j);}
     if(s){if(j.stage==='metadata'&&s.status==='complete'){if(+s.completedLength!==j.sourceTorrent.metadataBytes)throw Error('Torrent metadata size changed');await this.attachPayload(j,s);this.clearControlRetry(j);return 1;}
      if(j.stage==='payload'&&['paused','active','waiting','complete'].includes(s.status)){this.verifyPayload(j,s);await this.verifySelection(j);}
      if(s.status==='paused'){await this.client.rpc('unpause',[j.gid]);j.state=j.stage==='metadata'?'metadata':'downloading';j.lastProgress=this.clock();this.clearControlRetry(j);this.save(j);return 1;}
      if(s.status==='active'||s.status==='waiting'||s.status==='complete'&&j.stage==='payload'){j.state=j.stage==='metadata'?'metadata':'downloading';j.lastProgress=this.clock();this.clearControlRetry(j);this.save(j);return 1;}
      await this.client.rpc('removeDownloadResult',[j.gid]);this.resetMetadata(j);}}
    await this.launchGame(j);this.clearControlRetry(j);return 1;
   }catch(e){if(this.retryControl(j,e))return 1;for(const gid of[j.gid,j.pendingPayload].filter(Boolean))try{await this.client.rpc('forcePause',[gid]);}catch{}j.state='error';j.error=e.message;this.save(j);return 0;}
  }
  if(!['metadata','downloading','seeding'].includes(j.state)||!j.gid)return 0;
  try{
   const s=await this.queryGame(j);j.peers=+s.connections||0;j.seeders=+s.numSeeders||0;j.speed=j.stage==='payload'?(+s.downloadSpeed||0):0;j.uploadSpeed=+s.uploadSpeed||0;j.uploaded=+s.uploadLength||0;
   if(s.status==='error'||s.status==='removed')throw Error(s.errorCode==='32'?'Torrent metadata checksum changed; game download was not started.':s.errorMessage||'Source download failed');
   if(j.stage==='metadata'){
    const bytes=+s.completedLength||0;if(bytes>(j.metadataReceived||0)){j.metadataReceived=bytes;j.lastProgress=this.clock();}if(bytes>j.sourceTorrent.metadataBytes)throw Error('Torrent metadata exceeds indexed size');
    if(s.status==='complete'){if(bytes!==j.sourceTorrent.metadataBytes)throw Error('Torrent metadata size changed');await this.attachPayload(j,s);return 0;}
   }else{
    if(s.infoHash!==j.infoHash)throw Error('Torrent identity changed');
    // Large pieces can take minutes to finish. Network activity keeps the task alive,
    // but only verified selected-piece bytes are credited to the game progress bar.
    const engineReceived=+s.completedLength||0;
    if(engineReceived>(j.engineReceived||0)||+s.downloadSpeed>0)j.lastProgress=this.clock();j.engineReceived=engineReceived;
    // Hash scanning is engine work, not downloaded bytes. Do not count it as a stalled source.
    const verified=+s.verifiedLength||0;j.verifying=s.verifyIntegrityPending==='true'||s.verifiedLength!==undefined;
    if(verified>(j.verifiedLength||0)){j.verifiedLength=verified;j.lastProgress=this.clock();}
    const received=BT.completed(j.files,j.selected,j.pieceLength,s.bitfield)||0;if(received>j.received){j.received=received;j.lastProgress=this.clock();}j.eta=j.speed>0&&j.total>j.received?Math.ceil((j.total-j.received)/j.speed):null;
    if(s.status==='complete'){if(j.received!==j.total)throw Error('Selected game pieces are incomplete');await this.prepareGame(j);return 0;}if(s.seeder==='true'){j.state='seeding';}
   }
   this.clearControlRetry(j);
   if(s.status==='paused'){j.state='paused';this.save(j);return 0;}
   if(this.clock()-j.lastProgress>120000&&j.state!=='seeding'){await this.client.rpc('forcePause',[j.gid]);j.state='stalled';j.error='No source progress for two minutes. Resume to retry.';this.save(j);return 0;}
   if(this.clock()-(j.lastSave||0)>5000){j.lastSave=this.clock();this.save(j);}
  }catch(e){if(this.retryControl(j,e))return 0;for(const gid of[j.gid,j.pendingPayload].filter(Boolean))try{await this.client.rpc('forcePause',[gid]);}catch{}j.state='error';j.error=e.message;this.save(j);}return 0;
 }
 async action(id,action){
  const j=this.find(id);if(!j.catalogGame)return super.action(id,action);if(done(j))throw Error('Transfer is already complete or canceled');
  if(action==='pause'){if(j.gid)try{await this.client.rpc('forcePause',[j.gid]);}catch{}j.state='paused';j.speed=0;this.save(j);return;}
  if(action==='resume'){j.state='queued';j.error='';this.clearControlRetry(j);this.save(j);return;}
  if(action==='cancel'){
   const gids=new Set([j.gid,j.pendingPayload].filter(Boolean));if(j.gid&&j.stage==='metadata')try{const s=await this.queryGame(j);for(const gid of s.followedBy||[])gids.add(gid);}catch{}
   for(const gid of gids){try{await this.client.rpc('forceRemove',[gid]);}catch{}try{await this.client.rpc('removeDownloadResult',[gid]);}catch{}}
   j.state='canceled';j.speed=0;j.error='Partial files kept at '+j.destination;this.save(j);return;
  }throw Error('Unsupported game transfer action');
 }
 detail(id,offset=0){const d=super.detail(id,offset),j=this.find(id);if(j.catalogGame)Object.assign(d,{catalogGame:true,gameKey:j.gameKey,selectionLocked:true,gameDestination:j.gameDestination});return d;}
 summary(j){const d=super.summary(j);if(j.catalogGame)Object.assign(d,{key:j.gameKey,catalogGame:true,phase:j.stage==='metadata'&&!['paused','error','stalled','canceled','complete'].includes(j.state)?'metadata':j.state,destination:j.gameDestination,waitingForCollection:!!j.waitingForCollection,reconnecting:!!j.controlRetryAt,verifying:!!j.verifying,connectionNote:j.connectionNote||''});return d;}
}
const api={GameTransfers,validateRelease,selection};if(typeof module!=='undefined')module.exports=api;else root.RaffGameTransfers=api;
})(globalThis);
