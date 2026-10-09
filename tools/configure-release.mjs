import{readFile,writeFile}from'node:fs/promises';import{resolve}from'node:path';import{root}from'./toolchain.mjs';import policy from'../service/updates.js';
const args=process.argv.slice(2),value=k=>args[args.indexOf(k)+1];
if(!args.includes('--repo')||!args.includes('--version'))throw Error('Usage: node tools/configure-release.mjs --repo OWNER/REPO --version 1.2.0');
const repository=policy.repository(value('--repo')),version=value('--version');policy.version(version);if(version.startsWith('v'))throw Error('Use a version without v');
const old=policy.VERSION;for(const name of['service/updates.js','service/main.js','service/queue.js','service/host.c','src/cinema.inc','src/settings.inc','src/main.cpp','tools/package-release.mjs']){const p=resolve(root,name),s=await readFile(p,'utf8');await writeFile(p,s.replaceAll(old,version));}
await writeFile(resolve(root,'assets/update-config.json'),JSON.stringify({schema:1,repository,channel:'stable'},null,2)+'\n');
await writeFile(resolve(root,'release.json'),JSON.stringify({version,titleId:'PPSA99178',repository,channel:'stable'},null,2)+'\n');console.log('Configured '+repository+' v'+version+'; rebuild and test before publishing');
