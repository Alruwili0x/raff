/* SPDX-License-Identifier: GPL-3.0-or-later. Fresh installs use app-bundled
 * runtime files. Existing user settings and existing aria2 config are preserved. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
static int ensure(const char*p){struct stat s;if(!lstat(p,&s))return S_ISDIR(s.st_mode)?0:-1;return mkdir(p,0755);}
static int copy_missing(const char*src,const char*dst){struct stat st;if(!lstat(dst,&st))return S_ISREG(st.st_mode)?0:-1;int in=open(src,O_RDONLY|O_NOFOLLOW);if(in<0)return -1;int out=open(dst,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644);if(out<0){close(in);return -1;}char b[65536];ssize_t n;int rc=0;while((n=read(in,b,sizeof(b)))>0){ssize_t at=0;while(at<n){ssize_t w=write(out,b+at,n-at);if(w<=0){rc=-1;break;}at+=w;}if(rc)break;}if(n<0||fsync(out))rc=-1;close(in);close(out);if(rc)unlink(dst);return rc;}
int raff_bootstrap(void){
 const char*dirs[]={"/data/raff","/data/raff/native-v5","/data/raff/native-v5/updates","/data/aria2","/data/wfm"};for(int i=0;i<5;i++)if(ensure(dirs[i]))return -1;
 const char*files[]={"aria2.elf","web-file-mgr.elf","wfm-7zip-helper.elf","ca-bundle.crt"};for(int i=0;i<4;i++){char s[200],d[200];snprintf(s,sizeof(s),"/data/homebrew/PPSA99178/runtime/%s",files[i]);snprintf(d,sizeof(d),"/data/raff/%s",files[i]);if(copy_missing(s,d))return -2;}
 if(copy_missing("/data/raff/wfm-7zip-helper.elf","/data/wfm/wfm-7zip-helper.elf"))return -2;
 char secret[257]="";FILE*f=fopen("/data/aria2/aria2.conf","r");if(f){char line[1024];while(fgets(line,sizeof(line),f))if(!strncmp(line,"rpc-secret=",11)){strncpy(secret,line+11,256);secret[strcspn(secret,"\r\n")]=0;}fclose(f);}else{
  unsigned char bytes[32];arc4random_buf(bytes,32);for(int n=0;n<32;n++)snprintf(secret+n*2,3,"%02x",bytes[n]);
  int fd=open("/data/aria2/aria2.conf",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return -3;char cfg[1024];int n=snprintf(cfg,sizeof(cfg),"async-dns=false\ncheck-certificate=true\nca-certificate=/data/raff/ca-bundle.crt\nrpc-listen-all=false\nrpc-secret=%s\nmax-concurrent-downloads=3\n",secret);int ok=write(fd,cfg,n)==n&&!fsync(fd);close(fd);if(!ok)return -3;
 }
 int fd=open("/data/raff/native-v5/rpc-token.txt",O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0600);if(fd<0)return -3;int ok=write(fd,secret,strlen(secret))==(ssize_t)strlen(secret)&&!fsync(fd);close(fd);return ok?0:-3;
}
