import test from'node:test';import assert from'node:assert/strict';import fs from'node:fs';
import T from'../service/transport.js';import Q from'../service/queue.js';import L from'../shared/core.js';
const large=(id,host='same.example')=>({id,game:{url:'https://'+host+'/'+id+'.pkg',sizeBytes:4e9}});
test('new downloads share connections and metadata uses only one stream',()=>{
 const jobs=['a','b','c','d'].map(id=>large(id)),q={jobs,engines:new Map(jobs.map(j=>[j.id,{running:true}])),settings:{connections:16}};
 assert.deepEqual(jobs.map(j=>T.plan(q,j,j.game.url,'a.pkg').connections),[4,4,4,4]);
 assert.equal(T.plan(q,jobs[0],jobs[0].game.url,'source.html').connections,1);
 q.settings.connections=2;assert.equal(T.plan(q,jobs[0],jobs[0].game.url,'a.pkg').connections,2);
 q.jobs=[jobs[0]];q.engines.clear();q.settings.connections=16;assert.equal(T.plan(q,jobs[0],jobs[0].game.url,'a.pkg').connections,8);
});
test('real queue attachment passes the planned stream count to aria2 without changing partial-file options',async()=>{
 const calls=[],g={id:'test',title:'One',platform:'ps4',filename:'One.pkg',url:'https://same.example/one.pkg',sizeBytes:4e9};
 const c=new L.Client({wfm:'http://127.0.0.1:8888',aria:'http://127.0.0.1:6800'});c.rpc=async(m,p)=>{calls.push({m,p});return'gid'};
 const q=new Q.Queue({catalog:{games:[g]},read:()=>'',write:()=>{},clientFactory:()=>c});const j={id:'one',game:g};q.jobs=[j];q.attach(j);
 await c.add(g.url,'/data/raff/ps4-staging/one',g.filename,'gid');const options=calls[0].p[1];
 assert.equal(options.split,'8');assert.equal(options['max-connection-per-server'],'8');assert.equal(options['min-split-size'],'1M');assert.equal(options.continue,'true');assert.equal(options['allow-overwrite'],'false');assert.equal(j.transferConnections,8);
 assert.ok(!calls.some(x=>x.m==='changeOption'),'No live stream restart');
});
test('catalog associations do not duplicate choices or erase existing games',()=>{
 const cards=JSON.parse(fs.readFileSync(new URL('../assets/catalog.json',import.meta.url))),full=JSON.parse(fs.readFileSync(new URL('../assets/catalog-full.json',import.meta.url))).games;
 assert.equal(new Set(cards.map(c=>c.key)).size,cards.length);assert.equal(new Set(full.map(g=>g.id)).size,full.length);
 const options=cards.flatMap(c=>c.options.filter(o=>o.method!=='torrent').map(o=>o.id));assert.equal(options.length,new Set(options).size);assert.deepEqual(new Set(options),new Set(full.map(g=>g.id)));
 const gta=cards.find(c=>c.key==='xbox360:gta 4');assert.ok(gta?.options.length);assert.ok(full.some(g=>g.url==='https://pixeldrain.com/api/file/vMgbK2Js'));
 const shared=full.filter(g=>g.catalogs?.includes('blackbox')&&g.catalogs?.includes('orbit'));assert.equal(shared.length,147);
});
