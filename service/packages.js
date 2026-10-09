/* PKG post-processing. SPDX-License-Identifier: GPL-3.0-or-later */
(function(root){
'use strict';
const L=root.PS2Library||(typeof require==='function'?require('../shared/core.js'):null);
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
const hex=b=>Array.from(b,x=>x.toString(16).padStart(2,'0')).join('');
function verifiedCopy(receipt,state){
 if(!state||state.size!==receipt.size||state.header!==receipt.header)return false;
 let meta;try{meta=JSON.parse(state.metadata);}catch(_){return false;}
 if(Number(meta.originalFileSize)!==receipt.size)return false;
 // PlayGo locus 3 means the chunk is completely present in local storage.
 // A registered title, sparse app.pkg, or "ready to play" alone is insufficient.
 const xml=String(state.playgo||''),count=xml.match(/<chunk_status\b[^>]*\bchunk_count="(\d+)"/),chunks=xml.match(/<chunk\s[^>]*\/\s*>/g)||[];
 if(!count||Number(count[1])!==chunks.length||!chunks.length||!xml.includes('</chunk_status>'))return false;
 const ids=new Set();for(const chunk of chunks){const id=chunk.match(/\bid="(\d+)"/),locus=chunk.match(/(?:^|\s)locus="(\d+)"/);if(!id||!locus||locus[1]!=='3'||ids.has(id[1]))return false;ids.add(id[1]);}
 return true;
}
class Packages{
 constructor(client,{state,clock=()=>Date.now(),wait=sleep}={}){this.c=client;this.state=state||((id)=>root.nativePackageFiles(id));this.clock=clock;this.wait=wait;}
 async entry(path){return(await this.c.list(path.slice(0,path.lastIndexOf('/')))).find(e=>e.path===path);}
 async proof(r){return verifiedCopy(r,await this.state(r.titleId));}
 async run(j,persist,update,stopped){
  const c=this.c,dest=L.destinationFor(j.game);j.packageReceipts=j.packageReceipts||[];j.packageIndex=j.packageIndex||0;
  const files=j.files.filter(f=>/\.pkg$/i.test(f.name));
  while(!stopped()){
   const file=files[j.packageIndex];if(!file){j.phase='done';j.progress=1;persist();return;}
   const path=dest+'/'+file.name;
   if(!j.moved.includes(path)||!L.inside(path,dest)||!L.inside(file.path,j.dir))throw Error('PKG ownership mismatch');
   let r=j.packageReceipts[j.packageIndex];
   if(j.phase==='pkg-ready'){
    const e=await this.entry(path);if(!e||e.type!=='-'||Number(e.size)!==Number(file.size))throw Error('PKG missing or changed; retained / ملف PKG تغير');
    const bytes=await c.bytes(path,4096);L.verifyHeader(file.name,bytes,e.size);
    const info=await c.api('pkg-info',{path}),field=name=>(info.fields||[]).find(f=>f.name===name)?.value;
    const id=field('TITLE_ID')||String(info.content_id||'').match(/(?:CUSA|PPSA)\d{5}/)?.[0];
    if(!/^(?:CUSA|PPSA)\d{5}$/.test(id||'')||j.game.titleId&&id!==j.game.titleId)throw Error('PKG title does not match the game / معرّف الحزمة لا يطابق اللعبة');
    // Record the exact source header and size before submitting; the system may move it.
    r={path,titleId:id,contentId:info.content_id,size:Number(e.size),mtime:e.mtime,header:hex(bytes),category:field('CATEGORY')||'',startedAt:this.clock()};
    if(bytes.length!==4096)throw Error('Incomplete PKG header');
    j.packageReceipts[j.packageIndex]=r;
    if(await this.proof(r)){r.confirmedAt=this.clock();r.alreadyInstalled=true;j.phase='pkg-cleanup';persist();continue;}
    if(await c.available('/data')<r.size+256*1048576)throw Error('Not enough space to install; PKG kept / المساحة لا تكفي للتثبيت');
    j.phase='pkg-submit';persist();
    const result=await c.api('install-pkg',{paths:path},true);
    if(!Array.isArray(result.task_ids)||result.task_ids.length!==1||!Number.isInteger(result.task_ids[0]))throw Error('Installer response uncertain; PKG kept');
    r.taskId=result.task_ids[0];j.phase='pkg-installing';j.progress=0;persist();
   }else if(j.phase==='pkg-submit'){
    // Lost acknowledgement: only adopt the exact task from this submission window.
    const tasks=(await c.api('tasks')).tasks||[],t=tasks.find(t=>t.op==='pkg_install'&&t.src===path&&Number(t.created_at)*1000>=r.startedAt-1000);
    if(!t)throw Error('Check the PS5 installation queue; submission was interrupted. PKG kept / راجع قائمة تثبيت السوني');
    r.taskId=t.id;j.phase='pkg-installing';persist();
   }else if(j.phase==='pkg-installing'){
    const t=((await c.api('tasks')).tasks||[]).find(t=>t.id===r.taskId);
    if(t&&(t.op!=='pkg_install'||t.src!==path))throw Error('Installer task mismatch; PKG kept');
    if(t&&['failed','canceled'].includes(t.state)){
     j.phase='pkg-ready';persist();throw Error('Installation failed; PKG kept: '+(t.error_arg||t.error||t.state));
    }
    // WFM "done" means submission succeeded, not that installation has finished.
    if((!t||t.state==='done')&&await this.proof(r)){r.confirmedAt=this.clock();j.phase='pkg-cleanup';persist();continue;}
    if(this.clock()-r.startedAt>2*3600000)throw Error('Installation could not be confirmed; PKG kept / تعذر تأكيد التثبيت، الملف محفوظ');
    update();await this.wait(1500);
   }else if(j.phase==='pkg-cleanup'||j.phase==='pkg-deleting'){
    if(!r.confirmedAt||!await this.proof(r))throw Error('Installed copy is not fully verified; PKG kept');
    if(!j.installOptions.remove){r.retained=true;}else{
     const e=await this.entry(path);
     if(e){
      if(e.type!=='-'||Number(e.size)!==r.size||r.mtime!==undefined&&e.mtime!==r.mtime||hex(await c.bytes(path,4096))!==r.header)throw Error('PKG changed after installation; retained');
      if(j.phase==='pkg-deleting'){
       const t=((await c.api('tasks')).tasks||[]).find(t=>t.id===r.deleteTaskId);
       if(t&&(t.op!=='delete'||t.src!==path))throw Error('Cleanup task mismatch');
       if(t&&['queued','running'].includes(t.state)){await this.wait(250);continue;}
       throw Error('Cleanup incomplete; PKG retained');
      }
      if(stopped())return;
      j.phase='pkg-deleting';persist();
      const result=await c.api('delete',{paths:path},true);r.deleteTaskId=result.task_id;persist();await this.wait(250);continue;
     }
     r.removedAt=this.clock();
    }
    j.packageIndex++;j.phase='pkg-ready';persist();
   }else throw Error('Unknown package phase');
  }
 }
}
const api={Packages,verifiedCopy};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.RaffPackages=api;
})(globalThis);
