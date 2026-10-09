import{open,readdir,lstat,mkdir,readFile,writeFile}from'node:fs/promises';import{createHash}from'node:crypto';import{resolve,dirname}from'node:path';import{fileURLToPath}from'node:url';
const allowedFiles=new Set(['eboot.bin','raff-service.elf','raff-updater.elf','raff-owner.txt','LICENSE','README.md','THIRD_PARTY_NOTICES.md']);
export function allowedPath(p){return typeof p==='string'&&p.length<=240&&!/[\x00-\x1f\x7f\\:]/.test(p)&&p.split('/').every(s=>s&&!s.startsWith('.'))&&(allowedFiles.has(p)||/^(assets|sce_sys|sce_module|runtime|licenses|docs)\//.test(p));}
export async function bundle(app,dest){
 const files=[];async function walk(dir,prefix=''){for(const e of await readdir(dir,{withFileTypes:true})){const p=prefix+e.name,s=await lstat(resolve(app,p));if(s.isSymbolicLink())throw Error('Symlink rejected: '+p);if(s.isDirectory())await walk(resolve(app,p),p+'/');else{if(!allowedPath(p))throw Error('Unapproved release file: '+p);if(s.size)files.push({p,size:s.size});}}}await walk(app);
 // Artwork is retained from the installed app, including its on-demand cache.
 for(let n=files.length-1;n>=0;n--)if(/^assets\/(covers|media)\//.test(files[n].p))files.splice(n,1);
 files.sort((a,b)=>Buffer.compare(Buffer.from(a.p),Buffer.from(b.p)));if(files.length>50000)throw Error('Too many release files');
 for(const p of['eboot.bin','raff-service.elf','raff-updater.elf','raff-owner.txt','sce_sys/param.json','sce_module/libc.prx','assets/update-config.json','assets/catalog.json'])if(!files.some(f=>f.p===p))throw Error('Missing '+p);
 const param=JSON.parse(await readFile(resolve(app,'sce_sys/param.json'),'utf8'));if(param.titleId!=='PPSA99178')throw Error('Wrong application title');
 await mkdir(dirname(dest),{recursive:true});const target=await open(dest,'wx');const hash=createHash('sha256');let bytes=0;
 async function write(b){let at=0;while(at<b.length){const r=await target.write(b,at,b.length-at);if(!r.bytesWritten)throw Error('Short release write');at+=r.bytesWritten;}hash.update(b);bytes+=b.length;if(bytes>2*1024**3)throw Error('Package exceeds update limit');}
 try{const header=Buffer.alloc(12);header.write('RAFFUPD1');header.writeUInt32LE(files.length,8);await write(header);
  for(const f of files){const b=await readFile(resolve(app,f.p)),name=Buffer.from(f.p);const h=Buffer.alloc(42);h.writeUInt16LE(name.length);h.writeBigUInt64LE(BigInt(b.length),2);createHash('sha256').update(b).digest().copy(h,10);await write(h);await write(name);await write(b);}await target.sync();
 }finally{await target.close();}return{files:files.length,bytes,sha256:hash.digest('hex')};
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url)){const[app,dest]=process.argv.slice(2);if(!app||!dest)throw Error('Usage: node tools/release-bundle.mjs APP_DIRECTORY OUTPUT.raffupdate');const result=await bundle(resolve(app),resolve(dest));await writeFile(dest+'.sha256',result.sha256+'  '+dest.split(/[\\/]/).pop()+'\n');console.log(JSON.stringify(result));}
