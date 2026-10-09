import{readFile,writeFile,copyFile,readdir,mkdir,lstat,access,unlink}from'node:fs/promises';import{resolve,dirname}from'node:path';import{spawnSync}from'node:child_process';import{createHash}from'node:crypto';import{tc,root}from'./toolchain.mjs';import{bundle}from'./release-bundle.mjs';
const version='1.1.1',draft=process.argv.includes('--draft');
const config=JSON.parse(await readFile(resolve(root,'assets/update-config.json'),'utf8'));
if(!draft&&!/^[A-Za-z0-9-]+\/[A-Za-z0-9_.-]+$/.test(config.repository||''))throw Error('Set the official update repository before packaging a public release');
if((await readFile(resolve(tc.work,'build/ui-build-mode.txt'),'utf8')).trim()!=='release')throw Error('Refusing to package a UI automation build');
const stage=resolve(root,'dist','release-'+Date.now()),app=resolve(stage,'PPSA99178'),out=resolve(root,'public-release');await mkdir(app,{recursive:true});await mkdir(out,{recursive:true});
const inventory=[];async function copy(from,to){const s=await lstat(from);if(s.isSymbolicLink())throw Error('Symlink in release input');if(s.isDirectory()){await mkdir(to,{recursive:true});for(const n of await readdir(from))await copy(resolve(from,n),resolve(to,n));}else{await mkdir(dirname(to),{recursive:true});await copyFile(from,to);const b=await readFile(to);inventory.push({path:to.slice(app.length+1).replaceAll('\\','/'),bytes:b.length,sha256:createHash('sha256').update(b).digest('hex')});}}
await copy(resolve(root,'dist/PPSA99178/eboot.bin'),resolve(app,'eboot.bin'));
for(const n of['raff-service.elf','raff-updater.elf','LICENSE','THIRD_PARTY_NOTICES.md'])await copy(resolve(root,n),resolve(app,n));
await copy(resolve(root,'README-public.md'),resolve(app,'README.md'));
for(const n of['docs','licenses','runtime'])await copy(resolve(root,n),resolve(app,n));
for(const n of await readdir(resolve(root,'assets'))){if(['fonts','covers','platforms','emulators'].includes(n)||/^catalog.*\.json$/.test(n)||['downloads.sqlite','library.sqlite','core-registry.json','playstation-metadata.json','hub.json','systems.json','game-torrents.json','ps3-art-provenance.json','update-config.json','arabic.png','arabic-extra.png','arabic.json'].includes(n))await copy(resolve(root,'assets',n),resolve(app,'assets',n));}
await copy(resolve(root,'assets/brand/icon0.png'),resolve(app,'sce_sys/icon0.png'));
const runtime=resolve(root,'runtime/libc.prx');const hash=createHash('sha256').update(await readFile(runtime)).digest('hex');if(hash!=='e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036')throw Error('libc.prx digest mismatch');await copy(runtime,resolve(app,'sce_module/libc.prx'));
const param=JSON.parse(await readFile(resolve(root,'assets/app-param.json'),'utf8'));param.contentVersion=version.split('.').map((v,i)=>String(Number(v)+(i===1?10:0)).padStart(i===0?2:3,'0')).join('.');param.titleId='PPSA99178';param.versionFileUri='';await writeFile(resolve(app,'sce_sys/param.json'),JSON.stringify(param,null,2));await writeFile(resolve(app,'raff-owner.txt'),'Raff native Games app PPSA99178 v'+version+'\n');
for(const path of ['sce_sys/param.json','raff-owner.txt']){const b=await readFile(resolve(app,path));inventory.push({path,bytes:b.length,sha256:createHash('sha256').update(b).digest('hex')});}
const update=resolve(out,'Raff-v'+version+'.raffupdate'),zip=resolve(out,'Raff-v'+version+'-install.zip');for(const p of[update,zip])try{await access(p);await unlink(p);}catch(e){if(e.code!=='ENOENT')throw e;}
const result=await bundle(app,update);
const run=spawnSync(process.env.RAFF_PYTHON||'python',[resolve(root,'tools/archive-release.py'),stage,zip],{stdio:'inherit'});if(run.status!==0)throw Error(run.error?.message||'Install ZIP failed');
const hashes=[];for(const p of[zip,update]){const b=await readFile(p);hashes.push(createHash('sha256').update(b).digest('hex')+'  '+p.split(/[\\/]/).pop());}await writeFile(resolve(out,'SHA256SUMS.txt'),hashes.join('\n')+'\n');await writeFile(resolve(out,'manifest.json'),JSON.stringify({version,draft,repository:config.repository,update:result,files:inventory},null,2));
console.log(JSON.stringify({app,output:out,update:result,draft}));
