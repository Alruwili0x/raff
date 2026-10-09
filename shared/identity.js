(function (root) {
  'use strict';
  const region = /^(?:usa|us|u|europe|eur|eu|e|japan|jpn|jp|j|world|asia|as|korea|ko|kr|australia|brazil|china|taiwan|uk|france|germany|spain|italy|russia|ntsc(?:[- ]?[uj])?|pal)(?:\s*[,/+&-]\s*(?:usa|us|europe|eu|japan|jp|asia|as|korea|ko|kr|australia))*$/i;
  const languages = /^(?:en|fr|de|es|it|ja|ko|zh|pt|ru|nl|sv|da|no|fi|pl|ar)(?:\s*[,/+ -]\s*(?:en|fr|de|es|it|ja|ko|zh|pt|ru|nl|sv|da|no|fi|pl|ar))*$/i;
  const roman = { ii: '2', iii: '3', iv: '4', v: '5', vi: '6', vii: '7', viii: '8', ix: '9', x: '10' };
  function titleParts(value) {
    let title = String(value || '').normalize('NFKC').replace(/\.(?:nes|sfc|smc|z64|n64|v64|gb|gbc|gba|nds|3ds|cci|cxi|rvz|gcm|wbfs|md|bin|gen|smd|sms|gg|cso|chd|cue|pbp|m3u|xiso(?:\.iso)?|iso|nsp|pkg|fpkg|ffpkg|ffpfs|ffpfsc|exfat|7z|zip|rar)$/i, '').replace(/^(?:SLUS|SLES|SCUS|SCES|SLPM|SLPS|SCPS)[-_ .]?\d{3}[._]?\d{2}[._ -]*/i, '').replace(/^(?:PPSA|CUSA)\d{5}[ _-]*/i,'');
    const tags = [];
    title = title.replace(/[([]([^\])]+)[\])]/g, (whole, tag) => {
      tag = tag.trim();
      if (/^(?:BLUS|BLES|BCUS|BCES|NPUB|NPUA|NPEB|NPEA)\d{5}$/i.test(tag) || region.test(tag) || languages.test(tag) || /^(?:En|Fr|De|Es|It|Ja|Ko|Zh|Pt|Ru|Nl|Sv|Da|No|Fi|Pl|Ar){2,}$/i.test(tag) || /^(?:rev(?:ision)?\s*[\d.]+|v\d+(?:\.\d+)*|disc\s*\d+(?:\s*of\s*\d+)?|disk\s*\d+|dvd\s*\d+|cd\s*\d+|ps2|chd|xiso|iso|redump)$/i.test(tag)) { tags.push(tag); return ' '; }
      return whole;
    });
    title = title.replace(/[™®]/g, '').replace(/\s+/g, ' ').trim();
    const disc = tags.join(' ').match(/(?:disc|disk|dvd|cd)\s*(\d+)/i);
    const key = title.normalize('NFKD').toLowerCase().replace(/[\u0300-\u036f]/g, '').replace(/&/g, ' and ').replace(/[’‘']/g, '').replace(/[^\p{L}\p{N}]+/gu, ' ').trim().split(/\s+/).map(w => roman[w] || w).join(' ');
    return { title, key, disc: disc ? Number(disc[1]) : 1, tags };
  }
  function identity(game) {
    const p = game.platform || 'ps2';
    if (p === 'switch' && game.titleId) return 'switch:' + String(game.titleId).toUpperCase();
    return p + ':' + titleParts(game.title || game.filename || game.name).key;
  }
  function sameInstalled(game, entry) {
    const candidate = { platform: game.platform || 'ps2', title: entry.name || entry.filename || entry.title };
    const foreignId=String(candidate.title).toUpperCase().match(/(?:PPSA|CUSA)\d{5}/);
    if(foreignId && ((game.platform==='ps4'&&foreignId[0].startsWith('PPSA'))||(game.platform==='ps5'&&foreignId[0].startsWith('CUSA'))))return false;
    const a = titleParts(game.filename || game.title), b = titleParts(candidate.title);
    if (['switch','ps5','ps4'].includes(game.platform) && game.titleId && String(candidate.title).toUpperCase().includes(String(game.titleId).toUpperCase())) return true;
    // Providers sometimes remove every space from their download filenames.
    // Keep sequel numbers and edition words while recognizing those aliases.
    const compact = key => key.replace(/\s+/g, '');
    const sameName = identity(game) === identity(candidate) || a.key === b.key ||
      compact(titleParts(game.title).key) === compact(b.key);
    return sameName && a.disc === b.disc;
  }
  const installedFingerprints=new WeakMap();
  function installedFingerprint(game){
    let f=installedFingerprints.get(game);
    if(f&&f.title===game.title&&f.filename===game.filename&&f.name===game.name&&f.platform===game.platform&&f.titleId===game.titleId)return f;
    const p=game.platform||'ps2',title=titleParts(game.title||game.filename||game.name),file=game.filename?titleParts(game.filename):title;
    f={title:game.title,filename:game.filename,name:game.name,platform:game.platform,titleId:game.titleId,p,key:p==='switch'&&game.titleId?'switch:'+String(game.titleId).toUpperCase():p+':'+title.key,fileKey:file.key,disc:file.disc,compact:title.key.replace(/\s+/g,'')};
    installedFingerprints.set(game,f);return f;
  }
  // Compile installed names once per inventory change; never normalize every game/entry pair.
  function installedKeys(games,inventory){
    const indexes=new Map(),out=new Set();
    for(const[p,entries]of Object.entries(inventory)){const rows=[],discs=new Map();for(const e of entries){const name=String(e.name||e.filename||e.title||''),upper=name.toUpperCase(),foreign=upper.match(/(?:PPSA|CUSA)\d{5}/);if(foreign&&(p==='ps4'&&foreign[0].startsWith('PPSA')||p==='ps5'&&foreign[0].startsWith('CUSA')))continue;const b=titleParts(name);let disc=discs.get(b.disc);if(!disc){disc={keys:new Set(),identities:new Set(),compact:new Set()};discs.set(b.disc,disc);}disc.keys.add(b.key);disc.identities.add(p+':'+b.key);disc.compact.add(b.key.replace(/\s+/g,''));rows.push(upper);}indexes.set(p,{rows,discs});}
    for(const game of games){const index=indexes.get(game.platform||'ps2');if(!index?.rows.length)continue;const f=installedFingerprint(game),disc=index.discs.get(f.disc);if((['switch','ps4','ps5'].includes(f.p)&&game.titleId&&index.rows.some(n=>n.includes(String(game.titleId).toUpperCase())))||disc&&(disc.identities.has(f.key)||disc.keys.has(f.fileKey)||disc.compact.has(f.compact)))out.add(f.key);}
    return [...out];
  }
  function groups(games) {
    const byKey = new Map();
    for (const original of games) {
      const game = Object.assign({}, original), key = identity(game), parts = titleParts(game.title);
      let group = byKey.get(key);
      if (!group) { group = { key, platform: game.platform || 'ps2', title: parts.title, options: [], aliases: [] }; byKey.set(key, group); }
      if (!group.aliases.includes(game.title)) group.aliases.push(game.title);
      const signature = game.url || game.sourcePage || game.id;
      if (signature && group.options.some(o => (o.url || o.sourcePage || o.id) === signature)) continue;
      game.variant = parts.tags.join(' · '); game.disc = titleParts(game.filename || game.title).disc;
      group.options.push(game);
    }
    return Array.from(byKey.values());
  }
  const api = { titleParts, identity, sameInstalled, installedKeys, groups };
  if (typeof module !== 'undefined' && module.exports) module.exports = api; else root.RaffIdentity = api;
})(typeof globalThis !== 'undefined' ? globalThis : window);
