/* SPDX-License-Identifier: GPL-3.0-or-later
 * First-install ELF. Downloads one pinned Raff release over verified HTTPS.
 * Does not overwrite an existing installation or change system configuration.
 */
#define RAFF_UPDATER_SELFTEST
#ifndef UP
#define UP "/data/raff/installer"
#endif
#include "updater.c"
#include "console_curl.h"
#include "installer-release.h"
#include <curl/curl.h>
#include <sys/socket.h>
#include <netinet/in.h>
extern const unsigned char installer_ca[],installer_ca_end[];
extern int sceKernelSendNotificationRequest(size_t,const void*,size_t,int);
static void notice(const char*message){struct{char prefix[45],message[1024],suffix[2051];}n={0};snprintf(n.message,sizeof(n.message),"%s",message);sceKernelSendNotificationRequest(0,&n,sizeof(n),0);}
static void install_result(const char*phase,const char*error){char report[400];size_t at=0;for(const char*p=error?error:"";*p&&at+1<sizeof(report);p++){unsigned char ch=*p;report[at++]=ch<32?' ':ch=='"'||ch=='\\'?'\'':ch;}report[at]=0;receipt(phase,report);notice(error&&*error?error:!strcmp(phase,"installed")?"Raff installed. Open Raff from Games after the homebrew scan.\nتم تثبيت رفّ، افتحه من Games بعد فحص التطبيقات":"Raff installer is working...");}
struct download {int fd;uint64_t bytes;int percent;time_t last;};
static size_t receive(void*ptr,size_t size,size_t nmemb,void*user){struct download*d=user;if(size&&nmemb>SIZE_MAX/size)return 0;size_t n=size*nmemb;if(n>INSTALL_BYTES-d->bytes)return 0;size_t at=0;while(at<n){ssize_t wrote=write(d->fd,(char*)ptr+at,n-at);if(wrote<0&&errno==EINTR)continue;if(wrote<=0)return 0;at+=wrote;}d->bytes+=n;return n;}
static int progress(void*user,curl_off_t total,curl_off_t current,curl_off_t ut,curl_off_t un){(void)total;(void)current;(void)ut;(void)un;struct download*d=user;int p=(int)(d->bytes*100/INSTALL_BYTES);time_t now=time(NULL);if(p>=d->percent+25&&now-d->last>=5){char msg[100];snprintf(msg,sizeof(msg),"Raff installer: %d%%\nتثبيت رفّ: %d%%",p,p);notice(msg);d->percent=p;d->last=now;}return 0;}
static void rescan(void){
#ifndef RAFF_INSTALLER_TEST
 int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return;struct timeval timeout={2,0};setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));struct sockaddr_in a={0};a.sin_len=sizeof(a);a.sin_family=AF_INET;a.sin_port=htons(10101);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 if(!connect(fd,(void*)&a,sizeof(a))){const char req[]="POST /api/v1/scan HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Type: application/json\r\nContent-Length: 24\r\nConnection: close\r\n\r\n{\"reset_attempts\":false}";send(fd,req,sizeof(req)-1,0);}close(fd);
#endif
}
int main(void){
 if(parents(UP "/result.json"))return 1;
 int lock=open(UP "/install.lock",O_WRONLY|O_CREAT|O_NOFOLLOW,0600);if(lock<0||flock(lock,LOCK_EX|LOCK_NB)){notice("Raff installer is already running");return 2;}
 struct stat existing;if(!lstat(APP,&existing)){install_result("existing","Raff is already installed. Use Settings > About & updates.\nرفّ مثبت، حدّثه من الإعدادات داخل التطبيق");return 0;}if(errno!=ENOENT)return 3;
 struct statvfs space;if(statvfs(UP,&space)||(uint64_t)space.f_bavail*space.f_frsize<INSTALL_BYTES*2+134217728){install_result("error","Not enough space to install Raff\nالمساحة غير كافية لتثبيت رفّ");return 4;}
 notice("Installing Raff. Keep the PS5 on until completion.\nجاري تثبيت رفّ، انتظر حتى تظهر رسالة الاكتمال");
 const char*package=UP "/package.raffupdate";int fd=open(package,O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0600);if(fd<0)return 5;
 if(curl_global_init(CURL_GLOBAL_DEFAULT)){close(fd);return 6;}CURL*c=curl_easy_init();if(!c){close(fd);return 6;}
 struct download download={.fd=fd};struct curl_blob ca={(void*)installer_ca,(size_t)(installer_ca_end-installer_ca),CURL_BLOB_NOCOPY};
 console_curl_setup(c);curl_easy_setopt(c,CURLOPT_CAINFO,NULL);curl_easy_setopt(c,CURLOPT_CAINFO_BLOB,&ca);curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,2L);
 curl_easy_setopt(c,CURLOPT_URL,INSTALL_URL);curl_easy_setopt(c,CURLOPT_PROTOCOLS_STR,"https");curl_easy_setopt(c,CURLOPT_REDIR_PROTOCOLS_STR,"https");curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(c,CURLOPT_MAXREDIRS,6L);curl_easy_setopt(c,CURLOPT_FAILONERROR,1L);curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT,20L);curl_easy_setopt(c,CURLOPT_TIMEOUT,3600L);curl_easy_setopt(c,CURLOPT_LOW_SPEED_LIMIT,1024L);curl_easy_setopt(c,CURLOPT_LOW_SPEED_TIME,60L);curl_easy_setopt(c,CURLOPT_MAXFILESIZE_LARGE,(curl_off_t)INSTALL_BYTES);curl_easy_setopt(c,CURLOPT_USERAGENT,"Raff-Installer/" INSTALL_VERSION);
 curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(c,CURLOPT_WRITEDATA,&download);curl_easy_setopt(c,CURLOPT_XFERINFOFUNCTION,progress);curl_easy_setopt(c,CURLOPT_XFERINFODATA,&download);curl_easy_setopt(c,CURLOPT_NOPROGRESS,0L);
 receipt("downloading","");CURLcode rc=curl_easy_perform(c);long status=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&status);int flushed=!fsync(fd);close(fd);curl_easy_cleanup(c);curl_global_cleanup();
 if(rc!=CURLE_OK||status!=200||download.bytes!=INSTALL_BYTES||!flushed){char error[200];snprintf(error,sizeof(error),"Raff download failed (%d / HTTP %ld). Check internet and run the installer again.",(int)rc,status);install_result("error",error);return 7;}
 notice("Raff: verifying installation files\nرفّ: جاري التحقق من ملفات التثبيت");receipt("verifying","");_Atomic uint64_t verified_bytes=0;
 int verified=stage_bundle(package,INSTALL_SHA256,&verified_bytes);if(verified){install_result("error","Raff package verification failed. No application was replaced.");return 8;}
 /* parents(APP) creates parent directories only; APP itself must stay absent. */
 if(parents(APP)||!lstat(APP,&existing)||errno!=ENOENT||rename(STAGE,APP)){install_result("error","Raff could not be installed. Existing files were kept.");return 9;}
 sync_parent();unlink(package);unlink(UP "/verified");install_result("installed","");rescan();return 0;
}
