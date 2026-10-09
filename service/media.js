/* On-demand official Libretro thumbnails. Covers and gameplay stay separate.
 * Requests are single-stream, bounded, cached and never block catalog queries. */
(function(root){
'use strict';
const directory='/data/homebrew/PPSA99178/assets/media';
const parse=s=>{try{return JSON.parse(s);}catch{return{};}};
class Media{
 constructor(library,queue){this.library=library;this.queue=queue;this.waiting=[];this.known=new Set();this.running=false;this.lastRequest=0;}
 cached(id){return parse(this.library.rows('SELECT value FROM metadata_cache WHERE game_id=?',[id])[0]?.value||'{}').media||{id,screenshots:[]};}
 request(id,gameplay=false){const cached=this.cached(id);if(!this.known.has(id)&&(!cached.checkedAt||!cached.cover&&Date.now()-Date.parse(cached.checkedAt)>1800000||gameplay&&!cached.gameplayChecked)&&this.waiting.length<24){this.known.add(id);this.waiting.push({id,gameplay});}return cached;}
 async tick(){if(this.running||!this.waiting.length||Date.now()-this.lastRequest<750)return;this.running=true;const task=this.waiting.shift();this.lastRequest=Date.now();
  try{await this.fetch(task);}catch(e){/* Missing/broken art never breaks discovery. */}finally{this.known.delete(task.id);this.running=false;}
 }
 async image(client,url,name){const rows=this.library.fs('stat',directory+'/'+name);if(rows?.type==='file'&&rows.size>24&&rows.size<=4194304)return true;
  const gid=await client.rpc('addUri',[[url],{dir:directory,out:name,split:'1','max-connection-per-server':'1','max-tries':'2','retry-wait':'3','connect-timeout':'8',timeout:'12','auto-file-renaming':'false','allow-overwrite':'false','file-allocation':'none','follow-torrent':'false','follow-metalink':'false','check-certificate':'true','max-download-limit':'2M'}]);
  let complete=false;try{for(let n=0;n<100;n++){const status=await client.rpc('tellStatus',[gid,['status','totalLength','completedLength']]);if(Number(status.totalLength)>4194304||Number(status.completedLength)>4194304)throw Error('Image limit');if(status.status==='error'||status.status==='removed')return false;if(status.status==='complete'){complete=true;break;}await new Promise(r=>setTimeout(r,200));}if(!complete)return false;
   const bytes=new Uint8Array(nativeHTTP('http://127.0.0.1:8888/fs?path='+encodeURIComponent(directory+'/'+name),'GET','','',32,true).body);if(bytes.length<24||bytes[0]!==137||bytes[1]!==80||bytes[2]!==78||bytes[3]!==71)return false;const n=i=>bytes[i]*16777216+bytes[i+1]*65536+bytes[i+2]*256+bytes[i+3];return n(16)>0&&n(20)>0&&n(16)<=2048&&n(20)<=2048;
  }finally{if(!complete)await client.rpc('forceRemove',[gid]).catch(()=>{});else await client.rpc('removeDownloadResult',[gid]).catch(()=>{});}
 }
 async fetch({id,gameplay}){if(id.startsWith('source-'))return this.fetchSource(id);const l=this.library,g=l.rows('SELECT * FROM catalog.games WHERE id=?',[id])[0];if(!g)return;const m=parse(g.metadata),p=l.rows('SELECT * FROM catalog.platforms WHERE id=?',[g.platform])[0];let releases=l.rows('SELECT id,source_key,metadata FROM catalog.releases WHERE game_id=? ORDER BY source_key LIMIT 3',[id]);
  const previous=parse(l.rows('SELECT value FROM metadata_cache WHERE game_id=?',[id])[0]?.value||'{}');let media=previous.media||{id,screenshots:[]};let database=p.name;
  if(g.legacy_key){const equivalent={ps2:'sony-playstation-2'}[g.platform];const match=equivalent?l.rows('SELECT id FROM catalog.games WHERE platform=? AND search_title=? LIMIT 2',[equivalent,g.search_title]):[];if(match.length===1){releases=l.rows('SELECT id,source_key,metadata FROM catalog.releases WHERE game_id=? ORDER BY source_key LIMIT 3',[match[0].id]);database=l.rows('SELECT name FROM catalog.platforms WHERE id=?',[equivalent])[0].name;media.match='exact-title-and-platform';}else{media.checkedAt=new Date().toISOString();media.gameplayChecked=true;this.save(id,previous,media);return;}}
  // Bound persistent artwork to 256 games. Existing cache remains useful offline.
  if(!previous.media&&l.rows('SELECT count(*) n FROM metadata_cache')[0].n>=256)return;
  const client=this.queue.clientFactory();await this.queue.gate.call('artwork',()=>client.ensureDir(directory));
  for(const r of releases){const label=r.source_key.replace(/[&*/:`<>?\\|]/g,'_'),key='media-'+r.id;const base='https://thumbnails.libretro.com/'+encodeURIComponent(database)+'/';
   if(!media.cover&&await this.image(client,base+'Named_Boxarts/'+encodeURIComponent(label)+'.png',key+'-box.png'))media.cover=key+'-box';
   if(gameplay&&media.screenshots.length<3&&!media.screenshots.some(s=>s.key===key+'-snap')&&await this.image(client,base+'Named_Snaps/'+encodeURIComponent(label)+'.png',key+'-snap.png')){const hash=l.fs('md5',directory+'/'+key+'-snap.png');if(!media.screenshots.some(s=>s.md5===hash))media.screenshots.push({key:key+'-snap',md5:hash,source:base+'Named_Snaps/'+encodeURIComponent(label)+'.png',release:r.id,kind:'gameplay',match:'exact-provider-release-label'});}
   if(media.cover&&(!gameplay||media.screenshots.length>=3))break;
  }
  media.checkedAt=new Date().toISOString();media.gameplayChecked=!!gameplay;this.save(id,previous,media);
 }
 async fetchSource(id){const c=this.queue.sources?.art(id);if(!c?.artwork)return;const l=this.library,previous=parse(l.rows('SELECT value FROM metadata_cache WHERE game_id=?',[id])[0]?.value||'{}'),media=previous.media||{id,screenshots:[]};if(!previous.media&&l.rows('SELECT count(*) n FROM metadata_cache')[0].n>=3000)return;const client=this.queue.clientFactory();await this.queue.gate.call('artwork',()=>client.ensureDir(directory));
  for(const label of c.artwork.labels.slice(0,3)){const key='media-'+id+'-box',url='https://thumbnails.libretro.com/'+encodeURIComponent(c.artwork.database)+'/Named_Boxarts/'+encodeURIComponent(label.replace(/[&*/:`<>?\\|]/g,'_'))+'.png';if(await this.image(client,url,key+'.png')){media.cover=key;break;}}
  media.checkedAt=new Date().toISOString();media.gameplayChecked=true;this.save(id,previous,media);
 }
 save(id,previous,media){this.library.run('INSERT INTO metadata_cache(game_id,value,updated_at) VALUES(?,?,?) ON CONFLICT(game_id) DO UPDATE SET value=excluded.value,updated_at=excluded.updated_at',[id,JSON.stringify({...previous,media}),new Date().toISOString()]);}
}
root.RaffMedia={Media};
})(globalThis);
