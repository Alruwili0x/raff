import{tmpdir}from'node:os';
import test from 'node:test';import assert from 'node:assert/strict';
import{spawnSync}from'node:child_process';import{resolve}from'node:path';
import {DatabaseSync} from 'node:sqlite';import{readFileSync}from'node:fs';import{createRequire}from'node:module';
const require=createRequire(import.meta.url);const {Library,cue,playlist,path,fts}=require('../service/library.js');
const registry=JSON.parse(readFileSync(new URL('../assets/core-registry.json',import.meta.url)));
const coreSource=readFileSync(new URL('../service/database.c',import.meta.url),'utf8');
const schema=coreSource.split('const char *schema=')[1].split('\n  rc=')[0];
const decoded=[...schema.matchAll(/"((?:[^"\\]|\\.)*)"/g)].map(m=>JSON.parse('"'+m[1]+'"')).join('');
function setup(){
 const userDb=resolve(tmpdir(),'raff-test-'+Date.now()+'-'+Math.random().toString(16).slice(2)+'.sqlite');const db=new DatabaseSync(userDb);db.exec(decoded.split('ATTACH DATABASE')[0]);
 const catalog=new URL('../assets/library.sqlite',import.meta.url).pathname.replace(/^\/([A-Za-z]:)/,'$1');db.prepare('ATTACH DATABASE ? AS catalog').run(decodeURIComponent(catalog));
 const entries=new Map(),writes=new Map();
 const fs=(op,p,v)=>{if(op==='stat')return entries.get(p)?{type:'file',size:entries.get(p).length||1,mtime:10}:null;if(op==='read'){if(!entries.has(p))throw Error('missing');return entries.get(p);}if(op==='list')throw Error('No RetroArch installed on this test target');if(op==='export'){if(writes.has(p))throw Error('exists');writes.set(p,v);return p;}throw Error(op);};
 const sql=(q,params)=>{if(q.includes('MATCH')){const code="import sqlite3,json,sys;v=json.load(sys.stdin);d=sqlite3.connect(v['user']);d.row_factory=sqlite3.Row;d.execute('ATTACH DATABASE ? AS catalog',(v['catalog'],));print(json.dumps({'rows':[dict(r) for r in d.execute(v['q'],v['params'])]}))";const r=spawnSync(process.env.RAFF_PYTHON||'python',['-c',code],{input:JSON.stringify({user:userDb,catalog:decodeURIComponent(catalog),q,params}),encoding:'utf8'});if(r.status)throw Error(r.stderr);return JSON.parse(r.stdout);}const stmt=db.prepare(q);return {rows:stmt.all(...params)};};
 const settings={favorites:[]};const lib=new Library({sql,fs,registry,settings,saveSettings(){}});lib.init();return{lib,db,entries,writes};
}
test('CUE validates Arabic paths, spaces and every referenced track',()=>{
 const text='FILE "المسار الأول.bin" BINARY\n TRACK 01 MODE2/2352\nFILE "Track 02.bin" BINARY\n TRACK 02 AUDIO';
 const found=new Set(['/data/ألعاب/المسار الأول.bin','/data/ألعاب/Track 02.bin']);const read=p=>found.has(p)?{type:'file',size:2352}:null;
 assert.equal(cue(text,'/data/ألعاب/Game.cue',read).tracks.length,2);
 found.delete('/data/ألعاب/Track 02.bin');assert.throws(()=>cue(text,'/data/ألعاب/Game.cue',read),/Missing CUE track/);
 assert.throws(()=>cue('FILE "../outside.bin" BINARY\n TRACK 01 MODE1/2352','/data/roms/a.cue',read),/escapes/);
});
test('M3U keeps disc order, deduplicates and detects missing discs',()=>{const stat=p=>p.endsWith('2.chd')?null:{type:'file',size:5};assert.deepEqual(playlist('Disc 1.chd\nDisc 1.chd','/data/roms/Game.m3u',stat).files,['/data/roms/Disc 1.chd']);assert.throws(()=>playlist('Disc 1.chd\nDisc 2.chd','/data/roms/Game.m3u',stat),/Missing M3U/);});
test('untrusted paths and search query operators are contained',()=>{for(const p of ['/data/../user/app','/user/app','/data/a\0b','C:\\games'])assert.throws(()=>path(p));assert.equal(fts('" OR *'), '"OR"*');});
test('real catalog supports bounded search, pagination, details and explicit target availability',()=>{
 const{lib,db}=setup();try{const a=lib.browse({platform:'sony-playstation',query:'Final Fantasy',limit:12});assert.ok(a.total>10);assert.ok(a.items.length<=12);const b=lib.browse({platform:'sony-playstation',query:'Final Fantasy',offset:12,limit:12});assert.ok(!b.items.some(x=>a.items.some(y=>y.id===x.id)));const d=lib.detail(a.items[0].id);assert.ok(d.releases.length);assert.equal(d.compatibility.status,'unsupported');assert.equal(d.compatibility.localLaunch,'not-tested');assert.deepEqual(new Set(lib.platforms().map(p=>p.id)),new Set(['ps2','ps4','ps5','switch','sony-playstation','sony-playstation-2']));}finally{db.close();}
});
test('offline import, duplicate prevention, favorite and real playlist export preserve files',()=>{
 const{lib,db,entries,writes}=setup();try{const file='/data/ألعاب/Game 1.chd';entries.set(file,'example test content');const first=lib.importFile({path:file,platform:'sony-playstation'});assert.equal(first.duplicate,false);assert.equal(lib.importFile({path:file,platform:'sony-playstation'}).duplicate,true);assert.equal(lib.local().total,1);const g=lib.browse({platform:'sony-playstation',limit:12}).items[0];lib.favorite(g.id,true);assert.equal(lib.browse({scope:'favorites'}).total,1);const result=lib.exportPlaylist({installationId:first.id});assert.equal(result.launchAttempted,false);assert.equal(JSON.parse(writes.get(result.path)).items[0].path,file);lib.remove(first.id);assert.equal(lib.local().total,0);assert.ok(entries.has(file));}finally{db.close();}
});
test('unsupported cores cannot be silently launched or selected for export',()=>{const{lib,db,entries}=setup();try{assert.throws(()=>lib.configureTarget({launchMode:'shell'}),/audited launch bridge/);entries.set('/data/a.chd','content');const r=lib.importFile({path:'/data/a.chd',platform:'sony-playstation'});assert.throws(()=>lib.exportPlaylist({installationId:r.id,core:'swanstation_libretro'}),/not installed/);assert.equal(lib.firmware('swanstation_libretro','ps5-local').status,'unsupported');}finally{db.close();}});

test('classic browsing position round-trips without replacing other UI state',()=>{
 const {lib,db}=setup();
 const existing={platform:'ps2',query:'original'};lib.view({id:'main',value:existing});
 const value={tab:4,sort:0,filter:1,query:'',key:'ps1:tekken 3'};
 lib.view({id:'classic-v082',value});
 assert.deepEqual(lib.view({id:'classic-v082'}),value);
 assert.deepEqual(lib.view({id:'main'}),existing);
 assert.throws(()=>lib.view({id:'classic-v082',value:{query:'x'.repeat(9000)}}),/large/);
 assert.deepEqual(lib.view({id:'classic-v082'}),value);db.close();
});
