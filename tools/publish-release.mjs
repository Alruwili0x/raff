/* Creates a reviewable draft, never publishes an untested binary as stable. */
import{readFile,readdir,access}from'node:fs/promises';import{resolve}from'node:path';import{spawnSync}from'node:child_process';import{root}from'./toolchain.mjs';import policy from'../service/updates.js';
const cfg=JSON.parse(await readFile(resolve(root,'assets/update-config.json'),'utf8')),repo=policy.repository(cfg.repository),tag='v'+policy.VERSION;
const out=resolve(root,'public-release'),manifest=JSON.parse(await readFile(resolve(out,'manifest.json'),'utf8'));if(manifest.draft||manifest.repository!==repo||manifest.version!==policy.VERSION)throw Error('Rebuild a public package for this exact repository/version');
const files=['Raff-'+tag+'-Installer.elf','Raff-'+tag+'-install.zip','Raff-'+tag+'.raffupdate','Raff-'+tag+'-source.zip','Raff-'+tag+'-build-assets.zip','SHA256SUMS.txt','VALIDATION.md'];for(const f of files)await access(resolve(out,f));
function gh(args,capture=false){const p=spawnSync('gh',args,{cwd:root,encoding:'utf8',stdio:capture?'pipe':'inherit'});if(p.status!==0)throw Error(p.error?.message||p.stderr||'GitHub CLI failed');return p.stdout;}
const remote=JSON.parse(gh(['repo','view','--json','nameWithOwner'],true));if(remote.nameWithOwner.toLowerCase()!==repo.toLowerCase())throw Error('Git remote and configured update repository differ');
const third=(await readdir(out)).filter(f=>f.endsWith('.tar.gz'));
gh(['release','create',tag,'--repo',repo,'--draft','--title','Raff '+tag,'--notes-file',resolve(out,'VALIDATION.md'),...files.map(f=>resolve(out,f)),...third.map(f=>resolve(out,f))]);
