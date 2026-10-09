import{readFile}from'node:fs/promises';import{resolve,dirname}from'node:path';import{fileURLToPath}from'node:url';
export const root=resolve(dirname(fileURLToPath(import.meta.url)),'..');
const legacy=resolve(root,'../..');let user={};try{user=JSON.parse(await readFile(resolve(root,'toolchain.local.json'),'utf8'));}catch(e){if(e.code!=='ENOENT')throw e;}
const location=(key,fallback)=>resolve(user[key]||process.env['RAFF_'+key.toUpperCase()]||fallback);
export const tc={
 work:location('work',resolve(legacy,'work/raff-v5')),
 zig:location('zig',resolve(legacy,'work/toolchain/zig/zig-x86_64-windows-0.14.1/zig.exe')),
 sdk:location('sdk',resolve(legacy,'work/toolchain/opt/ps5-payload-sdk')),
 gl:location('gl',resolve(legacy,'work/raff-v5/deps/opengl/ps5-opengl-sdk-1.0.1/sdk')),
 quickjs:location('quickjs',resolve(legacy,'work/raff-v5/deps/quickjs/quickjs-535a7c250ff4a577ec36c3e103daab6dadeea650')),
 pacbrew:location('pacbrew',resolve(legacy,'work/raff-v9/pacbrew')),
 converter:location('converter',resolve(legacy,'work/raff-v5/host/ps5-native-tool.exe')),
};
export const buildEnv={...process.env,ZIG_GLOBAL_CACHE_DIR:resolve(tc.work,'zig-global-cache'),ZIG_LOCAL_CACHE_DIR:resolve(tc.work,'zig-local-cache')};
