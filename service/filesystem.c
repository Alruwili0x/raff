/* Bounded file access for import/validation. Directory symlinks are not followed.
 * Writes are confined to named exports and the PSXS5 index/artwork integration. */
#include "filesystem.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <openssl/md5.h>
#define EXPORT "/data/raff/native-v5/exports/"
static int allowed(const char*p){
  if(!p || strlen(p)>=PATH_MAX || p[0]!='/' || strstr(p,"/../") || strstr(p,"/./") || strchr(p,'\\') || strchr(p,'\n') || strchr(p,'\r'))return 0;
  size_t n=strlen(p);if(n>=3&&!strcmp(p+n-3,"/.."))return 0;
  if(strcmp(p,"/data")&&strncmp(p,"/data/",6)&&strcmp(p,"/mnt")&&strncmp(p,"/mnt/",5))return 0;
  // Reject a symlink in every existing component, including the final component.
  char path[PATH_MAX];strcpy(path,p);for(char *x=path+1;;x++){if(*x!='/'&&*x)continue;char old=*x;*x=0;struct stat st;if(lstat(path,&st)==0&&S_ISLNK(st.st_mode))return 0;*x=old;if(!old)break;}return 1;
}
static JSValue fs(JSContext*c,JSValueConst self,int argc,JSValueConst*argv){
  (void)self;if(argc<2)return JS_ThrowTypeError(c,"Filesystem arguments");
  size_t path_length=0;const char *op=JS_ToCString(c,argv[0]),*path=JS_ToCStringLen(c,&path_length,argv[1]);JSValue out=JS_UNDEFINED;
  if(!op||!path||strlen(path)!=path_length||!allowed(path)){out=JS_ThrowTypeError(c,"Path rejected");goto end;}
  if(!strcmp(op,"list")){
    int offset=0;if(argc>2)JS_ToInt32(c,&offset,argv[2]);if(offset<0||offset>1000000){out=JS_ThrowRangeError(c,"Directory offset");goto end;}
    DIR*d=opendir(path);if(!d){out=JS_ThrowInternalError(c,"Directory: %s",strerror(errno));goto end;}
    JSValue rows=JS_NewArray(c);struct dirent*e;int index=0,count=0,more=0;
    while((e=readdir(d))){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;if(index++<offset)continue;if(count==256){more=1;break;}
      char full[PATH_MAX];int n=snprintf(full,sizeof(full),"%s/%s",path,e->d_name);struct stat st;if(n<0||n>=(int)sizeof(full)||lstat(full,&st))continue;
      JSValue row=JS_NewObject(c);JS_SetPropertyStr(c,row,"name",JS_NewString(c,e->d_name));JS_SetPropertyStr(c,row,"path",JS_NewString(c,full));JS_SetPropertyStr(c,row,"type",JS_NewString(c,S_ISDIR(st.st_mode)?"directory":S_ISREG(st.st_mode)?"file":"unsupported"));JS_SetPropertyStr(c,row,"size",JS_NewFloat64(c,(double)st.st_size));JS_SetPropertyStr(c,row,"mtime",JS_NewFloat64(c,(double)st.st_mtime));JS_SetPropertyUint32(c,rows,count++,row);
    }closedir(d);out=JS_NewObject(c);JS_SetPropertyStr(c,out,"entries",rows);JS_SetPropertyStr(c,out,"next",more?JS_NewInt32(c,index-1):JS_NULL);
  }else if(!strcmp(op,"slice")){
    if(argc<3){out=JS_ThrowTypeError(c,"Slice arguments");goto end;}JSValue ov=JS_GetPropertyStr(c,argv[2],"offset"),lv=JS_GetPropertyStr(c,argv[2],"length");double offset=0;int length=0;JS_ToFloat64(c,&offset,ov);JS_ToInt32(c,&length,lv);JS_FreeValue(c,ov);JS_FreeValue(c,lv);
    if(offset<0||offset>9007199254740991.0||length<0||length>1048576){out=JS_ThrowRangeError(c,"Slice limit");goto end;}int fd=open(path,O_RDONLY|O_NOFOLLOW);struct stat st;if(fd<0||fstat(fd,&st)||!S_ISREG(st.st_mode)||lseek(fd,(off_t)offset,SEEK_SET)<0){if(fd>=0)close(fd);out=JS_ThrowInternalError(c,"Slice unavailable");goto end;}unsigned char*b=malloc(length?length:1);if(!b){close(fd);out=JS_ThrowOutOfMemory(c);goto end;}size_t used=0;while(used<(size_t)length){ssize_t n=read(fd,b+used,length-used);if(n<0&&errno==EINTR)continue;if(n<=0)break;used+=n;}close(fd);out=JS_NewArrayBufferCopy(c,b,used);free(b);
  }else if(!strcmp(op,"read")){
    int fd=open(path,O_RDONLY|O_NOFOLLOW);struct stat st;if(fd<0||fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0||st.st_size>1048576){if(fd>=0)close(fd);out=JS_ThrowInternalError(c,"Cannot read this file (1 MiB text limit)");goto end;}
    size_t n=(size_t)st.st_size;char *b=malloc(n+1);if(!b){close(fd);out=JS_ThrowOutOfMemory(c);goto end;}size_t used=0;while(used<n){ssize_t r=read(fd,b+used,n-used);if(r<0&&errno==EINTR)continue;if(r<=0)break;used+=r;}close(fd);out=used==n?JS_NewStringLen(c,b,n):JS_ThrowInternalError(c,"Incomplete read");free(b);
  }else if(!strcmp(op,"md5")){
    int fd=open(path,O_RDONLY|O_NOFOLLOW);struct stat st;if(fd<0||fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0||st.st_size>67108864){if(fd>=0)close(fd);out=JS_ThrowRangeError(c,"Firmware hash: 64 MiB limit");goto end;}
    MD5_CTX hash;MD5_Init(&hash);unsigned char buffer[32768],digest[16];ssize_t count;while((count=read(fd,buffer,sizeof(buffer)))>0)MD5_Update(&hash,buffer,count);close(fd);if(count<0){out=JS_ThrowInternalError(c,"Firmware read failed");goto end;}MD5_Final(digest,&hash);char hex[33];for(int i=0;i<16;i++)snprintf(hex+i*2,3,"%02x",digest[i]);out=JS_NewString(c,hex);
  }else if(!strcmp(op,"stat")){
    struct stat st;if(lstat(path,&st)){out=JS_NULL;goto end;}out=JS_NewObject(c);JS_SetPropertyStr(c,out,"size",JS_NewFloat64(c,(double)st.st_size));JS_SetPropertyStr(c,out,"mtime",JS_NewFloat64(c,(double)st.st_mtime));JS_SetPropertyStr(c,out,"type",JS_NewString(c,S_ISDIR(st.st_mode)?"directory":S_ISREG(st.st_mode)?"file":"unsupported"));
  }else if(!strcmp(op,"space")){
    struct statvfs s;if(statvfs(path,&s))out=JS_ThrowInternalError(c,"Space: %s",strerror(errno));else out=JS_NewFloat64(c,(double)s.f_bavail*s.f_frsize);
  }else if(!strcmp(op,"psxs5-index")){
    const char *fixed="/data/PSXS5/library.txt",*temp="/data/PSXS5/.raff-library.new",*marker="# PSXS5 library index - managed by Raff\n";
    if(strcmp(path,fixed)||!allowed(temp)||argc<3){out=JS_ThrowTypeError(c,"PSXS5 index path rejected");goto end;}
    size_t n=0;const char*b=JS_ToCStringLen(c,&n,argv[2]);
    if(!b||n>1048576||strlen(b)!=n||strncmp(b,marker,strlen(marker))){JS_FreeCString(c,b);out=JS_ThrowTypeError(c,"PSXS5 index rejected");goto end;}
    int old=open(path,O_RDONLY|O_NOFOLLOW);if(old>=0){char head[80]={0};ssize_t got=read(old,head,strlen(marker));close(old);if(got!=(ssize_t)strlen(marker)||strncmp(head,marker,strlen(marker))){JS_FreeCString(c,b);out=JS_ThrowTypeError(c,"Preserve existing PSXS5 index");goto end;}}
    int fd=open(temp,O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0644);size_t used=0;if(fd>=0){while(used<n){ssize_t r=write(fd,b+used,n-used);if(r<0&&errno==EINTR)continue;if(r<=0)break;used+=r;}}
    int ok=fd>=0&&used==n&&!fsync(fd);if(fd>=0)close(fd);JS_FreeCString(c,b);if(ok)ok=!rename(temp,path);if(!ok){unlink(temp);out=JS_ThrowInternalError(c,"PSXS5 index incomplete");}else out=JS_NewBool(c,1);
  }else if(!strcmp(op,"psxs5-cover")){
    const char*prefix="/data/PSXS5/covers/";if(strncmp(path,prefix,strlen(prefix))||strchr(path+strlen(prefix),'/')||strlen(path)>320||strlen(path)<4||strcmp(path+strlen(path)-4,".jpg")||argc<3){out=JS_ThrowTypeError(c,"PSXS5 cover path rejected");goto end;}
    struct stat exists;if(!lstat(path,&exists)){out=JS_NewBool(c,S_ISREG(exists.st_mode));goto end;}
    const char*key=JS_ToCString(c,argv[2]);if(!key||!strlen(key)||strlen(key)>80||strspn(key,"abcdefghijklmnopqrstuvwxyz0123456789-_")!=strlen(key)){JS_FreeCString(c,key);out=JS_ThrowTypeError(c,"PSXS5 artwork key rejected");goto end;}
    char source[200];snprintf(source,sizeof(source),"/data/homebrew/PPSA99178/assets/covers/%s.jpg",key);JS_FreeCString(c,key);int in=open(source,O_RDONLY|O_NOFOLLOW);struct stat st;if(in<0||fstat(in,&st)||!S_ISREG(st.st_mode)||st.st_size<24||st.st_size>4194304){if(in>=0)close(in);out=JS_NewBool(c,0);goto end;}
    int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644);if(fd<0){close(in);out=JS_ThrowInternalError(c,"PSXS5 cover cannot be created");goto end;}
    char buf[32768];ssize_t got;off_t total=0;int ok=1;while((got=read(in,buf,sizeof(buf)))>0){size_t at=0;while(at<(size_t)got){ssize_t n=write(fd,buf+at,(size_t)got-at);if(n<0&&errno==EINTR)continue;if(n<=0){ok=0;break;}at+=n;}if(!ok)break;total+=got;}if(got<0||total!=st.st_size||fsync(fd))ok=0;close(in);close(fd);if(!ok)unlink(path);out=JS_NewBool(c,ok);
  }else if(!strcmp(op,"export")||!strcmp(op,"playlist")){
    const char*name=strrchr(path,'/');int playlist_ok=!strcmp(op,"playlist")&&name&&!strncmp(name+1,"Raff-",5)&&strlen(name+1)==41&&!strcmp(name+38,".m3u");
    const char*retro="/data/homebrew/PPSA99169/playlists/";
    int retro_ok=!strncmp(path,retro,strlen(retro))&&name&&!strncmp(name+1,"Raff-",5)&&strlen(name+1)==41&&!strcmp(name+38,".lpl");
    if((!playlist_ok&&!retro_ok&&(strncmp(path,EXPORT,strlen(EXPORT))||strchr(path+strlen(EXPORT),'/')))||argc<3){out=JS_ThrowTypeError(c,"Export path rejected");goto end;}
    mkdir("/data/raff/native-v5/exports",0755);
    size_t n=0;const char*b=JS_ToCStringLen(c,&n,argv[2]);if(!b||n>1048576){JS_FreeCString(c,b);out=JS_ThrowRangeError(c,"Export limit");goto end;}
    int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644);if(fd<0){JS_FreeCString(c,b);out=JS_ThrowInternalError(c,"Export exists or cannot be created");goto end;}
    size_t used=0;while(used<n){ssize_t r=write(fd,b+used,n-used);if(r<0&&errno==EINTR)continue;if(r<=0)break;used+=r;}int rc=fsync(fd);close(fd);JS_FreeCString(c,b);if(used!=n||rc){unlink(path);out=JS_ThrowInternalError(c,"Export incomplete");}else out=JS_NewString(c,path);
  }else out=JS_ThrowTypeError(c,"Unknown filesystem operation");
end:JS_FreeCString(c,op);JS_FreeCString(c,path);return out;
}
void raff_filesystem_init(JSContext*c){JSValue g=JS_GetGlobalObject(c);JS_SetPropertyStr(c,g,"nativeFS",JS_NewCFunction(c,fs,"nativeFS",3));JS_FreeValue(c,g);}
