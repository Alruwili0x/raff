"""Build a reproducible, searchable metadata catalog. No game content is included."""
from pathlib import Path
import hashlib, json, re, sqlite3, time, unicodedata, os
from rdb_reader import records

ROOT = Path(__file__).resolve().parents[1]
CACHE = Path(os.environ.get('RAFF_PROVIDER_CACHE', ROOT / 'work/providers'))
OUT = ROOT / 'assets/library.sqlite'
def compact(v): return json.dumps(v, ensure_ascii=False, separators=(',', ':'))
def identity(*v): return hashlib.sha256('\0'.join(str(x) for x in v).encode()).hexdigest()[:32]
def normalized(v): return ' '.join(re.findall(r'\w+', unicodedata.normalize('NFKC', v).casefold()))
def slug(v): return re.sub(r'[^a-z0-9]+', '-', v.lower()).strip('-') or identity(v)
def plain(v):
    if isinstance(v, bytes): return v.hex()
    if isinstance(v, dict): return {k:plain(x) for k,x in v.items()}
    if isinstance(v, list): return [plain(x) for x in v]
    return v

def read_info(path):
    out={}
    for line in path.read_text(encoding='utf-8-sig').splitlines():
        m=re.match(r'^\s*(\w+)\s*=\s*(.*?)\s*$', line)
        if not m: continue
        value=m[2]
        if value.startswith('"'):
            try:value=json.loads(value)
            except ValueError:value=value.strip('"')
        out[m[1]]=value
    return out

# Remove only known release qualifiers, never fuzzy-match titles or remove arbitrary subtitles.
REGIONS={'USA','Europe','Japan','World','Asia','Australia','Brazil','Canada','China','France','Germany','Hong Kong','Italy','Korea','Netherlands','Russia','Spain','Sweden','Taiwan','United Kingdom','United States'}
def release_parts(name):
    region=[];revision=[];discs=[];languages=[]
    def replace(m):
        x=m[1]; parts=x.split(', ')
        if all(p in REGIONS for p in parts):region.extend(parts);return ''
        if re.fullmatch(r'Disc [0-9]+(?: of [0-9]+)?',x,re.I):discs.append(x);return ''
        if re.fullmatch(r'(?:Rev [A-Za-z0-9.]+|v[0-9]+(?:\.[0-9]+)+)',x):revision.append(x);return ''
        if re.fullmatch(r'(?:En|Ja|Fr|De|Es|It|Nl|Pt|Sv|No|Da|Fi|Ko|Zh|Ar)(?:,(?:En|Ja|Fr|De|Es|It|Nl|Pt|Sv|No|Da|Fi|Ko|Zh|Ar))*',x):languages.extend(x.split(','));return ''
        return m[0]
    title=' '.join(re.sub(r'\(([^()]*)\)',replace,name).split())
    return title,region,revision,discs,languages

sync=json.loads((CACHE/'sync.json').read_text())
if sync['failures'] or any(s['total']!=s['completed'] for s in sync['sources'].values()):
    raise SystemExit('Provider synchronization is incomplete; refusing a completeness claim')
temporary=OUT.with_suffix('.sqlite.new')
if temporary.exists():temporary.unlink()
db=sqlite3.connect(temporary)
db.executescript('''
PRAGMA journal_mode=OFF;
PRAGMA synchronous=OFF;
PRAGMA user_version=9;
CREATE TABLE meta(key TEXT PRIMARY KEY,value TEXT NOT NULL);
CREATE TABLE platforms(id TEXT PRIMARY KEY,name TEXT NOT NULL,family TEXT NOT NULL,kind TEXT NOT NULL,cores TEXT NOT NULL,source TEXT NOT NULL,release_count INTEGER NOT NULL DEFAULT 0,game_count INTEGER NOT NULL DEFAULT 0);
CREATE TABLE cores(id TEXT PRIMARY KEY,name TEXT NOT NULL,version TEXT,systems TEXT NOT NULL,extensions TEXT NOT NULL,firmware TEXT NOT NULL,info TEXT NOT NULL,source TEXT NOT NULL,commit_hash TEXT NOT NULL,blob_hash TEXT NOT NULL);
CREATE TABLE games(id TEXT PRIMARY KEY,platform TEXT NOT NULL,title TEXT NOT NULL,search_title TEXT NOT NULL,cover TEXT,metadata TEXT NOT NULL,legacy_key TEXT,added_at TEXT NOT NULL);
CREATE INDEX games_platform_title ON games(platform,search_title,id);
CREATE UNIQUE INDEX games_legacy_key ON games(legacy_key) WHERE legacy_key IS NOT NULL;
CREATE VIRTUAL TABLE search USING fts5(title,content='games',content_rowid='rowid',tokenize='unicode61 remove_diacritics 2',prefix='2 3 4');
CREATE TABLE releases(id TEXT PRIMARY KEY,game_id TEXT NOT NULL,source_key TEXT NOT NULL,region TEXT,revision TEXT,languages TEXT,serial TEXT,metadata TEXT NOT NULL);
CREATE INDEX releases_game ON releases(game_id);
CREATE TABLE disc_sets(id TEXT PRIMARY KEY,release_id TEXT NOT NULL,label TEXT,disc_number INTEGER);
CREATE TABLE assets(id TEXT PRIMARY KEY,release_id TEXT NOT NULL,disc_set_id TEXT,filename TEXT,size INTEGER,format TEXT,crc TEXT,md5 TEXT,sha1 TEXT,download_option TEXT,source_id TEXT NOT NULL);
CREATE INDEX assets_release ON assets(release_id);
CREATE INDEX assets_filename_size ON assets(filename,size);
CREATE INDEX assets_crc ON assets(crc,size);
CREATE INDEX assets_sha1 ON assets(sha1);
CREATE TABLE sources(id TEXT PRIMARY KEY,url TEXT NOT NULL,revision TEXT,authorization TEXT NOT NULL,metadata_only INTEGER NOT NULL);
CREATE TABLE compatibility(id TEXT PRIMARY KEY,release_id TEXT NOT NULL,core_id TEXT NOT NULL,core_version TEXT NOT NULL,target TEXT NOT NULL,configuration TEXT NOT NULL,status TEXT NOT NULL,evidence_url TEXT NOT NULL,checked_at TEXT NOT NULL);
CREATE TABLE coverage(platform TEXT PRIMARY KEY,source TEXT NOT NULL,expected INTEGER NOT NULL,imported INTEGER NOT NULL,status TEXT NOT NULL,error TEXT);
CREATE TABLE unnamed_metadata(platform TEXT NOT NULL,record_number INTEGER NOT NULL,metadata TEXT NOT NULL,PRIMARY KEY(platform,record_number));
''')
core_commit=sync['sources']['libretro-core-info']['commit'];db_commit=sync['sources']['libretro-database']['commit']
now=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
platforms={}; cores=[]
for path in sorted((CACHE/'cores').glob('*.info')):
    if path.name.startswith('00_example'):continue
    v=read_info(path); cid=path.stem; names=[n for n in v.get('database','').split('|') if n]
    if not names:names=[v.get('manufacturer','')+' - '+v.get('systemname',v.get('corename',cid))]
    firmware=[]
    for n in range(int(v.get('firmware_count','0'))):
        prefix=f'firmware{n}_';fname=v.get(prefix+'path','');md5=re.search(re.escape(fname)+r'\s*\(md5\):\s*([a-f0-9]{32})',v.get('notes',''),re.I)
        firmware.append({'path':fname,'description':v.get(prefix+'desc',''),'optional':v.get(prefix+'opt')=='true','md5':md5[1].lower() if md5 else None,'requirementEvidence':'core-info','coreVersion':v.get('display_version')})
    kind='engine' if 'Game' in v.get('categories','') else 'arcade' if any('MAME' in n or 'FinalBurn' in n for n in names) else 'emulator'
    for name in names:
        pid=slug(name);entry=platforms.setdefault(pid,{'name':name,'family':v.get('manufacturer','Other') or 'Other','kind':kind,'cores':[]});entry['cores'].append(cid)
    source='https://github.com/libretro/libretro-core-info/blob/'+core_commit+'/'+path.name
    blob=hashlib.sha1(b'blob '+str(path.stat().st_size).encode()+b'\0'+path.read_bytes()).hexdigest()
    db.execute('INSERT INTO cores VALUES(?,?,?,?,?,?,?,?,?,?)',(cid,v.get('display_name',cid),v.get('display_version'),compact([slug(n) for n in names]),compact(v.get('supported_extensions','').split('|')),compact(firmware),compact(v),source,core_commit,blob))
    cores.append({'id':cid,'name':v.get('display_name',cid),'systems':[slug(n) for n in names],'extensions':v.get('supported_extensions','').split('|'),'firmware':firmware,'version':v.get('display_version'),'source':source,'info':v})
source='libretro-database'
db.execute('INSERT INTO sources VALUES(?,?,?,?,?)',(source,'https://github.com/libretro/libretro-database',db_commit,'metadata-license',1))
total=0
for path in sorted((CACHE/'rdb').glob('*.rdb')):
    name=path.stem;pid=slug(name);platforms.setdefault(pid,{'name':name,'family':name.split(' - ')[0] if ' - ' in name else 'Other','kind':'emulator','cores':[]})
    count=0; unnamed=0
    for row in records(path):
        row=plain(row);name=row.get('name');count+=1
        if not name:
            db.execute('INSERT INTO unnamed_metadata VALUES(?,?,?)',(pid,count,compact(row)));unnamed+=1;continue
        title,regions,revisions,discs,languages=release_parts(name)
        gid=identity('libretro-game',pid,title);rid=identity('libretro-release',pid,name,row.get('serial',''),row.get('sha1',row.get('crc','')))
        metadata={'provider':'Libretro database','providerRevision':db_commit,'sourceTitle':name,'database':path.stem,'developer':row.get('developer'),'publisher':row.get('publisher'),'genre':row.get('genre'),'year':row.get('releaseyear'),'month':row.get('releasemonth'),'day':row.get('releaseday'),'players':row.get('users'),'rating':None,'descriptionAr':None,'description':row.get('description') if row.get('description')!=name else None,'screenshots':[],'compatibility':'untested'}
        db.execute('INSERT OR IGNORE INTO games VALUES(?,?,?,?,?,?,?,?)',(gid,pid,title,normalized(title),None,compact(metadata),None,now))
        db.execute('INSERT OR IGNORE INTO releases VALUES(?,?,?,?,?,?,?,?)',(rid,gid,name,compact(regions),compact(revisions),compact({'interface':None,'subtitles':None,'audio':None,'reportedCodes':languages,'evidence':'release-name'}),row.get('serial'),compact(row)))
        discid=identity('disc',rid);number=re.search(r'\d+',discs[0]) if discs else None
        db.execute('INSERT OR IGNORE INTO disc_sets VALUES(?,?,?,?)',(discid,rid,discs[0] if discs else None,int(number[0]) if number else None))
        filename=row.get('rom_name');ext=filename.rsplit('.',1)[-1].lower() if filename and '.' in filename else None
        db.execute('INSERT OR IGNORE INTO assets VALUES(?,?,?,?,?,?,?,?,?,?,?)',(identity('asset',rid,filename),rid,discid,filename,row.get('size'),ext,row.get('crc'),row.get('md5'),row.get('sha1'),None,source))
    total+=count
    db.execute('INSERT INTO coverage VALUES(?,?,?,?,?,?)',(pid,source,count,count,'complete-with-unnamed-metadata' if unnamed else 'complete-pinned-source',f'{unnamed} hash-only records retained without inventing game titles' if unnamed else None));db.commit()
    print(path.stem,count,flush=True)

# Preserve existing PlayStation/Switch identities and download options, without treating URLs as a distribution license.
legacy=json.loads((ROOT/'assets/catalog.json').read_text());cards=legacy if isinstance(legacy,list) else legacy.get('games',[])
full=json.loads((ROOT/'assets/catalog-full.json').read_text())['games'];options={g['id']:g for g in full}
for pid,title in [('ps2','PlayStation 2'),('ps4','PlayStation 4'),('ps5','PlayStation 5'),('switch','Nintendo Switch')]:platforms[pid]={'name':title,'family':'Sony' if pid!='switch' else 'Nintendo','kind':'standalone','cores':[]}
db.execute('INSERT INTO sources VALUES(?,?,?,?,?)',('legacy-user-sources','user-configured:raff-v0.8','0.8.0','user-confirmation-required',0))
for card in cards:
    pid=card.get('platform','ps2');key=card.get('key',card.get('id'));gid='legacy:'+str(key);title=card['title']
    meta={'provider':'Existing Raff catalog','legacy':card,'rating':card.get('rating'),'descriptionAr':None,'screenshots':[],'compatibility':'untested'}
    db.execute('INSERT OR IGNORE INTO games VALUES(?,?,?,?,?,?,?,?)',(gid,pid,title,normalized(title),card.get('cover'),compact(meta),str(key),now))
    for option in card.get('options',[]):
        v=options.get(option if isinstance(option,str) else option.get('id'),option if isinstance(option,dict) else {})
        if not v:continue
        rid='legacy-release:'+str(v['id']);db.execute('INSERT OR IGNORE INTO releases VALUES(?,?,?,?,?,?,?,?)',(rid,gid,str(v['id']),compact([v.get('region')] if v.get('region') else []),None,None,v.get('titleId'),compact(v)))
        filename=v.get('filename','');db.execute('INSERT OR IGNORE INTO assets VALUES(?,?,?,?,?,?,?,?,?,?,?)',('legacy-asset:'+str(v['id']),rid,None,filename,v.get('sizeBytes'),filename.rsplit('.',1)[-1].lower(),None,None,None,str(v['id']),'legacy-user-sources'))
for pid,v in platforms.items():
    count=db.execute('SELECT count(*) FROM games WHERE platform=?',(pid,)).fetchone()[0]
    release_count=db.execute('SELECT count(*) FROM releases r JOIN games g ON g.id=r.game_id WHERE g.platform=?',(pid,)).fetchone()[0]
    db.execute('INSERT INTO platforms VALUES(?,?,?,?,?,?,?,?)',(pid,v['name'],v['family'],v['kind'],compact(sorted(set(v['cores']))),source if v['kind']!='standalone' else 'legacy-user-sources',release_count,count))
db.execute('INSERT INTO search(rowid,title) SELECT rowid,title FROM games')
summary={'version':9,'generatedAt':now,'sources':sync['sources'],'coreCount':len(cores),'platformCount':len(platforms),'rdbRecords':total,'games':db.execute('SELECT count(*) FROM games').fetchone()[0],'releases':db.execute('SELECT count(*) FROM releases').fetchone()[0],'downloadableAuthorized':0,'verifiedPlayable':0,'failures':sync['failures'],'coverage':[dict(zip(['platform','name','games','releases'],row)) for row in db.execute('SELECT id,name,game_count,release_count FROM platforms ORDER BY name')]}
db.execute('INSERT INTO meta VALUES(?,?)',('catalog',compact(summary)));db.commit()
assert db.execute('PRAGMA integrity_check').fetchone()[0]=='ok'
db.executescript("CREATE INDEX IF NOT EXISTS games_title ON games(search_title,id);CREATE INDEX IF NOT EXISTS games_rating ON games(platform,coalesce(json_extract(metadata,'$.legacy.score'),-1) DESC,search_title,id);CREATE INDEX IF NOT EXISTS games_popular ON games(platform,coalesce(json_extract(metadata,'$.legacy.popularityScore'),json_extract(metadata,'$.legacy.downloads'),-1) DESC,search_title,id);");db.commit();
db.execute('VACUUM');db.close();temporary.replace(OUT)
(ROOT/'assets/core-registry.json').write_text(compact({'schema':1,'commit':core_commit,'cores':cores,'platforms':platforms}),encoding='utf8')
(ROOT/'catalog-coverage-v0.9.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf8')
print('Built',OUT,OUT.stat().st_size,'bytes',summary['games'],'games',summary['releases'],'releases')
