// PSXS5 can read known files while sandboxed. Keep its explicit game index current.
// No helper whitelist or persistent jailbreak permissions are changed.
(function(root){'use strict';
const I=root.RaffIdentity||(typeof require==='function'?require('../shared/identity.js'):null);
const base='/data/PSXS5/games',marker='# PSXS5 library index - managed by Raff\n';
function entries(files,games){const ps1=games.filter(g=>g.platform==='ps1');return files.filter(f=>f.path?.startsWith(base+'/')&&!/[\t\r\n]/.test(f.path)&&!f.path.split('/').some(p=>p==='.'||p==='..')&&/\.(?:chd|cue|pbp|m3u|iso|img|ccd|mdf)$/i.test(f.name)).map(f=>{const card=ps1.find(g=>I.sameInstalled(g,f)),stem=f.name.replace(/\.[^.]+$/,'');return{path:f.path,stem,title:(card?.title||stem).replace(/[\t\r\n]/g,' '),cover:card?.cover||''};});}
class Sync{
 constructor(fs,queue){this.fs=fs;this.queue=queue;this.signature='';}
 tick(){if(this.queue.settings.paths?.ps1!==base)return;const files=this.queue.installed.ps1||[],signature=JSON.stringify(files.map(f=>[f.path,f.name]));if(signature===this.signature)return;const rows=entries(files,this.queue.discovery);
  const file='/data/PSXS5/library.txt',old=this.fs('stat',file);if(old&&!this.fs('read',file).startsWith(marker))return;
  const text=marker+'# title<TAB>serial<TAB>discs<TAB>path<TAB>first disc name\n'+rows.map(e=>[e.title,'',1,e.path,e.stem].join('\t')).join('\n')+'\n';
  this.fs('psxs5-index',file,text);
  for(const e of rows)if(/^[a-z0-9_-]{1,80}$/.test(e.cover))try{this.fs('psxs5-cover','/data/PSXS5/covers/'+e.stem+'.jpg',e.cover);}catch{}this.signature=signature;
 }
}
const api={Sync,entries,marker};if(typeof module!=='undefined')module.exports=api;else root.RaffPSXS5=api;
})(globalThis);
