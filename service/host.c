/* Raff background download service. SPDX-License-Identifier: GPL-3.0-or-later */
#include "quickjs.h"
#include "database.h"
#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/sha.h>

#define ROOT "/data/raff/native-v5"
extern const unsigned char js_data[],js_data_end[],catalog_data[],catalog_data_end[];
extern void raff_updater_init(JSContext*);
extern int raff_bootstrap(void);
static int stop_requested;
static char api_token[65];
static int session_token(void){
 unsigned char random[32];arc4random_buf(random,sizeof(random));const char*digits="0123456789abcdef";
 for(int n=0;n<32;n++){api_token[n*2]=digits[random[n]>>4];api_token[n*2+1]=digits[random[n]&15];}
 const char*paths[]={ROOT "/api-token.txt","/data/homebrew/PPSA99178/.session-token"};
 for(int n=0;n<2;n++){int fd=open(paths[n],O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0600);if(fd<0)return -1;/* The payload is root; the sandboxed frontend must read its app-local copy. */
  int ok=!fchmod(fd,n==0?0600:0644)&&write(fd,api_token,64)==64&&!fsync(fd);close(fd);if(!ok)return -1;}return 0;
}
static int authorized(const char*request,size_t header){
 const char*p=strstr(request,"\r\n");int found=0;
 while(p&&p<request+header-2){p+=2;if(!strncasecmp(p,"Origin:",7))return 0;if(!strncasecmp(p,"X-Raff-Token:",13)){const char*v=p+13;while(*v==' '||*v=='\t')v++;const char*e=strstr(v,"\r\n");if(!e||e-v!=64)return 0;unsigned diff=0;for(int n=0;n<64;n++)diff|=(unsigned char)v[n]^(unsigned char)api_token[n];if(diff||found)return 0;found=1;}p=strstr(p,"\r\n");}return found;
}
static JSValue stop_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){(void)c;(void)t;(void)argc;(void)argv;stop_requested=1;return JS_UNDEFINED;}
static JSRuntime *runtime;static JSContext *context;static FILE* log_file;
static int all(int fd,const void *ptr,size_t size){const char *p=ptr;while(size){ssize_t n=send(fd,p,size,0);if(n<0&&errno==EINTR)continue;if(n<=0)return-1;p+=n;size-=n;}return 0;}
static int connect_local(int port){int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return-1;struct timeval t={3,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&t,sizeof(t));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&t,sizeof(t));struct sockaddr_in a={0};a.sin_len=sizeof(a);a.sin_family=AF_INET;a.sin_port=htons(port);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(connect(fd,(void*)&a,sizeof(a))){close(fd);return-1;}return fd;}
static int start_elf(const char*path){FILE*f=fopen(path,"rb");if(!f)return -1;unsigned char b[65536];if(fread(b,1,4,f)!=4||memcmp(b,"\177ELF",4)){fclose(f);return -1;}rewind(f);int fd=connect_local(9021);if(fd>=0){size_t n;while((n=fread(b,1,sizeof(b),f)))if(all(fd,b,n))break;shutdown(fd,SHUT_WR);close(fd);}fclose(f);return fd<0?-1:0;}
static JSValue launch_update_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){(void)t;(void)argc;(void)argv;if(start_elf("/data/homebrew/PPSA99178/raff-updater.elf")){unlink(ROOT "/updates/armed");return JS_ThrowInternalError(c,"ELF loader unavailable");}return JS_UNDEFINED;}
static void support(void){int fd=connect_local(6800);if(fd<0)start_elf("/data/raff/aria2.elf");else close(fd);fd=connect_local(8888);if(fd<0){start_elf("/data/raff/wfm-7zip-helper.elf");start_elf("/data/raff/web-file-mgr.elf");}else close(fd);}
static void exception(void){JSValue err=JS_GetException(context);const char *s=JS_ToCString(context,err);fprintf(log_file,"JS: %s\n",s?s:"error");JS_FreeCString(context,s);JSValue stack=JS_GetPropertyStr(context,err,"stack");s=JS_ToCString(context,stack);fprintf(log_file,"%s\n",s?s:"");JS_FreeCString(context,s);JS_FreeValue(context,stack);JS_FreeValue(context,err);fflush(log_file);}
static JSValue log_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){(void)t;const char*s=argc?JS_ToCString(c,argv[0]):NULL;if(s){fprintf(log_file,"%s\n",s);fflush(log_file);JS_FreeCString(c,s);}return JS_UNDEFINED;}
static JSValue random_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){(void)t;int n=0;if(argc)JS_ToInt32(c,&n,argv[0]);if(n<0||n>4096)return JS_ThrowRangeError(c,"random limit");unsigned char b[4096];arc4random_buf(b,n);return JS_NewArrayBufferCopy(c,b,n);}
static JSValue decode_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){(void)t;size_t n=0;uint8_t*b=argc?JS_GetArrayBuffer(c,&n,argv[0]):NULL;if(!b)return JS_NewString(c,"");return JS_NewStringLen(c,(char*)b,n);}
static JSValue digest_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){
 (void)t;if(argc<2)return JS_ThrowTypeError(c,"Digest arguments");size_t n=0;uint8_t*b=JS_GetArrayBuffer(c,&n,argv[1]);const char*a=JS_ToCString(c,argv[0]);unsigned char hash[32];int length=0;
 if(a&&b&&n<=1048576){if(!strcmp(a,"sha1")){SHA_CTX ctx;SHA1_Init(&ctx);SHA1_Update(&ctx,b,n);SHA1_Final(hash,&ctx);length=20;}else if(!strcmp(a,"sha256")){SHA256_CTX ctx;SHA256_Init(&ctx);SHA256_Update(&ctx,b,n);SHA256_Final(hash,&ctx);length=32;}}JS_FreeCString(c,a);if(!length)return JS_ThrowTypeError(c,"Invalid digest request");
 char hex[65];const char*d="0123456789abcdef";for(int i=0;i<length;i++){hex[i*2]=d[hash[i]>>4];hex[i*2+1]=d[hash[i]&15];}hex[length*2]=0;return JS_NewString(c,hex);
}
static const char *file_key(const char *name){if(!strcmp(name,"update-config"))return "/data/homebrew/PPSA99178/assets/update-config.json";if(!strcmp(name,"updates"))return ROOT "/updates/state.json";if(!strcmp(name,"update-result"))return ROOT "/updates/result.json";if(!strcmp(name,"jobs"))return ROOT "/jobs.json";if(!strcmp(name,"settings"))return ROOT "/settings.json";if(!strcmp(name,"rpc-token"))return ROOT "/rpc-token.txt";if(!strcmp(name,"game-torrents"))return "/data/homebrew/PPSA99178/assets/game-torrents.json";if(!strcmp(name,"hub"))return "/data/homebrew/PPSA99178/assets/hub.json";if(!strcmp(name,"cards"))return "/data/homebrew/PPSA99178/assets/catalog.json";if(!strcmp(name,"registry"))return "/data/homebrew/PPSA99178/assets/core-registry.json";return NULL;}
/* Read-only proof from the system's installed package and PlayGo chunk state.
 * Never writes under /user/app and never treats a title-directory alone as success. */
static JSValue package_files_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){
 (void)t;const char*id=argc?JS_ToCString(c,argv[0]):NULL;
 if(!id)return JS_ThrowTypeError(c,"Missing title ID");
 int valid=strlen(id)==9&&(!strncmp(id,"CUSA",4)||!strncmp(id,"PPSA",4));
 for(int i=4;valid&&i<9;i++)if(id[i]<'0'||id[i]>'9')valid=0;
 char root[128];snprintf(root,sizeof(root),"/user/app/%s",valid?id:"");JS_FreeCString(c,id);
 if(!valid)return JS_ThrowTypeError(c,"Invalid title ID");
 JSValue out=JS_NewObject(c);struct stat st;if(lstat(root,&st)||!S_ISDIR(st.st_mode))return out;
 const char*names[]={"app.pkg","app.json","app.xml"},*keys[]={"header","metadata","playgo"};
 for(int i=0;i<3;i++){char path[160];snprintf(path,sizeof(path),"%s/%s",root,names[i]);int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)continue;
  if(fstat(fd,&st)||!S_ISREG(st.st_mode)){close(fd);continue;}size_t limit=i==0?4096:131072;
  if(i&&st.st_size>(off_t)limit){close(fd);continue;}char*b=malloc(limit+1);if(!b){close(fd);continue;}size_t used=0;
  while(used<limit){ssize_t n=read(fd,b+used,limit-used);if(n<0&&errno==EINTR)continue;if(n<=0)break;used+=n;}close(fd);b[used]=0;
  if(i==0){JS_SetPropertyStr(c,out,"size",JS_NewFloat64(c,(double)st.st_size));char*hex=malloc(used*2+1);if(hex){const char*d="0123456789abcdef";for(size_t n=0;n<used;n++){hex[n*2]=d[(unsigned char)b[n]>>4];hex[n*2+1]=d[(unsigned char)b[n]&15];}hex[used*2]=0;JS_SetPropertyStr(c,out,keys[i],JS_NewString(c,hex));free(hex);}}
  else JS_SetPropertyStr(c,out,keys[i],JS_NewStringLen(c,b,used));free(b);
 }return out;
}
static JSValue read_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){(void)t;const char*n=argc?JS_ToCString(c,argv[0]):NULL;if(!n)return JS_EXCEPTION;if(!strcmp(n,"catalog")){JS_FreeCString(c,n);return JS_NewStringLen(c,(char*)catalog_data,catalog_data_end-catalog_data);}const char*p=file_key(n);JS_FreeCString(c,n);if(!p)return JS_ThrowTypeError(c,"Invalid data key");FILE*f=fopen(p,"rb");if(!f)return JS_NewString(c,"");fseek(f,0,SEEK_END);long len=ftell(f);rewind(f);if(len<0||len>16*1024*1024){fclose(f);return JS_ThrowRangeError(c,"Data size");}char*b=malloc(len+1);if(!b){fclose(f);return JS_ThrowOutOfMemory(c);}size_t used=fread(b,1,len,f);fclose(f);JSValue r=used==(size_t)len?JS_NewStringLen(c,b,len):JS_ThrowInternalError(c,"Read failed");free(b);return r;}
static JSValue write_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){(void)t;if(argc<2)return JS_ThrowTypeError(c,"Missing data");const char*n=JS_ToCString(c,argv[0]),*p=n?file_key(n):NULL;size_t len=0;const char*b=JS_ToCStringLen(c,&len,argv[1]);if(!n||(strcmp(n,"jobs")&&strcmp(n,"settings")&&strcmp(n,"updates"))||!p||!b||len>16*1024*1024){JS_FreeCString(c,n);JS_FreeCString(c,b);return JS_ThrowTypeError(c,"Invalid data");}char tmp[256];snprintf(tmp,sizeof(tmp),"%s.new",p);int fd=open(tmp,O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0644),rc=-1;if(fd>=0){size_t at=0;while(at<len){ssize_t v=write(fd,b+at,len-at);if(v<0&&errno==EINTR)continue;if(v<=0)break;at+=v;}if(at==len&&!fsync(fd))rc=0;close(fd);if(!rc)rc=rename(tmp,p);}JS_FreeCString(c,n);JS_FreeCString(c,b);return rc?JS_ThrowInternalError(c,"Save failed"):JS_UNDEFINED;}
static JSValue http_js(JSContext*c,JSValueConst t,int argc,JSValueConst*argv){(void)t;if(argc<6)return JS_ThrowTypeError(c,"HTTP arguments");const char*u=JS_ToCString(c,argv[0]),*m=JS_ToCString(c,argv[1]),*body=JS_ToCString(c,argv[2]),*mime=JS_ToCString(c,argv[3]);int limit=0,range=JS_ToBool(c,argv[5]);JS_ToInt32(c,&limit,argv[4]);JSValue result=JS_EXCEPTION;int fd=-1;char *data=NULL;const char *error="HTTP failed";unsigned port=0;char path[4096];
 if(!u||!m||!body||!mime)goto end;
 if(sscanf(u,"http://127.0.0.1:%u%4095s",&port,path)!=2||(port!=8888&&port!=6800)||path[0]!='/'||strchr(path,'\r')||strchr(path,'\n')||(strcmp(m,"GET")&&strcmp(m,"POST"))||strchr(mime,'\r')||strchr(mime,'\n')||limit<0||limit>4194304){error="Local HTTP request rejected";goto end;}
 fd=connect_local(port);if(fd<0){error="Local service unavailable";goto end;}
 /* Large torrent collections can hold aria2's RPC loop while initializing its hash check. */
 struct timeval io_timeout={8,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&io_timeout,sizeof(io_timeout));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&io_timeout,sizeof(io_timeout));
 error="Local HTTP send failed";char header[8192],extra[80]="";if(range)snprintf(extra,sizeof(extra),"Range: bytes=0-%d\r\n",limit-1);int hn=snprintf(header,sizeof(header),"%s %s HTTP/1.1\r\nHost: 127.0.0.1:%u\r\nConnection: close\r\nContent-Type: %s\r\nContent-Length: %zu\r\n%s\r\n",m,path,port,*mime?mime:"text/plain",strlen(body),extra);if(hn<0||hn>=(int)sizeof(header)||all(fd,header,hn)||all(fd,body,strlen(body)))goto end;
 size_t used=0,head=0;data=malloc((size_t)limit+16385);if(!data){error="Out of memory";goto end;}
 while(used<(size_t)limit+16384){ssize_t n=recv(fd,data+used,(size_t)limit+16384-used,0);if(n<0&&errno==EINTR)continue;if(n<0){error=(errno==EAGAIN||errno==EWOULDBLOCK)?"Local HTTP response timeout":"Local HTTP receive failed";goto end;}if(!n)break;used+=n;data[used]=0;if(!head){char*e=strstr(data,"\r\n\r\n");if(e)head=e-data+4;else if(used>=16384){error="HTTP headers too large";goto end;}}if(head&&used-head>=(size_t)limit)break;}
 int status=0;if(!head||sscanf(data,"HTTP/%*s %d",&status)!=1){error="Invalid HTTP response";goto end;}size_t size=used-head;if(size>(size_t)limit)size=limit;
 // WFM and aria2 use Content-Length. Decode chunked responses as well.
 for(size_t i=0;i+17<head;i++)if(!strncasecmp(data+i,"Transfer-Encoding",17)){char*e=strstr(data+i,"\r\n");if(e){char saved=*e;*e=0;int chunked=strstr(data+i,"chunked")!=NULL;*e=saved;if(chunked){size_t at=head,out=0;while(at<used){char*next=NULL;unsigned long n=strtoul(data+at,&next,16);if(!next||next==data+at||strncmp(next,"\r\n",2)){error="Invalid HTTP chunks";goto end;}at=next-data+2;if(!n)break;if(n>used-at||out+n>(size_t)limit){error="Incomplete HTTP chunk";goto end;}memmove(data+head+out,data+at,n);out+=n;at+=n+2;}size=out;} }break;}
 result=JS_NewObject(c);JS_SetPropertyStr(c,result,"status",JS_NewInt32(c,status));JS_SetPropertyStr(c,result,"body",JS_NewArrayBufferCopy(c,(uint8_t*)data+head,size));
end:if(fd>=0)close(fd);free(data);JS_FreeCString(c,u);JS_FreeCString(c,m);JS_FreeCString(c,body);JS_FreeCString(c,mime);if(JS_IsException(result))return JS_ThrowInternalError(c,"%s",error);return result;}
static void pump(void){JSValue g=JS_GetGlobalObject(context),fn=JS_GetPropertyStr(context,g,"tick"),v=JS_Call(context,fn,g,0,NULL);if(JS_IsException(v))exception();JS_FreeValue(context,v);JS_FreeValue(context,fn);JS_FreeValue(context,g);JSContext*c;for(int n=0;n<256;n++){int rc=JS_ExecutePendingJob(runtime,&c);if(rc<=0){if(rc<0)exception();break;}}}
static void serve(int fd,int local){char request[32768]={0};size_t used=0,header=0,length=0;for(;;){ssize_t n=recv(fd,request+used,sizeof(request)-used-1,0);if(n<=0)return;used+=n;request[used]=0;if(!header){char*e=strstr(request,"\r\n\r\n");if(e){header=e-request+4;char*p=request;while(p<e){if(!strncasecmp(p,"Content-Length:",15)){length=strtoul(p+15,NULL,10);break;}p=strstr(p,"\r\n");if(!p)break;p+=2;}}}if(length>16384)return;if(header&&used>=header+length)break;if(used>=sizeof(request)-1||length>16384)return;}
 char method[8],path[1024];if(sscanf(request,"%7s %1023s",method,path)!=2)return;if(!local||(strcmp(path,"/health")&&!authorized(request,header))){const char*r="HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";all(fd,r,strlen(r));return;}JSValue g=JS_GetGlobalObject(context),fn=JS_GetPropertyStr(context,g,"route"),args[]={JS_NewString(context,method),JS_NewString(context,path),JS_NewStringLen(context,request+header,length)};JSValue promise=JS_Call(context,fn,g,3,args);for(int i=0;i<3;i++)JS_FreeValue(context,args[i]);JS_FreeValue(context,fn);JS_FreeValue(context,g);
 int waits=0;while(!JS_IsException(promise)&&JS_PromiseState(context,promise)==JS_PROMISE_PENDING&&waits++<500){pump();usleep(10000);}JSValue value=JS_IsException(promise)?JS_UNDEFINED:JS_PromiseResult(context,promise);const char*body=JS_ToCString(context,value);const char*allocated=body;if(!body)body="{\"error\":\"Service response failed\"}";char hdr[256];int hn=snprintf(hdr,sizeof(hdr),"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %zu\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n",strlen(body));all(fd,hdr,hn);all(fd,body,strlen(body));if(allocated)JS_FreeCString(context,allocated);JS_FreeValue(context,value);JS_FreeValue(context,promise);}
int main(void){signal(SIGPIPE,SIG_IGN);struct stat update_lock;if(!lstat(ROOT "/updates/armed",&update_lock))return 9;int existing=connect_local(8794);if(existing>=0){close(existing);return 0;}if(raff_bootstrap())return 8;struct stat st;if(lstat("/data/raff",&st)||!S_ISDIR(st.st_mode))return 1;if(lstat(ROOT,&st)){if(mkdir(ROOT,0755))return 2;}else if(!S_ISDIR(st.st_mode))return 2;log_file=fopen(ROOT "/service.log","a");if(!log_file)return 3;
 int server=socket(AF_INET,SOCK_STREAM,0),yes=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));struct sockaddr_in a={0};a.sin_len=sizeof(a);a.sin_family=AF_INET;a.sin_port=htons(8794);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(server,(void*)&a,sizeof(a))||listen(server,8))return 4;fcntl(server,F_SETFL,O_NONBLOCK);
 if(session_token()){fprintf(log_file,"Session token creation failed\n");return 7;}support();runtime=JS_NewRuntime();JS_SetMemoryLimit(runtime,192*1024*1024);JS_SetMaxStackSize(runtime,2*1024*1024);context=JS_NewContext(runtime);JSValue g=JS_GetGlobalObject(context);
 #define FUNC(name,fn,n) JS_SetPropertyStr(context,g,name,JS_NewCFunction(context,fn,name,n))
 JS_SetPropertyStr(context,g,"nativePid",JS_NewInt32(context,getpid()));FUNC("nativeDigest",digest_js,2);FUNC("nativePackageFiles",package_files_js,1);FUNC("nativeStop",stop_js,0);FUNC("nativeLog",log_js,1);FUNC("nativeRandom",random_js,1);FUNC("nativeDecode",decode_js,1);FUNC("nativeRead",read_js,1);FUNC("nativeWrite",write_js,2);FUNC("nativeHTTP",http_js,6);JS_FreeValue(context,g);
 raff_updater_init(context);JSValue ug=JS_GetGlobalObject(context);JS_SetPropertyStr(context,ug,"nativeLaunchUpdater",JS_NewCFunction(context,launch_update_js,"nativeLaunchUpdater",0));JS_FreeValue(context,ug);raff_filesystem_init(context);if(raff_database_init(context)){fprintf(log_file,"Library database failed to initialize\n");fflush(log_file);return 6;}
 JSValue v=JS_Eval(context,(char*)js_data,js_data_end-js_data,"raff-service.js",JS_EVAL_TYPE_GLOBAL);if(JS_IsException(v)){exception();return 5;}JS_FreeValue(context,v);fprintf(log_file,"Raff 1.1.1 transfer and hub service listening 127.0.0.1:8794\n");fflush(log_file);
 FILE*pid_file=fopen(ROOT "/service.pid","w");if(pid_file){fprintf(pid_file,"%d\n",getpid());fclose(pid_file);}
 for(;!stop_requested;){pump();struct sockaddr_in peer;socklen_t plen=sizeof(peer);int fd=accept(server,(void*)&peer,&plen);if(fd>=0){fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)&~O_NONBLOCK);struct timeval t={2,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&t,sizeof(t));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&t,sizeof(t));serve(fd,peer.sin_addr.s_addr==htonl(INADDR_LOOPBACK));close(fd);}else usleep(10000);}close(server);unlink(ROOT "/service.pid");fflush(log_file);fclose(log_file);raff_database_close();JS_FreeContext(context);JS_FreeRuntime(runtime);return 0;}
