import {readFile,writeFile} from 'node:fs/promises';
import {fileURLToPath} from 'node:url';import {resolve} from 'node:path';
export function titleKey(value){const roman={i:1,ii:2,iii:3,iv:4,v:5,vi:6,vii:7,viii:8,ix:9,x:10};return String(value).normalize('NFKD').replace(/[\u0300-\u036f]/g,'').toLowerCase().replace(/[™®'’]/g,'').replace(/&/g,' and ').replace(/\b(?:ps4|ps5|for ps5)\b/g,'').replace(/[^a-z0-9]+/g,' ').trim().split(/ +/).map(t=>roman[t]||t).join(' ').replace(/ (?:digital deluxe|deluxe|complete|definitive|game of the year|standard|special|ultimate) edition(?: bundle)?$/,'').replace(/ directors cut$/,'').replace(/ and$/,'').trim();}
export function enrich(cards,metadata){
 const ratings=new Map();for(const r of metadata.ratings)for(const t of [r.title,r.requested,...r.products.map(p=>p.name)])if(t)ratings.set(titleKey(t),r);
 return cards.map(g=>{if(!['ps4','ps5'].includes(g.platform))return g;const key=titleKey(g.title),r=ratings.get(key);if(r){g={...g,rating:r.rating,score:Math.round(r.rating*20),ratingCount:r.countLabel,ratingSource:r.source,ratingCheckedAt:r.checkedAt};}
 const chart=metadata.charts.filter(c=>c.platform===g.platform&&titleKey(c.title)===key).sort((a,b)=>b.period.localeCompare(a.period)||a.rank-b.rank)[0];
 if(chart)g={...g,chartRank:chart.rank,chartPeriod:chart.period,chartRegion:chart.region,chartSource:chart.source,popularityScore:1000-chart.rank};
 return g;});
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 const path=new URL('../assets/catalog.json',import.meta.url),metadata=JSON.parse(await readFile(new URL('../assets/playstation-metadata.json',import.meta.url))),cards=enrich(JSON.parse(await readFile(path)),metadata);
 await writeFile(path,JSON.stringify(cards));for(const p of ['ps4','ps5'])console.log(p,{ratings:cards.filter(c=>c.platform===p&&c.rating).length,charts:cards.filter(c=>c.platform===p&&c.chartRank).length});
}
