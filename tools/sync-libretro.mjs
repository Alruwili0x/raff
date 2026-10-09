// Complete pinned-tree synchronization: checkpointed, resumable, bounded and hash verified.
import{mkdir,readFile,writeFile,rename}from'node:fs/promises';
import{createHash}from'node:crypto';import{resolve,join}from'node:path';
const root=resolve(process.env.RAFF_PROVIDER_CACHE||'work/providers');await mkdir(root,{recursive:true});
const manifestPath=join(root,'sync.json');let state={version:1,sources:{},files:{},failures:[]};try{state=JSON.parse(await readFile(manifestPath,'utf8'));}catch{}
async function checkpoint(){await writeFile(manifestPath+'.new',JSON.stringify(state,null,2));await rename(manifestPath+'.new',manifestPath);}
const blob=b=>createHash('sha1').update(Buffer.from('blob '+b.length+'\0')).update(b).digest('hex');
let nextRequest=0;async function limited(url){const wait=Math.max(0,nextRequest-Date.now());nextRequest=Date.now()+wait+250;await new Promise(r=>setTimeout(r,wait));return fetch(url,{signal:AbortSignal.timeout(120000),headers:{'User-Agent':'Raff-Metadata-Sync/0.9'}});}
for(const [repo,cache,folder,choose]of[['libretro-core-info','core-tree','cores',p=>p.endsWith('.info')||p==='COPYING'],['libretro-database','database-tree','rdb',p=>p.startsWith('rdb/')&&p.endsWith('.rdb')||p==='LICENSE']]){
 const tree=JSON.parse(await readFile(join(root,cache+'.json'),'utf8'));if(tree.truncated)throw Error('Incomplete tree');
 const files=tree.tree.filter(x=>x.type==='blob'&&choose(x.path));await mkdir(join(root,folder),{recursive:true});
 state.sources[repo]={commit:tree.sha,total:files.length,startedAt:new Date().toISOString(),completed:0};state.failures=state.failures.filter(x=>x.repo!==repo);await checkpoint();
 for(const f of files){
  const filename=f.path.split('/').at(-1);if(/[\\/:]/.test(filename)||filename==='..')throw Error('Unsafe name');
  const path=join(root,folder,filename),key=repo+'/'+f.path;let existing;try{existing=await readFile(path);}catch{}
  if(existing&&blob(existing)===f.sha){state.sources[repo].completed++;continue;}
  let error;for(let attempt=0;attempt<4;attempt++){
   try{const r=await limited('https://raw.githubusercontent.com/libretro/'+repo+'/'+tree.sha+'/'+f.path.split('/').map(encodeURIComponent).join('/'));
    if(!r.ok){const retry=r.headers.get('retry-after');if([429,503].includes(r.status)&&retry){const seconds=Number(retry);await new Promise(q=>setTimeout(q,Math.min(60000,Number.isFinite(seconds)?seconds*1000:Math.max(0,Date.parse(retry)-Date.now()))));}throw Error('HTTP '+r.status);}
    const b=Buffer.from(await r.arrayBuffer());if(b.length!==f.size||blob(b)!==f.sha)throw Error('Git blob identity mismatch');
    await writeFile(path+'.part',b);await rename(path+'.part',path);state.files[key]={blob:f.sha,bytes:b.length,sha256:createHash('sha256').update(b).digest('hex'),syncedAt:new Date().toISOString()};error=null;break;
   }catch(e){error=e.message;await new Promise(q=>setTimeout(q,500*2**attempt));}
  }
  if(error)state.failures.push({repo,path:f.path,error});else state.sources[repo].completed++;
  await checkpoint();if(state.sources[repo].completed%20===0)console.log(repo,state.sources[repo].completed+'/'+files.length);
 }
 state.sources[repo].finishedAt=new Date().toISOString();await checkpoint();console.log(repo,'complete',state.sources[repo]);
}
if(state.failures.length){console.error(state.failures);process.exitCode=1;}
