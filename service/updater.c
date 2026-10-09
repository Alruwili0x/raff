/* SPDX-License-Identifier: GPL-3.0-or-later
 * Fixed-destination, whole-app update transaction. User data never lives here.
 * Bundle: RAFFUPD1, u32 count; sorted records {u16 path bytes, u64 size,
 * SHA256[32], UTF-8 path, contents}. Integers are little-endian. No links.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/file.h>
#include <openssl/sha.h>
#include <stdatomic.h>
#ifndef UP
#define UP "/data/raff/native-v5/updates"
#endif
#ifndef APP
#define APP "/data/homebrew/PPSA99178"
#endif
#define STAGE UP "/staged-app"
#define PREVIOUS UP "/previous-app"
#define LIMIT (2ULL*1024*1024*1024)
static int directory(const char*p){struct stat st;return !lstat(p,&st)&&S_ISDIR(st.st_mode);}
static int regular(const char*p){struct stat st;return !lstat(p,&st)&&S_ISREG(st.st_mode);}
static int parents(const char*p){char b[1024];if(strlen(p)>=sizeof(b))return -1;strcpy(b,p);for(char*s=b+1;*s;s++)if(*s=='/'){*s=0;if(!directory(b)&&(mkdir(b,0755)||!directory(b)))return -1;*s='/';}return 0;}
/* Only called on these literal, owned update directories. Never follow links. */
static int remove_tree(const char*p){struct stat st;if(lstat(p,&st))return errno==ENOENT?0:-1;if(!S_ISDIR(st.st_mode))return S_ISREG(st.st_mode)?unlink(p):-1;DIR*d=opendir(p);if(!d)return -1;struct dirent*e;int rc=0;while((e=readdir(d))){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;char b[1024];int n=snprintf(b,sizeof(b),"%s/%s",p,e->d_name);if(n<0||n>=(int)sizeof(b)||remove_tree(b)){rc=-1;break;}}closedir(d);return rc?rc:rmdir(p);}
static int atomic_text(const char*p,const char*t){char tmp[1024];snprintf(tmp,sizeof(tmp),"%s.new",p);int fd=open(tmp,O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0600);if(fd<0)return -1;size_t n=strlen(t);int rc=write(fd,t,n)==(ssize_t)n&&!fsync(fd)?0:-1;close(fd);if(!rc)rc=rename(tmp,p);return rc;}
static void sync_parent(void){int fd=open(UP,O_RDONLY|O_DIRECTORY);if(fd>=0){fsync(fd);close(fd);}fd=open("/data/homebrew",O_RDONLY|O_DIRECTORY);if(fd>=0){fsync(fd);close(fd);}}
static int valid_path(const char*p){
 if(!*p||strlen(p)>240||p[0]=='.'||p[0]=='/'||strchr(p,'\\')||strstr(p,"//")||strstr(p,"/.")||strchr(p,':'))return 0;
 for(const unsigned char*s=(void*)p;*s;s++)if(*s<32||*s==127)return 0;
 const char*files[]={"eboot.bin","raff-service.elf","raff-updater.elf","raff-owner.txt","LICENSE","README.md","THIRD_PARTY_NOTICES.md"};
 for(unsigned n=0;n<sizeof(files)/sizeof(*files);n++)if(!strcmp(p,files[n]))return 1;
 const char*dirs[]={"assets/","sce_sys/","sce_module/","runtime/","licenses/","docs/"};
 for(unsigned n=0;n<sizeof(dirs)/sizeof(*dirs);n++)if(!strncmp(p,dirs[n],strlen(dirs[n])))return 1;
 return 0;
}
static int read_full(FILE*f,void*p,size_t n){return fread(p,1,n,f)==n?0:-1;}
static uint64_t little(const unsigned char*p,int n){uint64_t v=0;for(int i=n-1;i>=0;i--)v=v*256+p[i];return v;}
/* The native loader requires execute permission on PRX modules. Only known
 * executable types get 0755; assets and configuration remain read-only to others. */
static mode_t file_mode(const char*p){size_t n=strlen(p);return !strcmp(p,"eboot.bin")||(n>4&&(!strcmp(p+n-4,".elf")||!strcmp(p+n-4,".prx")))?0755:0644;}
static void receipt(const char*phase,const char*error){char b[512];snprintf(b,sizeof(b),"{\"phase\":\"%s\",\"error\":\"%s\"}\n",phase,error?error:"");atomic_text(UP "/result.json",b);
#ifdef RAFF_UPDATER_MAIN
 if(!strcmp(phase,"installed")||!strcmp(phase,"error")){struct{char prefix[45],message[1024],suffix[2051];}n={0};snprintf(n.message,sizeof(n.message),"%s",!strcmp(phase,"installed")?"Raff updated. Open Raff again.\nتم تحديث رفّ، افتح التطبيق من جديد":"Raff update did not finish. Check About and updates.\nلم يكتمل تحديث رفّ، راجع صفحة التحديثات");extern int sceKernelSendNotificationRequest(size_t,const void*,size_t,int);sceKernelSendNotificationRequest(0,&n,sizeof(n),0);}
#endif
}
static int stage_bundle(const char*path,const char*expected,_Atomic uint64_t*progress){
 int fd=open(path,O_RDONLY|O_NOFOLLOW);struct stat st;if(fd<0||fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<12||st.st_size>(off_t)LIMIT){if(fd>=0)close(fd);return -1;}
 FILE*f=fdopen(fd,"rb");if(!f){close(fd);return -1;}unsigned char buf[65536],digest[32];SHA256_CTX hash;SHA256_Init(&hash);size_t got;uint64_t total=0;
 while((got=fread(buf,1,sizeof(buf),f))){SHA256_Update(&hash,buf,got);total+=got;*progress=total;}SHA256_Final(digest,&hash);char hex[65];for(int n=0;n<32;n++)snprintf(hex+n*2,3,"%02x",digest[n]);
 if(ferror(f)||total!=(uint64_t)st.st_size||strcmp(hex,expected)){fclose(f);return -2;}
 struct statvfs space;if(statvfs(UP,&space)||(uint64_t)space.f_bavail*space.f_frsize<total+67108864){fclose(f);return -3;}
 if(remove_tree(STAGE)||mkdir(STAGE,0755)){fclose(f);return -4;}
 rewind(f);unsigned char head[12];if(read_full(f,head,12)||memcmp(head,"RAFFUPD1",8)){fclose(f);return -5;}uint64_t count=little(head+8,4);if(!count||count>50000){fclose(f);return -5;}
 char previous[241]="";int result=0;uint64_t consumed=12;
 for(uint64_t n=0;n<count;n++){
  unsigned char record[42];char pathpart[241],dest[1024];if(read_full(f,record,42)){result=-5;break;}
  uint64_t len=little(record,2),size=little(record+2,8);consumed+=42+len+size;
  if(!len||len>240||!size||size>LIMIT||consumed>total||read_full(f,pathpart,len)){result=-5;break;}pathpart[len]=0;
  if(strlen(pathpart)!=len||!valid_path(pathpart)||strcmp(previous,pathpart)>=0){result=-6;break;}strcpy(previous,pathpart);
  snprintf(dest,sizeof(dest),STAGE "/%s",pathpart);if(parents(dest)){result=-4;break;}
  int out=open(dest,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644);if(out<0){result=-4;break;}SHA256_Init(&hash);
  uint64_t left=size;while(left){size_t take=left>sizeof(buf)?sizeof(buf):(size_t)left;if(read_full(f,buf,take)){result=-5;break;}SHA256_Update(&hash,buf,take);size_t at=0;while(at<take){ssize_t w=write(out,buf+at,take-at);if(w<0&&errno==EINTR)continue;if(w<=0){result=-4;break;}at+=w;}if(result)break;left-=take;}
  SHA256_Final(digest,&hash);if(memcmp(digest,record+10,32))result=-2;if(fchmod(out,file_mode(pathpart))||fsync(out))result=-4;close(out);if(result)break;
 }
 if(!result&&(consumed!=total||fgetc(f)!=EOF))result=-5;fclose(f);
 const char*required[]={"eboot.bin","raff-service.elf","raff-updater.elf","raff-owner.txt","sce_sys/param.json","sce_module/libc.prx","assets/update-config.json","assets/catalog.json"};
 for(unsigned n=0;!result&&n<sizeof(required)/sizeof(*required);n++){char p[1024];snprintf(p,sizeof(p),STAGE "/%s",required[n]);if(!regular(p))result=-7;}
 if(result){remove_tree(STAGE);return result;}
 if(atomic_text(UP "/verified",expected))return -4;sync_parent();return 0;
}
#ifdef RAFF_UPDATER_MAIN
/* The helper is sent to the payload loader before the app exits. It replaces
 * files only after both the app sandbox and the old service have disappeared. */
#include <sys/socket.h>
#include <netinet/in.h>
static int service_running(void){int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return 1;struct sockaddr_in a={0};a.sin_len=sizeof(a);a.sin_family=AF_INET;a.sin_port=htons(8794);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);int yes=!connect(fd,(void*)&a,sizeof(a));close(fd);return yes;}
static int app_running(void){DIR*d=opendir("/mnt/sandbox");if(!d)return 1;struct dirent*e;int yes=0;while((e=readdir(d)))if(!strncmp(e->d_name,"PPSA99178",9)){yes=1;break;}closedir(d);return yes;}
static int cache_copy(const char*a,const char*b){int in=open(a,O_RDONLY|O_NOFOLLOW);if(in<0)return -1;struct stat s;if(fstat(in,&s)||!S_ISREG(s.st_mode)||s.st_size>4194304){close(in);return -1;}int out=open(b,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644);if(out<0){close(in);return -1;}char buf[65536];ssize_t got;off_t total=0;int rc=0;while((got=read(in,buf,sizeof(buf)))>0){size_t at=0;while(at<(size_t)got){ssize_t n=write(out,buf+at,got-at);if(n<0&&errno==EINTR)continue;if(n<=0){rc=-1;break;}at+=n;}if(rc)break;total+=got;}if(got<0||total!=s.st_size)rc=-1;close(in);close(out);if(rc)unlink(b);return rc;}
/* Cache files are hard-linked, or copied if the filesystem rejects hard links.
 * Existing artwork stays available without downloading it with every update.
 * The previous app retains the same files for rollback. No symlinks or code. */
static int preserve_art(const char*folder){char src[256],dst[256];snprintf(src,sizeof(src),APP "/assets/%s",folder);snprintf(dst,sizeof(dst),STAGE "/assets/%s",folder);if(!directory(src))return 0;if(!directory(dst)&&mkdir(dst,0755))return -1;DIR*d=opendir(src);if(!d)return -1;struct dirent*e;int rc=0,count=0;while((e=readdir(d))){if(e->d_name[0]=='.')continue;if(++count>50000){rc=-1;break;}size_t len=strlen(e->d_name);if(len<5||strcmp(e->d_name+len-4,".jpg")&&strcmp(e->d_name+len-4,".png"))continue;char a[1024],b[1024];snprintf(a,sizeof(a),"%s/%s",src,e->d_name);snprintf(b,sizeof(b),"%s/%s",dst,e->d_name);if(!regular(a))continue;struct stat sa,sb;if(!lstat(a,&sa)&&!lstat(b,&sb)&&S_ISREG(sb.st_mode)){if(sa.st_size==sb.st_size)continue;if(unlink(b)){rc=-1;break;}}if(link(a,b)&&cache_copy(a,b)){rc=-1;break;}}closedir(d);return rc;}
int main(void){
 if(!regular(UP "/armed")||!regular(UP "/verified")||!directory(STAGE))return 1;
 int lock=open(UP "/apply.lock",O_WRONLY|O_CREAT|O_NOFOLLOW,0600);if(lock<0||flock(lock,LOCK_EX|LOCK_NB))return 2;
 receipt("waiting-close","");for(int n=0;n<180&&(app_running()||service_running());n++)sleep(1);
 if(app_running()||service_running()){receipt("error","Close Raff completely and retry");unlink(UP "/apply.lock");unlink(UP "/armed");return 3;}
 receipt("preserving-art","");if(!directory(APP)||!regular(APP "/raff-owner.txt")||preserve_art("covers")||preserve_art("media")||remove_tree(PREVIOUS)){receipt("error","Existing application could not be backed up");unlink(UP "/apply.lock");unlink(UP "/armed");return 4;}
 if(app_running()||service_running()){receipt("error","Raff was reopened during preparation; close it and retry");unlink(UP "/apply.lock");unlink(UP "/armed");return 4;}
 if(atomic_text(UP "/transaction","prepared")){receipt("error","Could not prepare update transaction");unlink(UP "/apply.lock");unlink(UP "/armed");return 5;}
 if(rename(APP,PREVIOUS)){receipt("error","Application is still in use");unlink(UP "/apply.lock");unlink(UP "/armed");return 6;}sync_parent();
 if(rename(STAGE,APP)){int restored=!rename(PREVIOUS,APP);sync_parent();receipt("error",restored?"Update failed; previous application restored":"Recovery required: restore previous-app");unlink(UP "/apply.lock");unlink(UP "/armed");return 7;}
 sync_parent();receipt("installed","");unlink(UP "/armed");unlink(UP "/verified");unlink(UP "/transaction");unlink(UP "/apply.lock");return 0;
}
#elif !defined(RAFF_UPDATER_SELFTEST)
#include "quickjs.h"
#include <pthread.h>
static pthread_mutex_t update_mutex=PTHREAD_MUTEX_INITIALIZER;
static int phase;static _Atomic uint64_t verified_bytes;static char bundle[256],expected_hash[65];
static void*worker(void*unused){(void)unused;int rc=stage_bundle(bundle,expected_hash,&verified_bytes);pthread_mutex_lock(&update_mutex);phase=rc?rc:2;pthread_mutex_unlock(&update_mutex);return NULL;}
static JSValue update_js(JSContext*c,JSValueConst self,int argc,JSValueConst*argv){
 (void)self;const char*op=argc?JS_ToCString(c,argv[0]):NULL;if(!op)return JS_EXCEPTION;JSValue out=JS_UNDEFINED;
 if(!strcmp(op,"verify")){
  JSValue gv=JS_GetPropertyStr(c,argv[1],"gid"),hv=JS_GetPropertyStr(c,argv[1],"sha256");const char*g=JS_ToCString(c,gv),*h=JS_ToCString(c,hv);
  int valid=g&&h&&strlen(g)==16&&strspn(g,"0123456789abcdef")==16&&strlen(h)==64&&strspn(h,"0123456789abcdef")==64;
  pthread_mutex_lock(&update_mutex);if(!valid||phase==1)out=JS_ThrowTypeError(c,"Invalid or busy update verification");else{
   snprintf(bundle,sizeof(bundle),UP "/package-%s.raffupdate",g);strcpy(expected_hash,h);phase=1;verified_bytes=0;unlink(UP "/verified");
   pthread_attr_t a;pthread_attr_init(&a);pthread_attr_setstacksize(&a,1024*1024);pthread_attr_setdetachstate(&a,PTHREAD_CREATE_DETACHED);pthread_t t;int rc=pthread_create(&t,&a,worker,NULL);pthread_attr_destroy(&a);if(rc){phase=-8;out=JS_ThrowInternalError(c,"Update worker unavailable");}
  }pthread_mutex_unlock(&update_mutex);JS_FreeCString(c,g);JS_FreeCString(c,h);JS_FreeValue(c,gv);JS_FreeValue(c,hv);
 }else if(!strcmp(op,"status")){pthread_mutex_lock(&update_mutex);int n=phase;pthread_mutex_unlock(&update_mutex);out=JS_NewObject(c);JS_SetPropertyStr(c,out,"phase",JS_NewString(c,n==1?"verifying":n==2||(!n&&regular(UP "/verified"))?"ready":n<0?"error":"idle"));JS_SetPropertyStr(c,out,"received",JS_NewFloat64(c,(double)verified_bytes));JS_SetPropertyStr(c,out,"error",JS_NewString(c,"Update package verification failed; installed app is unchanged"));
 }else if(!strcmp(op,"arm")){unlink(UP "/result.json");if(!regular(UP "/verified")||!directory(STAGE)||atomic_text(UP "/armed","1"))out=JS_ThrowInternalError(c,"No verified update is ready");}
 else out=JS_ThrowTypeError(c,"Unknown update operation");JS_FreeCString(c,op);return out;
}
void raff_updater_init(JSContext*c){JSValue g=JS_GetGlobalObject(c);JS_SetPropertyStr(c,g,"nativeUpdateFiles",JS_NewCFunction(c,update_js,"nativeUpdateFiles",2));JS_FreeValue(c,g);}
#endif
