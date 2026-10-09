import {spawnSync} from 'node:child_process';
import {readFile,writeFile,mkdir,readdir,stat,copyFile} from 'node:fs/promises';
import {resolve,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
const root=resolve(dirname(fileURLToPath(import.meta.url)),'..'),workspace=resolve(root,'../..');
import{tc,buildEnv}from'./toolchain.mjs';
const work=tc.work,out=resolve(work,'build');
const zig=tc.zig;
const sdk=tc.sdk;
const gl=tc.gl;
const env=buildEnv;
function run(exe,args){const r=spawnSync(exe,args,{stdio:'inherit',env});if(r.status!==0)process.exit(r.status||1);}
async function walk(p){let r=[];for(const f of await readdir(p,{withFileTypes:true}))r.push(...f.isDirectory()?await walk(resolve(p,f.name)):[resolve(p,f.name)]);return r;}
await mkdir(out,{recursive:true});
if(!await stat(resolve(out,'compiler_rt.a')).catch(()=>null))run(zig,['build-lib',resolve(dirname(zig),'lib/compiler_rt.zig'),'-target','x86_64-freestanding','-O','ReleaseFast','-fPIC','-femit-bin='+resolve(out,'compiler_rt.a')]);
const uiDependencies=await Promise.all(['src/systems.inc','src/catalogs.inc','src/settings.inc','src/storefront.inc','src/cinema.inc','src/shelves.hpp','src/cinema-qa.inc','src/hub.inc'].map(p=>stat(resolve(root,p))));
const sources=(await walk(resolve(root,'src'))).filter(p=>/\.(c|cpp)$/.test(p));
sources.push(...['app_crt','app_cpp_runtime'].map(n=>resolve(root,'third_party/native',n+'.cpp')));
const objects=[];const qa=process.argv.includes('--ui-qa'),mode=qa?'qa':'release',modeFile=resolve(out,'ui-build-mode.txt'),previousMode=await readFile(modeFile,'utf8').catch(()=>'');
for(const s of sources){
 const o=resolve(out,s.substring(root.length+1).replace(/[\\/]/g,'_')+'.o');objects.push(o);
 const newer=await stat(o).catch(()=>null);if(newer&&newer.mtimeMs>(await stat(s)).mtimeMs&&(!s.endsWith('main.cpp')||previousMode===mode&&uiDependencies.every(d=>newer.mtimeMs>d.mtimeMs))&&!process.argv.includes('--clean'))continue;
 console.log('Compile',s.substring(root.length+1));
 run(zig,['cc','-target','x86_64-freebsd','-D__SCE__','-DGL_GLEXT_PROTOTYPES',...(qa&&s.endsWith('main.cpp')?['-DRAFF_UI_QA']:[]),'-O2','-fPIC','-fno-stack-protector','-fno-plt','-femulated-tls','-ffunction-sections','-fdata-sections','-nostdinc',...(s.endsWith('.cpp')?['-std=c++20','-fno-exceptions','-fno-rtti','-nostdinc++','-I',resolve(sdk,'target/include/c++/v1')]:['-std=c11']),'-I',resolve(sdk,'target/include'),'-isystem',resolve(dirname(zig),'lib/include'),'-I',resolve(gl,'include'),'-I',resolve(root,'src/vendor'),'-I',resolve(root,'third_party/harfbuzz/src'),'-c',s,'-o',o]);
}
await writeFile(modeFile,mode);
const libs=['libPS5OpenGL.a'].map(n=>resolve(gl,'lib',n));
libs.push(...['libc++.a','libc++abi.a','libunwind.a'].map(n=>resolve(sdk,'target/lib',n)));
libs.push(resolve(out,'compiler_rt.a'));
const stubs=(await readdir(resolve(sdk,'target/lib'))).filter(n=>n.endsWith('.so')).map(n=>resolve(sdk,'target/lib',n));
stubs.push(...['libSceAgc.so','libSceAgcDriver.so'].map(n=>resolve(gl,'lib',n)));
const symbols=['malloc','calloc','realloc','free','posix_memalign','malloc_usable_size','sceSystemServiceHideSplashScreen'];
run(zig,['ld.lld','-m','elf_x86_64','-pie','--no-dependent-libraries','--gc-sections','-z','max-page-size=0x4000','-T',resolve(root,'third_party/native/ps5-pie.ld'),'--eh-frame-hdr','--version-script',resolve(root,'third_party/native/app-symbols.map'),...symbols.map(n=>'--wrap='+n),'-u','ps5_agc_gate2_run','-e','_start','-o',resolve(out,'llvm-pie.elf'),...objects,'--start-group',...libs,'--end-group','--as-needed',...stubs]);
const tool=tc.converter;
const stubDir=resolve(out,'stubs');await mkdir(stubDir,{recursive:true});for(const path of stubs)await copyFile(path,resolve(stubDir,path.split(/[\\/]/).pop()));
run(tool,['link','--in',resolve(out,'llvm-pie.elf'),'--out',resolve(out,'eboot.elf'),'--stub-dir',stubDir,'--module-sdk','0x02000009','--companion-sdk','0x08050001','--file-name','eboot.elf']);
const app=resolve(root,'dist/PPSA99178');await mkdir(app,{recursive:true});
run(tool,['self','--sign','--in',resolve(out,'eboot.elf'),'--out',resolve(app,'eboot.bin'),'--magic','0x1D3D154F']);
run(tool,['self','--inspect','--file',resolve(app,'eboot.bin')]);
console.log('Native app built:',app);
