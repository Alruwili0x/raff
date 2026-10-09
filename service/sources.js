// Read-only downloads index. Large platform catalogs stay on disk, not in the JS heap.
(function(root){'use strict';
const I=root.RaffIdentity||(typeof require==='function'?require('../shared/identity.js'):null);
class Sources{
 constructor(sql){this.sql=sql;}
 rows(q,p=[]){return this.sql(q,p).rows;}
 value(table,id){if(!id)return null;const r=this.rows('SELECT value FROM downloads.'+table+' WHERE id=?',[id])[0];return r?JSON.parse(r.value):null;}
 direct(id){return this.value('directs',id);}
 torrent(id){return this.value('torrents',id);}
 collection(id){return this.value('collections',id);}
 art(id){const r=this.rows('SELECT value FROM downloads.cards WHERE artid=? LIMIT 1',[id])[0];return r?JSON.parse(r.value):null;}
 card(id){return this.value('cards',id);}
 installed(inventory){const keys=new Set();for(const[p,entries]of Object.entries(inventory))for(const e of entries){const k=I.titleParts(e.name||e.filename||e.title),compact=k.key.replace(/\s+/g,'');for(const r of this.rows('SELECT id FROM downloads.cards WHERE platform=? AND compact=? UNION SELECT cardid AS id FROM downloads.aliases WHERE platform=? AND compact=? LIMIT 128',[p,compact,p,compact]))keys.add(r.id);}return [...keys];}
 selected(v,queue){const keys=v.section==='favorites'?queue.settings.favorites:queue.status().installedKeys;const offset=Math.max(0,Math.min(10000,Math.floor(Number(v.offset)||0))),list=[...new Set((keys||[]).filter(k=>typeof k==='string'&&!k.startsWith('hub:')))];const selected=list.slice(offset,offset+200),items=[];for(const k of selected){const c=this.card(k);if(c)items.push(c);}return{section:v.section,offset,next:offset+200<list.length?offset+200:null,items};}
}
const api={Sources};if(typeof module!=='undefined')module.exports=api;else root.RaffSources=api;
})(globalThis);
