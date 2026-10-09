(function(root){
'use strict';
const sleep=n=>new Promise(r=>setTimeout(r,n));
const strong=v=>typeof v==='string'&&/^"[^\x00-\x1f\x7f]{1,220}"$/.test(v);
function retryDelay(value,now=Date.now()){if(!value)return 0;const n=/^\d+$/.test(String(value))?Number(value)*1000:Date.parse(value)-now;return Math.max(0,Math.min(86400000,Number.isFinite(n)?n:0));}
function sameFile(before,after,checksum){if(!before||!after)return false;if(before.total>0&&after.total>0&&before.total!==after.total)return false;if(strong(before.etag)&&strong(after.etag))return before.etag===after.etag;return !!checksum;}
class Policy{
 constructor(queue,probe=root.nativeProbe){this.queue=queue;this.native=probe;this.serial=Promise.resolve();this.samples=new Map();}
 inspect(url){if(!this.native)return Promise.resolve({range:false,status:0,etag:'',total:0,error:'Range probe unavailable'});const operation=this.serial.then(async()=>{const id=this.native(url);for(let n=0;n<200;n++){await sleep(100);const result=this.native('poll');if(result.id===id&&result.complete)return result;}throw Error('Remote identity probe timed out');});this.serial=operation.catch(()=>{});return operation;}
 capacity(j){const q=this.queue,host=new URL(j.game.url).hostname;let global=0,local=0;for(const other of q.jobs){if(other.id===j.id||!q.engines.get(other.id)?.running||!other.game.url)continue;const n=other.transfer?.connections||1;global+=n;if(new URL(other.game.url).hostname===host)local+=n;}return Math.max(0,Math.min(4,8-global,4-local,q.settings.connections));}
 async prepare(j,url,checksum){const previous=j.transfer?.identity;const result=await this.inspect(url);
  if(result.status===429||result.status===503){const wait=retryDelay(result.retryAfter)||30000;j.retryAt=Date.now()+wait;throw Error('Source rate limit. Retry after '+new Date(j.retryAt).toISOString());}
  if(result.error)throw Error(result.error);if(result.status>=400&&result.status!==416)throw Error('HTTP '+result.status+' while checking source');
  if((j.received>0||j.partialPresent||(j.recovered&&previous))&&!sameFile(previous,result,checksum))throw Error('Remote file identity cannot be confirmed. Partial file kept; use Restart for a fresh copy.');
  j.transfer={identity:result,connections:1,rangeVerified:!!result.range,resumeSupported:!!result.range&&(strong(result.etag)||!!checksum),stage:'baseline',url};j.recovered=false;this.queue.persist();
  return{connections:1,headers:strong(result.etag)?['If-Match: '+result.etag]:[],alwaysResume:!!result.range};
 }
 async resume(j){if(!j.transfer?.identity||!j.transfer.resumeSupported)throw Error('Safe resume is unavailable for this source. Partial file kept; use Restart.');const after=await this.inspect(j.game.url);if(!sameFile(j.transfer.identity,after,j.game.checksum))throw Error('Remote file changed. Partial file kept; use Restart.');if(after.status===429){j.retryAt=Date.now()+(retryDelay(after.retryAfter)||30000);throw Error('Source rate limit. Retry later.');}}
 observe(j){if(j.phase!=='downloading'||!j.transfer?.rangeVerified||j.speed<=0)return;let sample=this.samples.get(j.id);if(!sample){sample={since:Date.now(),speeds:[],baseline:0,pending:false};this.samples.set(j.id,sample);}if(sample.pending)return;sample.speeds.push(j.speed);if(sample.speeds.length>30)sample.speeds.shift();if(Date.now()-sample.since<15000||sample.speeds.length<8)return;
  const sorted=[...sample.speeds].sort((a,b)=>a-b),rate=sorted[Math.floor(sorted.length/2)],current=j.transfer.connections||1,cap=this.capacity(j);let next=current;
  if(current>1&&sample.baseline&&rate<sample.baseline*1.1){next=Math.max(1,current/2);j.transfer.stage='settled';}
  else if(j.transfer.stage!=='settled'&&current<cap){next=Math.min(cap,current*2);sample.baseline=rate;j.transfer.stage='measuring';}
  else j.transfer.stage='settled';
  sample.since=Date.now();sample.speeds=[];if(next===current)return;sample.pending=true;
  this.queue.clientFactory().rpc('changeOption',[j.gid,{split:String(next),'max-connection-per-server':String(next)}]).then(()=>{j.transfer.connections=next;j.transfer.measuredBytesPerSecond=rate;this.queue.persist();}).catch(()=>{j.transfer.stage='settled';}).finally(()=>sample.pending=false);
 }
}
// Plan new streams only. Updating aria2 connection options on an active GID
// restarts its HTTP requests; do not repeatedly restart a download to tune it.
function plan(queue,job,url,name){
 if(name==='source.html')return{connections:1,metadata:true};
 const host=new URL(url).hostname;
 const running=queue.jobs.filter(j=>j.id===job.id||queue.engines.get(j.id)?.running);
 const peers=running.filter(j=>{try{return new URL(j.game.url||j.game.sourcePage).hostname===host}catch{return false}});
 const limit=Math.min(16,Math.max(1,Number(queue.settings.connections)||16));
 const small=job.game.sizeBytes>0&&job.game.sizeBytes<8*1024*1024;
 const connections=small?1:Math.max(1,Math.min(limit,8,Math.floor(24/Math.max(1,running.length)),Math.floor(16/Math.max(1,peers.length))));
 return{connections,metadata:false};
}
const api={Policy,sameFile,strong,retryDelay,plan};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.RaffTransport=api;
})(globalThis);
