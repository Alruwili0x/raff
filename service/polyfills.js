// Small host adapters. HTTP transport is restricted to local PS5 services.
globalThis.console={log:(...v)=>nativeLog(v.join(' ')),error:(...v)=>nativeLog(v.join(' '))};
globalThis.crypto={getRandomValues(a){a.set(new Uint8Array(nativeRandom(a.byteLength)));return a;}};
globalThis.AbortController=class{constructor(){this.signal={aborted:false};}abort(){this.signal.aborted=true;}};
globalThis.URLSearchParams=class{
 constructor(v){this.items=[];if(typeof v==='string'){for(const part of v.replace(/^\?/,'').split('&'))if(part){const n=part.indexOf('=');this.items.push([decodeURIComponent((n<0?part:part.slice(0,n)).replace(/\+/g,' ')),decodeURIComponent((n<0?'':part.slice(n+1)).replace(/\+/g,' '))]);}}else for(const [k,x] of Object.entries(v||{}))this.items.push([k,String(x)]);}
 get(k){return(this.items.find(x=>x[0]===k)||[])[1]??null;}
 toString(){return this.items.map(([k,v])=>encodeURIComponent(k)+'='+encodeURIComponent(v)).join('&');}
};
globalThis.URL=class{
 constructor(v){const m=String(v).match(/^(https?):\/\/([^/?#]+)([^?#]*)(\?[^#]*)?(#.*)?$/i);if(!m||/[\x00-\x20\\]/.test(v))throw Error('Invalid URL');this.protocol=m[1].toLowerCase()+':';let host=m[2];this.username='';this.password='';if(host.includes('@')){this.username=host.split('@')[0];host=host.split('@').pop();}this.hostname=host.split(':')[0].toLowerCase();this.port=host.split(':')[1]||'';this.origin=this.protocol+'//'+host.toLowerCase();this.pathname=m[3]||'/';this.search=m[4]||'';this.hash=m[5]||'';this.searchParams=new URLSearchParams(this.search);this.href=this.origin+this.pathname+this.search+this.hash;}
};
globalThis.TextDecoder=class{decode(a){return nativeDecode(a.buffer.slice(a.byteOffset||0,(a.byteOffset||0)+a.byteLength));}};
let nextTimer=1;const timers=new Map();
globalThis.setTimeout=(fn,ms)=>{const id=nextTimer++;timers.set(id,{fn,at:Date.now()+Math.max(0,Number(ms)||0)});return id;};
globalThis.clearTimeout=id=>timers.delete(id);
globalThis.tick=()=>{const now=Date.now();for(const [id,t] of timers)if(t.at<=now){timers.delete(id);try{t.fn();}catch(e){console.error(e.stack||e);}}};
globalThis.fetch=async(url,options={})=>{
 if(options.signal?.aborted)throw Error('Request canceled');
 const headers=options.headers||{},range=String(headers.Range||'').match(/^bytes=0-(\d+)$/),max=range?Math.min(2097152,Number(range[1])+1):4194304;
 const r=nativeHTTP(String(url),options.method||'GET',options.body?String(options.body):'',headers['Content-Type']||'',max,Boolean(range));
 const bytes=new Uint8Array(r.body);let read=false;
 return{ok:r.status>=200&&r.status<300,status:r.status,headers:{get:k=>k.toLowerCase()==='content-length'?String(bytes.length):null},json:async()=>JSON.parse(nativeDecode(r.body)),text:async()=>nativeDecode(r.body),arrayBuffer:async()=>r.body,body:{getReader:()=>({read:async()=>read?{done:true}:(read=true,{done:false,value:bytes}),cancel:async()=>{read=true;}})}};
};
