(function (root) {
  'use strict';
  const GAMES = '/data/PCSX2/games';
  const STAGING = '/data/PCSX2/.ps2-library';
  const PLATFORMS = {
    "nes": {"id":"nes","name":"NES","title":"Nintendo Entertainment System","core":"fceumm","formats":["nes"],"games":"/data/homebrew/PPSA99169/content/NES","staging":"/data/RetroArch/.raff-nes","keepArchive":false,"bios":false,"experimental":false},
    "snes": {"id":"snes","name":"SNES","title":"Super Nintendo","core":"snes9x","formats":["sfc","smc"],"games":"/data/homebrew/PPSA99169/content/SNES","staging":"/data/RetroArch/.raff-snes","keepArchive":false,"bios":false,"experimental":false},
    "n64": {"id":"n64","name":"Nintendo 64","title":"Nintendo 64","core":"mupen64plus_next","formats":["z64","n64","v64"],"games":"/data/homebrew/PPSA99169/content/N64","staging":"/data/RetroArch/.raff-n64","keepArchive":false,"bios":false,"experimental":true},
    "gb": {"id":"gb","name":"Game Boy","title":"Game Boy","core":"mgba","formats":["gb"],"games":"/data/homebrew/PPSA99169/content/GB","staging":"/data/RetroArch/.raff-gb","keepArchive":false,"bios":false,"experimental":false},
    "gbc": {"id":"gbc","name":"Game Boy Color","title":"Game Boy Color","core":"mgba","formats":["gbc"],"games":"/data/homebrew/PPSA99169/content/GBC","staging":"/data/RetroArch/.raff-gbc","keepArchive":false,"bios":false,"experimental":false},
    "gba": {"id":"gba","name":"Game Boy Advance","title":"Game Boy Advance","core":"mgba","formats":["gba"],"games":"/data/homebrew/PPSA99169/content/GBA","staging":"/data/RetroArch/.raff-gba","keepArchive":false,"bios":false,"experimental":false},
    "nds": {"id":"nds","name":"Nintendo DS","title":"Nintendo DS","core":"desmume","formats":["nds"],"games":"/data/homebrew/PPSA99169/content/NDS","staging":"/data/RetroArch/.raff-nds","keepArchive":false,"bios":false,"experimental":false},
    "3ds": {"id":"3ds","name":"Nintendo 3DS","title":"Nintendo 3DS","core":"azahar","formats":["3ds","cci","cxi"],"games":"/data/homebrew/PPSA99169/content/3DS","staging":"/data/RetroArch/.raff-3ds","keepArchive":false,"bios":false,"experimental":true},
    "psp": {"id":"psp","name":"PSP","title":"PlayStation Portable","core":"ppsspp","formats":["iso","cso","pbp"],"games":"/data/homebrew/PPSA99169/content/PSP","staging":"/data/RetroArch/.raff-psp","keepArchive":false,"bios":false,"experimental":false},
    "gamecube": {"id":"gamecube","name":"GameCube","title":"Nintendo GameCube","core":"dolphin","formats":["rvz","gcm","iso"],"games":"/data/homebrew/PPSA99169/content/GAMECUBE","staging":"/data/RetroArch/.raff-gamecube","keepArchive":false,"bios":false,"experimental":true},
    "wii": {"id":"wii","name":"Wii","title":"Nintendo Wii","core":"dolphin","formats":["rvz","wbfs","iso"],"games":"/data/homebrew/PPSA99169/content/WII","staging":"/data/RetroArch/.raff-wii","keepArchive":false,"bios":false,"experimental":true},
    "genesis": {"id":"genesis","name":"Mega Drive","title":"Sega Mega Drive","core":"genesis_plus_gx","formats":["md","bin","gen","smd"],"games":"/data/homebrew/PPSA99169/content/GENESIS","staging":"/data/RetroArch/.raff-genesis","keepArchive":false,"bios":false,"experimental":false},
    "mastersystem": {"id":"mastersystem","name":"Master System","title":"Sega Master System","core":"genesis_plus_gx","formats":["sms"],"games":"/data/homebrew/PPSA99169/content/MASTERSYSTEM","staging":"/data/RetroArch/.raff-mastersystem","keepArchive":false,"bios":false,"experimental":false},
    "gamegear": {"id":"gamegear","name":"Game Gear","title":"Sega Game Gear","core":"genesis_plus_gx","formats":["gg"],"games":"/data/homebrew/PPSA99169/content/GAMEGEAR","staging":"/data/RetroArch/.raff-gamegear","keepArchive":false,"bios":false,"experimental":false},
    "saturn": {"id":"saturn","name":"Saturn","title":"Sega Saturn","core":"mednafen_saturn","formats":["chd","cue"],"games":"/data/homebrew/PPSA99169/content/SATURN","staging":"/data/RetroArch/.raff-saturn","keepArchive":false,"bios":true,"experimental":false},
    "segacd": {"id":"segacd","name":"Sega CD","title":"Sega CD","core":"genesis_plus_gx","formats":["chd","cue"],"games":"/data/homebrew/PPSA99169/content/SEGACD","staging":"/data/RetroArch/.raff-segacd","keepArchive":false,"bios":true,"experimental":false},
    "dreamcast": {"id":"dreamcast","name":"Dreamcast","title":"Sega Dreamcast","core":"PSFlycast","formats":["chd"],"games":"/data/homebrew/PPSA99247/games","staging":"/data/RetroArch/.raff-dreamcast","keepArchive":false,"bios":false,"experimental":true},
    "arcade": {"id":"arcade","name":"Arcade / FBNeo","title":"Arcade (FBNeo)","core":"fbneo","formats":["zip"],"games":"/data/homebrew/PPSA99169/content/ARCADE","staging":"/data/RetroArch/.raff-arcade","keepArchive":true,"bios":true,"experimental":false},
    xbox: {id:'xbox',name:'Xbox Original',games:'/data/xemu/games',staging:'/data/xemu/.raff-staging',formats:['iso','xiso']},
    xbox360: {id:'xbox360',name:'Xbox 360',games:'/data/xbox360',staging:'/data/xbox360-staging',formats:['iso']},
    ps1: { id:'ps1', name:'PlayStation 1', games:'/data/homebrew/PPSA99169/content/PS1', staging:'/data/RetroArch/.raff-ps1', formats:['cue','chd','pbp','m3u'] },
    ps3: {id:'ps3',name:'PlayStation 3',games:'/data/rpcs3/games',staging:'/data/rpcs3/.raff-staging',formats:['iso','pkg']},
    ps2: { id: 'ps2', name: 'PlayStation 2', games: GAMES, staging: STAGING, formats: ['iso', 'chd'] },
    switch: { id: 'switch', name: 'Nintendo Switch', games: '/data/prosperoeden/roms', staging: '/data/prosperoeden/.raff-library', formats: ['nsp'] },
    ps5: { id: 'ps5', name: 'PlayStation 5', games: '/data/etaHEN/games', staging: '/data/raff/ps5-staging', formats: ['pkg','fpkg','ffpkg','ffpfs','ffpfsc','exfat'] },
    ps4: { id: 'ps4', name: 'PlayStation 4', games: '/data/etaHEN/games', staging: '/data/raff/ps4-staging', formats: ['pkg','fpkg','ffpkg','ffpfs','ffpfsc','exfat'] }
  };
  function platformFor(id) { const p = PLATFORMS[id || 'ps2']; if (!p) fail('منصة غير مدعومة.'); return p; }
  function destinationPath(value) {
    const path = String(value || '').replace(/\/+$/, '');
    const retroContent = /^\/data\/homebrew\/(?:PPSA99169\/content|PPSA99247\/games)(?:\/|$)/.test(path);
    if (path.length > 240 || !/^(?:\/data\/[^/]+|\/mnt\/usb[0-7]\/[^/]+)(?:\/[^/]+)*$/.test(path) || /[\x00-\x1f\x7f\\]/.test(path) || path.split('/').some(p => p === '.' || p === '..') || Object.values(PLATFORMS).some(p=>path===p.staging||inside(path,p.staging)) || (!retroContent && /^\/data\/(?:homebrew|raff|prosperoeden\/\.raff-library|PCSX2\/\.ps2-library)(?:\/|$)/.test(path)))
      fail('اختر مجلد ألعاب داخل data أو USB متصل، بعيدًا عن ملفات التطبيق المؤقتة.');
    return path;
  }
  function destinationFor(game) { return game.destination ? destinationPath(game.destination) : platformFor(game.platform).games; }
  const staged = path => Object.values(PLATFORMS).some(p => inside(path, p.staging));
  const headerLimit = name => ext(name) === 'nsp' ? 1024 * 1024 : ext(name) === 'ffpkg' ? 131072 : 65536;
  const MiB = 1024 * 1024;
  const SOURCES = {
    ps2chd: { name: 'PS2CHD', origin: 'https://ps2chd.com', path: /^\/[a-z0-9-]+\/[a-z0-9-]+$/, anchor: 'dlpLink', host: 'download.ps2chd.com' },
    chdstation: { name: 'CHD Station', origin: 'https://chdstation.com', path: /^\/ps2-chd\/[a-z0-9-]+$/, anchor: 'dlgo', host: 'download.chdstation.com' },
    chdstationps1: { name: 'CHD Station PS1', origin: 'https://chdstation.com', path: /^\/ps1-chd\/[a-z0-9-]+$/, anchor: 'dlgo', host: 'download.chdstation.com', platform: 'ps1' },
    archive: { name: 'Internet Archive' },
    dlarchive: { name: 'DLArchive', origin: 'https://drive.dlarchive.my.id', path: /^\/dl\/?$/, platform: 'switch' }
  };
  function sourceFor(pageURL) {
    const page = new URL(pageURL);
    return Object.keys(SOURCES).find(key => SOURCES[key].origin === page.origin && SOURCES[key].path.test(page.pathname));
  }
  function fail(message) { throw new Error(message); }
  function filename(value) {
    if (typeof value !== 'string' || !value || value.length > 220 || /[\x00-\x1f\x7f/\\]/.test(value) || value === '.' || value === '..') fail('اسم الملف غير صالح.');
    return value;
  }
  function ext(name) { return name.split('.').pop().toLowerCase(); }
  function webURL(value) {
    const url = new URL(value);
    if (!/^https?:$/.test(url.protocol) || url.username || url.password) fail('يلزم رابط HTTP أو HTTPS مباشر.');
    return url.origin + url.pathname.replace(/\[/g, '%5B').replace(/\]/g, '%5D') + url.search + url.hash;
  }
  function origin(value) {
    const url = new URL(webURL(value));
    if (url.pathname !== '/' || url.search || url.hash) fail('أدخل عنوان الخدمة فقط، دون مسار إضافي.');
    return url.origin;
  }
  function inside(path, base) {
    return typeof path === 'string' && path.indexOf(base + '/') === 0 && !/[\x00-\x1f\\]/.test(path) && path.split('/').every((p, i) => i === 0 || (p && p !== '.' && p !== '..'));
  }
  function validateGame(input) {
    const game = Object.assign({}, input);
    if (game.destination) game.destination = destinationPath(game.destination);
    if (!game.title || typeof game.title !== 'string' || game.title.length > 200) fail('اسم اللعبة مطلوب.');
    if (game.url) {
      game.url = webURL(game.url);
      if (game.mirrors) {
        if (!Array.isArray(game.mirrors) || game.mirrors.length > 4) fail('قائمة خوادم التحميل غير صالحة.');
        game.mirrors = [...new Set(game.mirrors.map(webURL))].filter(url => url !== game.url);
      }
      game.filename = filename(game.filename || decodeURIComponent(new URL(game.url).pathname.split('/').pop()));
      game.platform = game.platform || (ext(game.filename) === 'nsp' ? 'switch' : 'ps2');
      if (!platformFor(game.platform).formats.concat(['zip', '7z', 'rar']).includes(ext(game.filename))) fail('صيغة الملف لا تطابق المنصة المختارة.');
      if (/^(zip|7z|rar)$/.test(ext(game.filename)) && !platformFor(game.platform).keepArchive && !(Number(game.unpackedBytes) > 0)) fail('حدد الحجم المتوقع بعد فك الضغط لهذه اللعبة.');
    } else {
      const page = new URL(game.sourcePage);
      const source = sourceFor(page.href);
      if (!source) fail('صفحة المصدر غير مدعومة.');
      game.platform = game.platform || SOURCES[source].platform || 'ps2';
      if (game.platform !== (SOURCES[source].platform || 'ps2')) fail('صيغة الملف لا تطابق المنصة المختارة.');
      game.source = source;
      game.sourcePage = page.href;
      if (source === 'dlarchive') {
        game.fileId = page.searchParams.get('id');
        if (!/^[a-z0-9]{6,40}$/.test(game.fileId || '')) fail('صفحة المصدر غير مدعومة.');
        if (game.filename && ext(filename(game.filename)) !== 'nsp') fail('صيغة الملف لا تطابق المنصة المختارة.');
      }
    }
    game.sizeBytes = Math.max(0, Number(game.sizeBytes) || 0);
    return game;
  }
  function parseDownload(html, source) {
    if (source === 'dlarchive') {
      let data; try { data = JSON.parse(html); } catch (_) { fail('تعذر قراءة رابط NSP من المصدر.'); }
      if (!data.ok || !data.url || !data.filename) fail('تعذر قراءة رابط NSP من المصدر.');
      const url = new URL(webURL(data.url)), name = filename(data.filename);
      if (url.protocol !== 'https:' || url.hostname !== 'direct-stor.berkasdrive.com' || ext(url.pathname) !== 'nsp' || ext(name) !== 'nsp') fail('رابط NSP غير متوقع من المصدر.');
      return { url: url.href, filename: name };
    }
    const provider = SOURCES[source || 'ps2chd'];
    if (!provider || !provider.anchor) fail('صفحة المصدر غير مدعومة.');
    const links = html.match(/<a\b[^>]*>/gi) || [];
    const anchor = links.find(t => new RegExp('\\bid\\s*=\\s*["\']' + provider.anchor + '["\']', 'i').test(t));
    const match = anchor && anchor.match(/\bhref\s*=\s*["']([^"']+)["']/i);
    if (!match) fail('لم يظهر رابط CHD مباشر في المصدر. قد تكون الصفحة تغيّرت.');
    const url = new URL(match[1].replace(/&amp;/g, '&'));
    if (url.protocol !== 'https:' || url.hostname !== provider.host || ext(url.pathname) !== 'chd') fail('رابط المصدر ليس ملف CHD متوقعًا.');
    return { url: url.href, filename: filename(decodeURIComponent(url.pathname.split('/').pop())) };
  }
  function verifyHeader(name, bytes, fileSize) {
    const text = (start, length) => String.fromCharCode.apply(null, Array.from(bytes.slice(start, start + length)));
    if(ext(name)==='cue'){
      const cue=new TextDecoder().decode(bytes);
      if(fileSize>65536||!/^\s*FILE\s+(?:"[^"/\\]+\.bin"|[^\s/\\]+\.bin)\s+BINARY\s*$/im.test(cue)||!/^\s*TRACK\s+\d+\s+/im.test(cue))fail('Invalid CUE track list');
    } else if (ext(name) === 'chd') {
      if (bytes.length < 16 || text(0, 8) !== 'MComprHD') fail('الملف المحمّل لا يحمل ترويسة CHD صحيحة.');
    } else if (ext(name) === 'iso') {
      let valid = false;
      for (let sector = 16; sector < 32; sector++) if (/^(CD001|BEA01|NSR02|NSR03)$/.test(text(sector * 2048 + 1, 5))) valid = true;
      if (!valid) fail('الملف المحمّل لا يحمل ترويسة ISO/UDF معروفة. بقي في المجلد المؤقت.');
    } else if (ext(name) === 'nsp') {
      const invalid = () => fail('بنية NSP غير صحيحة أو الملف غير مكتمل. بقي في المجلد المؤقت.');
      if (bytes.length < 16 || text(0, 4) !== 'PFS0') invalid();
      const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength), count = view.getUint32(4, true), strings = view.getUint32(8, true);
      const stringStart = 16 + count * 24, dataStart = stringStart + strings;
      if (!count || count > 8192 || !strings || dataStart > 1024 * 1024 || dataStart > bytes.length || view.getUint32(12, true)) invalid();
      const u64 = at => view.getUint32(at, true) + view.getUint32(at + 4, true) * 4294967296;
      const ranges = [], names = new Set(); let nca = false;
      for (let i = 0; i < count; i++) {
        const entry = 16 + i * 24, offset = u64(entry), length = u64(entry + 8), stringOffset = view.getUint32(entry + 16, true);
        if (!Number.isSafeInteger(offset + length + dataStart) || !length || stringOffset >= strings || view.getUint32(entry + 20, true)) invalid();
        const end = bytes.indexOf(0, stringStart + stringOffset);
        if (end < 0 || end >= dataStart) invalid();
        const part = text(stringStart + stringOffset, end - stringStart - stringOffset);
        if (!part || part.length > 220 || /[\x00-\x1f/\\]/.test(part) || names.has(part) || /\.ncz$/i.test(part)) invalid();
        names.add(part); if (/\.nca$/i.test(part)) nca = true;
        if (fileSize !== undefined && (!Number.isSafeInteger(Number(fileSize)) || dataStart + offset + length > Number(fileSize))) invalid();
        ranges.push([offset, offset + length]);
      }
      ranges.sort((a, b) => a[0] - b[0]);
      if (!nca || ranges.some((range, i) => i > 0 && range[0] < ranges[i - 1][1])) invalid();
    } else if (['pkg','fpkg'].includes(ext(name))) {
      const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
      const magic = bytes.length >= 4 ? view.getUint32(0, false) : 0;
      if (magic === 0x7f464948) {
        // PS5 finalized image: little-endian PFS and metadata segment ranges.
        const u64 = at => view.getUint32(at, true) + view.getUint32(at + 4, true) * 4294967296;
        if (bytes.length < 0xa8) fail('Invalid PS5 package header.');
        const pfs = u64(0x10), pfsSize = u64(0x18), container = u64(0x58);
        if (pfs !== 65536 || !pfsSize || container < pfs + pfsSize ||
            !Number.isSafeInteger(container + 0xa0) ||
            fileSize !== undefined && container + 0xa0 > Number(fileSize)) fail('Invalid PS5 package segment bounds.');
      } else if (bytes.length < 128 || magic !== 0x7f434e54 || (fileSize !== undefined && Number(fileSize) < 128))
        fail('ترويسة PKG غير صحيحة. بقي الملف في المجلد المؤقت.');
    } else if (['ffpfs','ffpfsc'].includes(ext(name))) {
      const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
      if (bytes.length < 128 || ![1,2].includes(view.getUint32(0,true)) || view.getUint32(4,true)!==0 ||
          view.getUint32(8,true)!==20130315 || view.getUint32(12,true)!==0 ||
          ![4096,16384,32768,65536].includes(view.getUint32(32,true)) || view.getUint32(36,true)!==0)
        fail('ترويسة صورة PS5 غير صحيحة. بقي الملف في المجلد المؤقت.');
    } else if (ext(name)==='exfat') {
      if(bytes.length<512 || text(3,8)!=='EXFAT   ' || bytes[510]!==0x55 || bytes[511]!==0xaa || bytes[108]<9 || bytes[108]>12)
        fail('ترويسة exFAT غير صحيحة. بقي الملف في المجلد المؤقت.');
    } else if (ext(name)==='ffpkg') {
      const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
      if(bytes.length<65536+1376 || view.getUint32(65536+1372,true)!==0x19540119)
        fail('ترويسة UFS2 غير صحيحة. FFPKG تختلف عن PKG.');
    } else fail('صيغة ملف اللعبة النهائي غير مدعومة.');
    return true;
  }
  // XDVDFS sector 32, also used by Xenia's game-partition detection.
  const XBOX_OFFSETS = [0,0xFB20,0x20600,0x2080000,0xFD90000];
  function verifyXboxSector(bytes,fileSize,partition=0) {
    const magic = 'MICROSOFT*XBOX*MEDIA';
    const text = at => String.fromCharCode.apply(null,Array.from(bytes.slice(at,at+20)));
    if(bytes.length!==2048 || text(0)!==magic || text(2028)!==magic) fail('Invalid Xbox disc header / ترويسة قرص Xbox غير صحيحة.');
    const v=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),sector=v.getUint32(20,true),size=v.getUint32(24,true);
    if(!Number.isSafeInteger(fileSize)||fileSize<67584||sector<33||size<14||size>33554432||partition+sector*2048+size>fileSize)fail('Incomplete Xbox disc / ملف Xbox غير مكتمل.');
    return true;
  }
  function verifyPS3Package(bytes,fileSize){
    const bad=()=>fail('Invalid PS3 PKG / حزمة PS3 غير صحيحة.');
    if(bytes.length<128||!Number.isSafeInteger(fileSize))bad();
    const v=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),u64=at=>v.getUint32(at,false)*4294967296+v.getUint32(at+4,false);
    const size=u64(24),offset=u64(32),data=u64(40);
    if(v.getUint32(0,false)!==0x7f504b47||v.getUint16(6,false)!==1||size!==fileSize||offset<128||data<=0||!Number.isSafeInteger(offset+data)||offset+data>size)bad();
    return true;
  }
  async function verifyPS3ISO(client,file){
    const size=Number(file.size),bad=()=>fail('Requires a complete decrypted PS3 ISO / يلزم قرص PS3 كامل وغير مشفر.');
    if(!Number.isSafeInteger(size)||size<65536)bad();
    const read=async(offset,length)=>{if(!Number.isSafeInteger(offset)||offset<0||length<1||length>1048576||offset+length>size)bad();const b=await client.bytes(file.path,length,offset);if(b.length!==length)bad();return b;};
    const view=b=>new DataView(b.buffer,b.byteOffset,b.byteLength),str=b=>String.fromCharCode.apply(null,Array.from(b));
    const pvd=await read(32768,2048);if(pvd[0]!==1||str(pvd.slice(1,6))!=='CD001'||view(pvd).getUint16(128,true)!==2048)bad();
    const record=(b,at)=>({offset:view(b).getUint32(at+2,true)*2048,length:view(b).getUint32(at+10,true),directory:!!(b[at+25]&2)});
    const find=async(dir,name)=>{if(!dir.directory)bad();const b=await read(dir.offset,dir.length);for(let at=0;at<b.length;){const len=b[at];if(!len){at=(Math.floor(at/2048)+1)*2048;continue;}if(len<34||at+len>b.length||b[at+32]>len-33)bad();const n=str(b.slice(at+33,at+33+b[at+32])).replace(/;\d+$/,'');if(n===name)return record(b,at);at+=len;}bad();};
    const game=await find({...record(pvd,156),directory:true},'PS3_GAME'),sfo=await find(game,'PARAM.SFO'),usr=await find(game,'USRDIR'),boot=await find(usr,'EBOOT.BIN');
    if(sfo.directory||boot.directory||sfo.length<20||boot.length<32)bad();
    const psf=await read(sfo.offset,20),sce=await read(boot.offset,32);
    if(str(psf.slice(0,4))!=='\x00PSF'||str(sce.slice(0,4))!=='SCE\x00')bad();
    return true;
  }
  async function verifyGameFile(client,file,platform){
    if(['ps1','saturn','segacd'].includes(platform)&&file.cueCompanion){if(!Number.isSafeInteger(Number(file.size))||file.size<=0||ext(file.name)!=='bin')fail('Invalid CUE track');return true;}

    if(PLATFORMS[platform]?.core){
      const size=Number(file.size),e=ext(file.name),b=await client.bytes(file.path,Math.min(65536,size));
      const text=(at,n)=>String.fromCharCode.apply(null,Array.from(b.slice(at,at+n))),v=new DataView(b.buffer,b.byteOffset,b.byteLength);
      const bad=()=>fail('Invalid '+platform+' game file / ملف اللعبة لا يطابق المحاكي');
      if(!Number.isSafeInteger(size)||size<64||b.length<Math.min(64,size)||/^\s*(?:<html|<!doctype|<\?xml|\{\s*"error)/i.test(text(0,80)))bad();
      if(['chd','cue','iso'].includes(e)&&!['gamecube','wii'].includes(platform))return verifyHeader(file.name,b,size);
      if(e==='zip'){if(!PLATFORMS[platform].keepArchive||text(0,4)!=='PK\x03\x04'||size<128)bad();const tail=await client.bytes(file.path,Math.min(65557,size),Math.max(0,size-65557));let end=-1;for(let i=tail.length-22;i>=0;i--)if(tail[i]===80&&tail[i+1]===75&&tail[i+2]===5&&tail[i+3]===6){end=i;break;}if(end<0)bad();return true;}
      if(e==='nes'){if(text(0,4)!=='NES\x1a'||size<16+b[4]*16384+b[5]*8192+(b[6]&4?512:0))bad();}
      else if(['z64','n64','v64'].includes(e)){if(![0x80371240,0x40123780,0x37804012].includes(v.getUint32(0,false))||size<4096)bad();}
      else if(e==='nds'){if(size<512||v.getUint32(0x20,true)<512||v.getUint32(0x20,true)+v.getUint32(0x2c,true)>size||v.getUint32(0x30,true)+v.getUint32(0x3c,true)>size)bad();}
      else if(['3ds','cci','cxi'].includes(e)){if(size<512||!['NCSD','NCCH'].includes(text(0x100,4)))bad();}
      else if(e==='rvz'){if(text(0,4)!=='RVZ\x01')bad();}
      else if(['gamecube','wii'].includes(platform)&&['gcm','iso'].includes(e)){if(v.getUint32(platform==='wii'?0x18:0x1c,false)!==(platform==='wii'?0x5d1c9ea3:0xc2339f3d))bad();}
      else if(e==='wbfs'){if(text(0,4)!=='WBFS')bad();}
      else if(e==='pbp'){if(text(0,4)!=='\x00PBP')bad();}
      else if(e==='cso'){if(!['CISO','ZISO'].includes(text(0,4)))bad();}
      else if(['gb','gbc'].includes(e)){if(size<32768||b.length<0x150||b[0x104]!==0xce||b[0x105]!==0xed)bad();}
      else if(e==='gba'){if(size<192||b[0xb2]!==0x96)bad();}
      else if(['sfc','smc','md','bin','gen','smd','sms','gg'].includes(e)){if(size<1024)bad();}
      else bad();
      return true;
    }
    if(platform==='ps3')return ext(file.name)==='pkg'?verifyPS3Package(await client.bytes(file.path,128),Number(file.size)):verifyPS3ISO(client,file);
    if(platform!=='xbox'&&platform!=='xbox360')return verifyHeader(file.name,await client.bytes(file.path,headerLimit(file.name)),Number(file.size));
    const offsets=platform==='xbox'?[0]:XBOX_OFFSETS;
    for(const partition of offsets){
      if(partition+67584>Number(file.size))continue;
      const data=await client.bytes(file.path,2048,partition+65536);
      if(String.fromCharCode.apply(null,Array.from(data.slice(0,20)))!=='MICROSOFT*XBOX*MEDIA')continue;
      return verifyXboxSector(data,Number(file.size),partition);
    }
    fail(platform==='xbox'?'Requires Xbox XISO; full Redump ISO must be converted first / يلزم ملف XISO.':'Invalid Xbox 360 ISO / قرص Xbox 360 غير صحيح.');
  }
  function hexId() {
    const array = new Uint8Array(8);
    root.crypto.getRandomValues(array);
    return Array.from(array, v => v.toString(16).padStart(2, '0')).join('');
  }
  const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
  class Client {
    constructor(config, fetcher) {
      this.wfm = origin(config.wfm);
      this.rpcURL = origin(config.aria) + '/jsonrpc';
      this.secret = config.secret || '';
      this.fetch = fetcher || root.fetch.bind(root);
    }
    async request(url, options) {
      const controller = new AbortController();
      const timer = setTimeout(() => controller.abort(), 15000);
      try {
        const response = await this.fetch(url, Object.assign({ cache: 'no-store', signal: controller.signal }, options));
        const data = await response.json();
        if (!response.ok || data.ok === false || data.error) {
          const error = new Error(data.error && (data.error.message || data.error) || 'HTTP ' + response.status);
          error.status = response.status; error.code = data.error_code;
          throw error;
        }
        return data;
      } finally { clearTimeout(timer); }
    }
    async rpc(method, params) {
      const args = this.secret ? ['token:' + this.secret].concat(params || []) : params || [];
      try {
        const response = await this.request(this.rpcURL, { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify({ jsonrpc: '2.0', id: 'ps2-library', method: 'aria2.' + method, params: args }) });
        return response.result;
      } catch (error) {
        // A missing RPC response does not prove that the engine rejected a mutation.
        error.rpcMethod = method;
        error.rpcTransport = !error.status && (/^(?:HTTP failed|Local service unavailable|Local HTTP (?:response timeout|receive failed|send failed))$/.test(error.message) || error.name === 'AbortError' || /fetch failed|Failed to fetch|NetworkError/i.test(error.message));
        throw error;
      }
    }
    api(route, values, form) {
      const query = new URLSearchParams(values || {}).toString();
      return this.request(this.wfm + '/api/' + route + (!form && query ? '?' + query : ''), form ? { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: query } : {});
    }
    async list(path) { return (await this.api('list', { path: path })).entries; }
    async ensureDir(path) {
      const usb = path.match(/^\/mnt\/(usb[0-7])(?:\/|$)/);
      if (usb && !(await this.list('/mnt')).some(e => e.name === usb[1] && e.type === 'd')) fail('قرص USB المحدد غير متصل.');
      const parts = path.split('/').filter(Boolean);
      let parent = '';
      for (const part of parts) {
        filename(part);
        const entries = await this.list(parent || '/');
        const entry = entries.find(e => e.name === part);
        if (entry && entry.type !== 'd') fail('المسار ليس مجلدًا عاديًا: ' + parent + '/' + part);
        if (!entry) await this.api('mkdir', { path: parent || '/', name: part });
        parent += '/' + part;
      }
    }
    async available(path) {
      const result = await this.api('space', { path: path || GAMES });
      const space = result.spaces.find(s => s.path === '/data') || result.spaces.find(s => s.current);
      if (!space) fail('تعذر معرفة المساحة الحرة.');
      return Number(space.free);
    }
    async add(url, dir, name, gid, checksum, mirrors = []) {
      if (!staged(dir)) fail('مجلد التنزيل خارج المسار المؤقت.');
      filename(name);
      const connections = String(Math.min(16, Math.max(1, Number(this.connections) || 16)));
      const options = { dir: dir, out: name, gid: gid, continue: 'true', 'allow-overwrite': 'false', 'auto-file-renaming': 'false', 'content-disposition': 'false', 'follow-torrent': 'false', 'follow-metalink': 'false', 'max-connection-per-server': connections, split: connections, 'min-split-size': '1M', 'max-tries': '5', 'retry-wait': '5', 'connect-timeout': '15', timeout: '45', 'max-download-limit': '0', 'file-allocation': 'none', 'enable-http-keep-alive': 'true', 'uri-selector': checksum && mirrors.length ? 'adaptive' : 'feedback' };
      if (checksum) options.checksum = checksum;
      return this.rpc('addUri', [[...new Set([url, ...(checksum?mirrors:[])].map(webURL))], options]);
    }
    async bytes(path, max, start = 0) {
      if(!Number.isSafeInteger(start)||start<0||!Number.isInteger(max)||max<1||max>2097152)fail('Invalid file range');
      if (!staged(path) && !Object.values(PLATFORMS).some(p => inside(path, p.games)) && !(this.destination && inside(path, destinationPath(this.destination)))) fail('مسار القراءة غير مسموح.');
      // WFM may ignore HTTP Range. Native Raff can seek locally, with the same
      // destination checks above and nativeFS's regular-file / symlink guards.
      if(start>0&&typeof root.nativeFS==='function'){
        if(max>1048576)fail('Native file slice limit');
        return new Uint8Array(root.nativeFS('slice',path,{offset:start,length:max}));
      }
      const controller = new AbortController();
      const timer = setTimeout(() => controller.abort(), 15000);
      try {
        const response = await this.fetch(this.wfm + '/fs?path=' + encodeURIComponent(path), { headers: { Range: 'bytes=' + start + '-' + (start + max - 1) }, signal: controller.signal });
        if (response.status !== 200 && response.status !== 206) { controller.abort(); fail('تعذرت قراءة الملف: HTTP ' + response.status); }
        if(start>0){const range=response.headers.get('Content-Range')||'';if(response.status!==206||!range.startsWith('bytes '+start+'-')){controller.abort();fail('WFM did not honor the requested file offset');}}
        // Older WFM versions ignore Range. Bound the stream and cancel after the header.
        if (response.body && response.body.getReader) {
          const reader = response.body.getReader(), chunks = []; let length = 0;
          while (length < max) {
            const part = await reader.read(); if (part.done) break;
            const chunk = part.value.subarray(0, max - length); chunks.push(chunk); length += chunk.length;
          }
          try { await reader.cancel(); } catch (_) {}
          const bytes = new Uint8Array(length); let offset = 0;
          chunks.forEach(chunk => { bytes.set(chunk, offset); offset += chunk.length; });
          return bytes;
        }
        const length = Number(response.headers.get('Content-Length'));
        if (!length || length > max) { controller.abort(); fail('نسخة المتصفح أو مدير الملفات تحتاج دعم قراءة جزء من الملف.'); }
        const bytes = new Uint8Array(await response.arrayBuffer());
        if (bytes.length > max) fail('استجابة الملف أكبر من الحد المطلوب.');
        return bytes;
      } finally { clearTimeout(timer); }
    }
    async scan(path, rootPath, depth, count, platform) {
      const p = platformFor(platform);
      if (depth > 8 || !inside(path, p.staging)) fail('بنية الأرشيف أعمق من الحد المدعوم.');
      const entries = await this.list(path);
      const found = [];
      for (const entry of entries) {
        if (++count.value > 2000) fail('الأرشيف يحتوي على ملفات كثيرة جدًا.');
        filename(entry.name);
        if (entry.path !== path + '/' + entry.name || !inside(entry.path, rootPath)) fail('مسار غير متوقع داخل الأرشيف.');
        if (entry.type === 'l') fail('الأرشيف يحتوي على رابط رمزي؛ لم تُنقل الملفات للمحاكي.');
        if (entry.type === 'd') found.push.apply(found, await this.scan(entry.path, rootPath, depth + 1, count, p.id));
        else if (entry.type === '-' && p.formats.includes(ext(entry.name))) found.push(entry);
      }
      return found;
    }
  }
  class Engine {
    constructor(client, save, update) {
      this.client = client; this.save = save; this.update = update || function () {}; this.job = null; this.running = false; this.stopRequested = false;
    }
    persist() { this.save(this.job); this.update(this.job); }
    async begin(input) {
      if (this.running || (this.job && this.job.phase !== 'done' && this.job.phase !== 'canceled')) fail('أكمل المهمة الحالية أو ألغها أولًا.');
      const game = validateGame(input);
      const id = hexId();
      this.job = { id: id, game: game, dir: platformFor(game.platform).staging + '/' + id, phase: 'prepare', endpoint: this.client.wfm, rpcURL: this.client.rpcURL, created: new Date().toISOString(), moved: [], progress: 0 };
      this.persist();
      return this.resume();
    }
    async downloadStatus(gid, expected) {
      while (!this.stopRequested) {
        const state = await this.client.rpc('tellStatus', [gid]);
        if (state.files && state.files.some(f => f.path !== expected)) fail('مسار التنزيل لا يطابق المهمة.');
        this.job.progress = Number(state.totalLength) ? Number(state.completedLength) / Number(state.totalLength) : 0;
        this.job.speed = Number(state.downloadSpeed); this.job.total = Number(state.totalLength); this.job.received = Number(state.completedLength); this.update(this.job);
        if (state.status === 'complete') {
          if (!(Number(state.totalLength) > 0) || state.completedLength !== state.totalLength) fail('التنزيل غير مكتمل.');
          return;
        }
        if (state.status === 'error' || state.status === 'removed') fail('تعذر التنزيل: ' + (state.errorMessage || state.status));
        if (state.status === 'paused') fail('التنزيل متوقف مؤقتًا. اضغط استئناف.');
        await sleep(900);
      }
      fail('توقفت متابعة المهمة.');
    }
    async waitTask(op, src, dst) {
      while (!this.stopRequested) {
        const result = await this.client.api('tasks');
        const task = result.tasks.find(t => t.id === this.job.taskId);
        if (!task) {
          // WFM reports a completed task once; another open WFM tab can consume it.
          const completion = result.completion;
          if (completion && completion.id === this.job.taskId && completion.op === op && completion.src === src) return;
          if (op === 'move') {
            const file = this.job.files[this.job.index];
            const sourceEntries = await this.client.list(src.slice(0, src.lastIndexOf('/')));
            const target = (await this.client.list(dst.slice(0, dst.lastIndexOf('/')))).find(e => e.name === file.name);
            if (!sourceEntries.some(e => e.path === src) && target && target.type === '-' && Number(target.size) === Number(file.size)) {
              await verifyGameFile(this.client,{...target,path:dst},this.job.game.platform);
              return;
            }
          }
          fail('لم يحتفظ مدير الملفات بنتيجة العملية. أغلق تبويبات مدير الملفات الأخرى وافحص المجلد المؤقت. لن ننقل أرشيفًا دون تأكيد اكتماله.');
        }
        if (task.op !== op || task.src !== src || task.dst !== dst) fail('سجل العملية لا يطابق المهمة الحالية.');
        this.job.progress = task.total ? task.done / task.total : 0; this.update(this.job);
        if (task.state === 'done') return;
        if (task.state === 'failed' || task.state === 'canceled') fail(task.error_code === 'archive_helper_not_running' ? 'أداة فك الضغط غير شغّالة.' : task.error || 'فشلت العملية.');
        await sleep(100);
      }
      fail('توقفت متابعة المهمة.');
    }
    async resume() {
      if (this.running || !this.job) return;
      const j = this.job, c = this.client;
      if (j.endpoint !== c.wfm || j.rpcURL !== c.rpcURL) fail('هذه المهمة مرتبطة بعنوان جهاز آخر.');
      const p = platformFor(j.game.platform), destination = destinationFor(j.game);
      c.destination = destination;
      if (j.dir && !inside(j.dir, p.staging)) fail('مسار المهمة لا يطابق المنصة.');
      this.running = true; this.stopRequested = false; delete j.error; this.persist();
      try {
        while (!this.stopRequested) {
          if (j.phase === 'prepare') {
            if (!j.dir || !inside(j.dir, p.staging)) fail('مسار المهمة لا يطابق المنصة.');
            await c.ensureDir(destination); await c.ensureDir(j.dir);
            const reserve = Math.max(j.game.sizeBytes, 64 * MiB) + Number(j.game.unpackedBytes || 0) + 256 * MiB;
            if (await c.available(destination) < reserve) fail('المساحة الحرة لا تكفي للتنزيل وفك الضغط.');
            if (!j.game.url) {
              j.gid = hexId(); j.phase = 'resolving'; this.persist();
              const metadataURL = j.game.source === 'dlarchive' ? 'https://uptobox.dlarchive.my.id/files_api.php?action=file-url&id=' + encodeURIComponent(j.game.fileId) : j.game.sourcePage + '/download';
              await c.add(metadataURL, j.dir, 'source.html', j.gid);
            } else { j.phase = 'ready'; this.persist(); }
          } else if (j.phase === 'resolving') {
            await this.downloadStatus(j.gid, j.dir + '/source.html');
            const metadata = (await c.list(j.dir)).find(e => e.name === 'source.html');
            if (!metadata || metadata.size > 2 * MiB) fail('صفحة المصدر غير متوقعة أو كبيرة جدًا.');
            const html = new TextDecoder().decode(await c.bytes(j.dir + '/source.html', 2 * MiB));
            Object.assign(j.game, parseDownload(html, j.game.source || sourceFor(j.game.sourcePage))); validateGame(j.game);
            j.phase = 'ready'; this.persist();
          } else if (j.phase === 'ready') {
            if ((await c.list(destination)).some(e => e.name === j.game.filename)) fail('يوجد ملف بهذا الاسم في مجلد الألعاب؛ لم تتم الكتابة فوقه.');
            j.gid = hexId(); j.phase = 'downloading'; this.persist();
            await c.add(j.game.url, j.dir, j.game.filename, j.gid, j.game.checksum, j.game.mirrors);
          } else if (j.phase === 'downloading') {
            const state = await c.rpc('tellStatus', [j.gid]);
            if (state.status === 'error') {
              if (state.files && state.files.some(f => f.path !== j.dir + '/' + j.game.filename)) fail('مسار التنزيل لا يطابق المهمة.');
              await c.rpc('removeDownloadResult', [j.gid]);
              j.gid = hexId(); this.persist();
              // Keep the partial file and .aria2 control file; retry from completed segments.
              await c.add(j.game.url, j.dir, j.game.filename, j.gid, j.game.checksum, j.game.mirrors);
            }
            if (state.status === 'paused') await c.rpc('unpause', [j.gid]);
            await this.downloadStatus(j.gid, j.dir + '/' + j.game.filename);
            j.phase = !p.keepArchive && /\.(zip|7z|rar)$/i.test(j.game.filename) ? 'extract-ready' : 'scan'; this.persist();
          } else if (j.phase === 'extract-ready') {
            if (await c.available(destination) < Number(j.game.unpackedBytes) + 128 * MiB) fail('المساحة المتبقية لا تكفي لفك الضغط.');
            j.extractDir = j.extractDir || j.dir + '/extracted'; if(!inside(j.extractDir,j.dir))fail('مسار المهمة لا يطابق المنصة.'); await c.ensureDir(j.extractDir);
            j.phase = 'extract-request'; this.persist();
            j.currentArchive=j.archives?j.archives[j.archiveIndex].path:j.dir + '/' + j.game.filename;
            const task = await c.api('extract', { paths: j.currentArchive, destination: j.extractDir, separate: '0', overwrite: '0' }, true);
            j.taskId = task.task_id; j.phase = 'extracting'; this.persist();
          } else if (j.phase === 'extracting') {
            await this.waitTask('extract', j.currentArchive || j.dir + '/' + j.game.filename, j.extractDir);
            if(j.archives)j.archiveIndex++;
            j.phase = j.archives&&j.archiveIndex<j.archives.length?'extract-ready':'scan'; this.persist();
          } else if (j.phase === 'scan') {
            const scanPath = j.extractDir || j.dir;
            j.files = j.inputs ? j.inputs.filter(f=>p.formats.includes(ext(f.name))) : await c.scan(scanPath, scanPath, 0, { value: 0 }, p.id);
            if(j.inputs&&j.extractDir)j.files.push(...await c.scan(j.extractDir,j.extractDir,0,{value:0},p.id));
            if (!j.files.length) fail('لم يوجد ملف لعبة للصيغة المختارة في التنزيل.');
            if(['ps1','saturn','segacd'].includes(p.id))for(const cue of [...j.files].filter(f=>ext(f.name)==='cue')){
              if(cue.size>65536)fail('CUE is too large');
              const content=new TextDecoder().decode(await c.bytes(cue.path,65536)),folder=cue.path.slice(0,cue.path.lastIndexOf('/')),entries=await c.list(folder);let count=0;
              for(const line of content.split(/\r?\n/)){if(!/^\s*FILE\b/i.test(line))continue;
                const match=line.match(/^\s*FILE\s+(?:"([^"]+)"|(\S+))\s+BINARY\s*$/i),name=match&&(match[1]||match[2]);
                if(!name||/[\\/]/.test(name)||ext(name)!=='bin')fail('CUE track must be a BIN in the same folder');filename(name);
                const track=entries.find(f=>f.name===name&&f.type==='-'&&f.path===folder+'/'+name&&f.size>0);if(!track||!inside(track.path,j.dir))fail('Missing CUE track: '+name);
                if(!j.files.some(f=>f.path===track.path))j.files.push({...track,cueCompanion:true});count++;
              }if(!count||!/^\s*TRACK\s+\d+\s+/im.test(content))fail('CUE has no complete tracks');
            }
            const names = new Set(); const existing = await c.list(destination);
            for (const file of j.files) {
              if (names.has(file.name) || existing.some(e => e.name === file.name)) fail('اسم ملف مكرر: ' + file.name + '. بقيت الملفات في المجلد المؤقت.');
              names.add(file.name);
              if (!(p.formats.includes(ext(file.name))||['ps1','saturn','segacd'].includes(p.id)&&file.cueCompanion&&ext(file.name)==='bin') || !inside(file.path, j.dir)) fail('مسار المهمة لا يطابق المنصة.');
              await verifyGameFile(c,file,p.id);
            }
            j.index = 0; j.phase = 'move-ready'; this.persist();
          } else if (j.phase === 'move-ready') {
            if (j.index >= j.files.length) { j.phase = ['ps4','ps5'].includes(p.id) && j.installOptions?.enabled && j.files.some(f => /\.pkg$/i.test(f.name)) ? 'pkg-ready' : 'done'; j.progress = j.phase === 'done' ? 1 : 0; this.persist(); if (j.phase === 'done') return; continue; }
            const file = j.files[j.index];
            j.phase = 'move-request'; this.persist();
            if (!(p.formats.includes(ext(file.name))||['ps1','saturn','segacd'].includes(p.id)&&file.cueCompanion&&ext(file.name)==='bin') || !inside(file.path, j.dir)) fail('مسار المهمة لا يطابق المنصة.');
            const task = await c.api('move', { paths: file.path, dst: destination, overwrite: '0' }, true);
            j.taskId = task.task_id; j.phase = 'moving'; this.persist();
          } else if (j.phase === 'moving') {
            const file = j.files[j.index];
            await this.waitTask('move', file.path, destination + '/' + file.name);
            const target = (await c.list(destination)).find(e => e.name === file.name);
            if (!target || target.type !== '-' || Number(target.size) !== Number(file.size)) fail('تعذر التحقق من الملف المنقول.');
            j.moved.push(destination + '/' + file.name); j.index++; j.phase = 'move-ready'; this.persist();
          } else if (j.phase.startsWith('pkg-')) {
            if (!c.installPackages) fail('Package installer unavailable; PKG kept.');
            await c.installPackages(j, () => this.persist(), () => this.update(j), () => this.stopRequested);
            return;
          } else if (j.phase === 'extract-request' || j.phase === 'move-request') {
            fail('انقطع الاتصال أثناء إرسال عملية ملفات. افحص مدير الملفات؛ لن نكرر العملية تلقائيًا.');
          } else return;
        }
      } catch (error) { j.error = error.message; this.persist(); throw error; }
      finally { this.running = false; this.update(j); }
    }
    async pause() {
      if (!this.job || !['downloading', 'resolving'].includes(this.job.phase)) fail('الإيقاف المؤقت متاح أثناء التنزيل فقط.');
      await this.client.rpc('pause', [this.job.gid]);
    }
    async cancel() {
      if (!this.job) return;
      const j = this.job;
      if (j.phase === 'extract-request' || j.phase === 'move-request') fail('افحص العملية في مدير الملفات أولًا؛ حالتها غير مؤكدة.');
      if (['downloading', 'resolving'].includes(j.phase)) {
        const state = await this.client.rpc('tellStatus', [j.gid]);
        if (['active', 'waiting', 'paused'].includes(state.status)) await this.client.rpc('remove', [j.gid]);
      } else if (['extracting', 'moving'].includes(j.phase)) {
        const task = (await this.client.api('tasks')).tasks.find(t => t.id === j.taskId);
        if (task && ['running', 'queued'].includes(task.state)) await this.client.api('cancel', { id: j.taskId });
      }
      this.stopRequested = true;
      while (this.running) await sleep(100);
      j.phase = 'canceled'; delete j.error; this.persist();
    }
  }
  const api = { verifyPS3Package, verifyPS3ISO, verifyGameFile, verifyXboxSector, XBOX_OFFSETS, Client: Client, Engine: Engine, validateGame: validateGame, destinationPath, destinationFor, parseDownload: parseDownload, verifyHeader: verifyHeader, headerLimit: headerLimit, filename: filename, inside: inside, sourceFor: sourceFor, SOURCES: SOURCES, PLATFORMS: PLATFORMS, platformFor: platformFor, GAMES: GAMES, STAGING: STAGING };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.PS2Library = api;
})(typeof globalThis !== 'undefined' ? globalThis : window);
