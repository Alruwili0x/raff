// Build-time indexing only: the console never loads a collection's full file list into the UI.
import fs from'node:fs/promises';import{createHash}from'node:crypto';import I from'../shared/identity.js';
import {fileURLToPath} from 'node:url';
const research=process.argv[2]||fileURLToPath(new URL('../work/research',import.meta.url)),out=process.argv[3]||fileURLToPath(new URL('../assets',import.meta.url));
const cards=JSON.parse(await fs.readFile(out+'/catalog.json')),full=JSON.parse(await fs.readFile(out+'/catalog-full.json'));
for(const c of cards)c.options=c.options.filter(o=>o.method!=='torrent');
const byKey=new Map(cards.map(c=>[c.key,c])),collections={},games=[];
const definitions=[
 ['ps1-chd','ps1','Internet Archive/chadmaster/chd_psx/CHD-PSX-USA/','Minerva_Myrient - Internet Archive - chadmaster.torrent','https://minerva-archive.org/browse/Internet%20Archive/chadmaster/chd_psx/CHD-PSX-USA/'],
 ['ps1-redump','ps1','Redump/Sony - PlayStation/','Minerva_Myrient - Redump - Sony - PlayStation.torrent','https://minerva-archive.org/browse/Redump/Sony%20-%20PlayStation/'],
 ['ps2-redump','ps2','Redump/Sony - PlayStation 2/','Minerva_Myrient - Redump - Sony - PlayStation 2.torrent','https://minerva-archive.org/browse/Redump/Sony%20-%20PlayStation%202/']
];
for(const[id,platform,prefix,filename,page]of definitions){
 const data=JSON.parse(await fs.readFile(research+'/'+id+'-index.json'));
 const url='https://minerva-archive.org/assets/Minerva_Myrient_v0.3/'+encodeURIComponent(filename);
 collections[id]={id,url,sourcePage:page,infoHash:data.infoHash,checksum:'sha-256='+data.sha256,metadataBytes:data.size,pieceLength:data.pieceLength,total:data.total,fileCount:data.files.length,source:'Minerva / r-roms'};
 const releases=new Map();
 for(const f of data.files){
  if(!f.path.startsWith(data.name+'/'+prefix)||!/\.(chd|zip|7z|iso)$/i.test(f.path))continue;
  const path=f.path.split('/');if(path.some(s=>!s||s==='.'||s==='..'||/[\x00-\x1f\\:*?"<>|]/.test(s))||f.path.length>900||!Number.isSafeInteger(f.offset)||f.length<=0)throw Error('Unsafe indexed source file');
  const name=path.at(-1),parts=I.titleParts(name),key=platform+':'+parts.key;
  const variant=parts.tags.filter(t=>!/^(?:disc|disk|dvd|cd)\s*\d/i.test(t)).join(' · ');
  // Normalize card identity, but only combine discs of the exact published edition.
  // Case/spelling/format variants in a source are alternative releases, never extra discs.
  const edition=name.replace(/\((?:disc|disk|dvd|cd)\s*\d+[^)]*\)/gi,'').replace(/\s+/g,' ').trim(),group=key+'|'+variant+'|'+edition;
  let g=releases.get(group);if(!g){g={key,platform,title:parts.title,variant,collection:id,files:[]};releases.set(group,g);}g.files.push(f);
 }
 for(const g of releases.values()){
  g.files.sort((a,b)=>a.index-b.index);g.id='torrent:'+id+':'+createHash('sha1').update(g.files.map(f=>f.path).join('\n')).digest('hex').slice(0,20);games.push(g);
  let card=byKey.get(g.key);if(!card){card={key:g.key,platform,title:g.title,cover:'',genre:'',date:'',score:-1,downloads:-1,size:0,options:[]};cards.push(card);byKey.set(g.key,card);}
  card.options.push({id:g.id,source:'minerva',method:'torrent',format:g.files[0].path.split('.').at(-1).toUpperCase(),variant:g.variant+(g.files.length>1?' / '+g.files.length+' discs':''),size:g.files.reduce((n,f)=>n+f.length,0),fileCount:g.files.length});
  card.metadataOnly=false;if(!card.size)card.size=card.options.at(-1).size;
 }
}
cards.sort((a,b)=>a.title.localeCompare(b.title,'en'));
await fs.writeFile(out+'/catalog.json',JSON.stringify(cards));await fs.writeFile(out+'/catalog-full.json',JSON.stringify(full));await fs.writeFile(out+'/game-torrents.json',JSON.stringify({schema:1,indexedAt:new Date().toISOString(),collections,games}));
const assets=await Promise.all(['catalog.json','game-torrents.json'].map(async f=>({file:f,bytes:(await fs.stat(out+'/'+f)).size})));
console.log(JSON.stringify({cards:cards.length,torrentOptions:games.length,platformCounts:Object.fromEntries(['ps1','ps2'].map(p=>[p,cards.filter(c=>c.platform===p&&c.options.length).length])),collectionFiles:Object.values(collections).map(c=>({id:c.id,files:c.fileCount})),assets},null,2));
