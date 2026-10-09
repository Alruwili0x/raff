/* SPDX-License-Identifier: GPL-3.0-or-later
 * Release checks and downloads use the existing local aria2 worker. No HTTP
 * requests or archive extraction run on the UI thread. */
(function(root){
'use strict';
const BASE='/data/raff/native-v5/updates', APP='/data/homebrew/PPSA99178';
const VERSION='1.1.1', INTERVAL=6*60*60*1000;
function version(s){const m=/^v?(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$/.exec(s||'');if(!m||m.slice(1).some(v=>Number(v)>99999))throw Error('Invalid release version');return m.slice(1).map(Number);}
function newer(a,b){a=version(a);b=version(b);for(let n=0;n<3;n++)if(a[n]!==b[n])return a[n]>b[n];return false;}
function repository(s){if(!/^[A-Za-z0-9][A-Za-z0-9-]{0,38}\/[A-Za-z0-9_.-]{1,100}$/.test(s||'')||s.split('/')[1]==='..')throw Error('Update repository is not configured');return s;}
function release(data,repo,current=VERSION){
 repository(repo);if(data.draft||data.prerelease||!newer(data.tag_name,current))return null;
 const tag=data.tag_name, asset=(data.assets||[]).find(a=>a.name==='Raff-'+tag+'.raffupdate');
 if(!asset||asset.state!=='uploaded'||!Number.isSafeInteger(asset.size)||asset.size<1024||asset.size>2*1024**3||!/^sha256:[a-f0-9]{64}$/.test(asset.digest||''))throw Error('Release package or SHA-256 digest missing');
 const url='https://github.com/'+repo+'/releases/download/'+tag+'/'+asset.name;
 if(asset.browser_download_url!==url)throw Error('Release asset does not belong to the configured repository');
 return{version:tag.replace(/^v/,''),tag,url,size:asset.size,sha256:asset.digest.slice(7),notes:String(data.body||'').slice(0,2000)};
}
class Updates{
 constructor({config,client,fs,native,read,write,id,clock=()=>Date.now(),busy=()=>false}){
  Object.assign(this,{config,client,fs,native,read,write,id,clock,busy});this.checking=false;this.working=false;this.lastManual=0;
  try{this.saved=JSON.parse(read('updates')||'{}');}catch{this.saved={};}
  this.state={phase:config.repository?'idle':'unconfigured',current:VERSION,repository:config.repository||'',received:0,total:0};
  // Jobs from a previous service session are reattached to their exact GID.
  if(this.saved.repository===config.repository){try{const stored=this.saved.state||{};if(stored.release)version(stored.release.version);Object.assign(this.state,stored);this.state.current=VERSION;if(this.state.release&&!newer(this.state.release.version,VERSION))this.state={...this.state,phase:'current',release:null};}catch{this.saved={};}}
 }
 save(){this.saved.repository=this.config.repository;this.saved.state=this.state;this.write('updates',JSON.stringify(this.saved));}
 status(){return {...this.state,nextCheck:this.saved.nextCheck||0};}
 async check(manual=false){
  repository(this.config.repository);if(this.checking||['checking','downloading','verifying','ready','applying'].includes(this.state.phase))return this.status();
  if(manual&&this.clock()-this.lastManual<60000&&this.lastManual)return this.status();
  if(!manual&&this.clock()<(this.saved.nextCheck||0))return this.status();
  this.checking=true;if(manual)this.lastManual=this.clock();
  try{await this.stopPrevious();await this.client.ensureDir(BASE);const gid=this.id();this.state={...this.state,phase:'checking',error:'',gid,started:this.clock(),kind:'release'};this.saved.nextCheck=this.clock()+INTERVAL+Math.floor(Math.random()*900000);this.save();
   await this.add('https://api.github.com/repos/'+this.config.repository+'/releases/latest','release-'+gid+'.json',gid,1048576);
  }catch(e){this.fail(e);}finally{this.checking=false;}return this.status();
 }
 async add(url,name,gid,size,sha){
  const options={dir:BASE,out:name,gid,'auto-file-renaming':'false','allow-overwrite':'false','continue':'true','max-connection-per-server':'2',split:'2','max-tries':'3','retry-wait':'5',timeout:'25','connect-timeout':'10','check-certificate':'true','ca-certificate':APP+'/runtime/ca-bundle.crt','file-allocation':'none','max-file-not-found':'1','max-download-limit':sha?'0':'256K',header:['Accept: application/vnd.github+json','User-Agent: Raff/'+VERSION]};
  if(sha)options.checksum='sha-256='+sha;
  await this.client.rpc('addUri',[[url],options]);
 }
 fail(e){this.state.phase='error';this.state.error=String(e.message||e).slice(0,180);this.saved.nextCheck=this.clock()+Math.max(900000,Math.min(INTERVAL,(this.saved.backoff||450000)*2));this.saved.backoff=this.saved.nextCheck-this.clock();this.save();}
 async stopPrevious(){
  if(!this.state.gid||this.state.phase!=='error')return;
  let s;try{s=await this.client.rpc('tellStatus',[this.state.gid]);}catch(e){if(/not found/i.test(e.message))return;throw e;}
  if(['complete','error','removed'].includes(s.status))return;
  const name=this.state.kind==='package'?'package-'+this.state.gid+'.raffupdate':'release-'+this.state.gid+'.json';
  if(!Array.isArray(s.files)||s.files.length!==1||s.files[0].path!==BASE+'/'+name)throw Error('Previous update transfer identity could not be confirmed');
  await this.client.rpc('forceRemove',[this.state.gid]);
 }
 async download(){
  if(!this.state.release||!['available','error'].includes(this.state.phase))throw Error('Check for an update first');
  const r=this.state.release;if(this.fs('space',BASE)<r.size*3+128*1048576)throw Error('Not enough free space for update and rollback');
  await this.stopPrevious();const gid=this.id();this.state={...this.state,phase:'downloading',error:'',gid,started:this.clock(),kind:'package',received:0,total:r.size};this.save();
  try{await this.add(r.url,'package-'+gid+'.raffupdate',gid,r.size,r.sha256);}catch(e){this.fail(e);}return this.status();
 }
 async tick(){
  if(this.working||!this.config.repository)return;this.working=true;
  try{
   if(['checking','downloading'].includes(this.state.phase)){
    const checking=this.state.phase==='checking',s=await this.client.rpc('tellStatus',[this.state.gid]);
    const limit=checking?1048576:this.state.release.size;
    if(Number(s.totalLength)>limit||Number(s.completedLength)>limit){await this.client.rpc('forceRemove',[this.state.gid]);throw Error('Update response exceeds its declared size');}
    this.state.received=Number(s.completedLength)||0;this.state.total=Number(s.totalLength)||limit;
    if(s.status==='error'||s.status==='removed')throw Error('Update download failed: '+(s.errorCode||s.status));
    if(s.status==='complete'){
     if(checking){const file=BASE+'/release-'+this.state.gid+'.json';this.state.release=release(JSON.parse(this.fs('read',file)),this.config.repository);this.state.phase=this.state.release?'available':'current';this.saved.backoff=0;this.save();}
     else{if(Number(s.totalLength)!==limit)throw Error('Incomplete update package');this.native('verify',{gid:this.state.gid,sha256:this.state.release.sha256});this.state.phase='verifying';this.save();}
    }else if(this.clock()-this.state.started>(checking?120000:24*60*60*1000))throw Error('Update timed out; try again later');
   }else if(this.state.phase==='verifying'){
    const n=this.native('status',{});this.state.verified=n.received||0;if(n.phase==='ready'){this.state.phase='ready';this.save();}else if(n.phase==='error')throw Error(n.error);
   }else if(['idle','current','error'].includes(this.state.phase))await this.check();
  }catch(e){this.fail(e);}finally{this.working=false;}
 }
 async apply(){if(this.state.phase!=='ready')throw Error('The update has not been verified');this.state.phase='preparing';try{if(await this.busy())throw Error('Pause downloads and wait for file operations to finish');this.native('apply',{});this.state.phase='applying';this.save();return this.status();}catch(e){this.state.phase='ready';throw e;}}
}
const api={Updates,release,repository,version,newer,VERSION};if(typeof module!=='undefined')module.exports=api;else root.RaffUpdates=api;
})(globalThis);
