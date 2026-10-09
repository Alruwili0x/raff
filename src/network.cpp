// Background networking keeps every service request off the render thread.
#include "network.hpp"
#include "platform/ps5/system.hpp"
#include <pthread.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <cstdio>
#include <cstring>
#include <deque>
#include <utility>
#include <vector>
#include <map>
extern "C"{int sceNetInit();int sceNetSocket(const char*,int,int,int);int sceNetConnect(int,const void*,unsigned);int sceNetSetsockopt(int,int,int,const void*,unsigned);int sceNetSend(int,const void*,size_t,int);int sceNetRecv(int,void*,size_t,int);int sceNetSocketClose(int);int sceNetShutdown(int,int);}
namespace raff{
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static Json latest={{"ready",false},{"jobs",Json::array()},{"settings",{{"parallel",3},{"connections",16},{"language","ar"},{"favorites",Json::array()}}}};
struct Request{std::string path,tag;Json body;uint64_t generation;};
static std::deque<Request> requests;static std::string notice;
static std::map<std::string,Json> results;
static std::map<std::string,uint64_t> generations;
static std::string token(){char b[65]={0};FILE*f=fopen("/app0/.session-token","rb");if(f){if(fread(b,1,64,f)!=64)b[0]=0;fclose(f);}return b;}
static int connect_port(int port){int fd=sceNetSocket("Raff",2,1,6);if(fd<0)return-1;int timeout=port==8794?12000000:3000000;for(int option:{0x1105,0x1106,0x1109})if(sceNetSetsockopt(fd,0xffff,option,&timeout,sizeof(timeout))<0){sceNetSocketClose(fd);return-1;}sockaddr_in a{};a.sin_len=sizeof(a);a.sin_family=2;a.sin_port=__builtin_bswap16(port);a.sin_addr.s_addr=0x0100007f;if(sceNetConnect(fd,&a,sizeof(a))<0){sceNetSocketClose(fd);return-1;}return fd;}
static bool sendall(int fd,const void*ptr,size_t n){auto*p=(const char*)ptr;while(n){int sent=sceNetSend(fd,p,n,0);if(sent<=0)return false;p+=sent;n-=sent;}return true;}
static Json http(const std::string&path,const std::string&body=""){int fd=connect_port(8794);if(fd<0){hui::sys::log("Raff HTTP connection unavailable");return Json();}std::string req=(body.empty()?"GET ":"POST ")+path+" HTTP/1.1\r\nHost: 127.0.0.1:8794\r\nConnection: close\r\nContent-Type: application/json\r\nX-Raff-Token: "+token()+"\r\nContent-Length: "+std::to_string(body.size())+"\r\n\r\n"+body;if(!sendall(fd,req.data(),req.size())){sceNetSocketClose(fd);return Json();}std::string data;char buffer[8192];int n;while((n=sceNetRecv(fd,buffer,sizeof(buffer),0))>0){data.append(buffer,n);if(data.size()>4*1024*1024)break;}sceNetSocketClose(fd);static int diagnostic=0;if(diagnostic++<16)hui::sys::log("Raff HTTP path=%s bytes=%zu recv=%d status=%.24s tokenLength=%zu",path.c_str(),data.size(),n,data.c_str(),token().size());auto at=data.find("\r\n\r\n");if(at==std::string::npos)return Json();if(data.compare(0,12,"HTTP/1.1 403")==0){hui::sys::log("Raff service rejected session authentication");return Json{{"error","local-authentication"}};}auto j=Json::parse(data.substr(at+4),nullptr,false);return j.is_discarded()?Json():j;}
static void boot_service(){FILE*lock=fopen("/data/raff/native-v5/updates/armed","rb");if(lock){fclose(lock);return;}FILE*f=fopen("/app0/raff-service.elf","rb");if(!f){hui::sys::log("Raff service payload missing");return;}int fd=connect_port(9021);if(fd<0){fclose(f);hui::sys::log("Raff ELF loader unavailable");return;}std::vector<char> buffer(65536);size_t n;while((n=fread(buffer.data(),1,buffer.size(),f)))if(!sendall(fd,buffer.data(),n))break;sceNetShutdown(fd,1);sceNetSocketClose(fd);fclose(f);hui::sys::log("Raff service sent to loader");}
static void*worker(void*){hui::sys::log("Raff network worker entered");int failures=0;for(;;){pthread_mutex_lock(&mutex);auto pending=std::move(requests);requests.clear();pthread_mutex_unlock(&mutex);
 for(const auto&r:pending){auto result=http(r.path,r.body.dump());pthread_mutex_lock(&mutex);if(!r.tag.empty()){if(generations[r.tag]==r.generation)results[r.tag]=result.is_null()?Json{{"error","offline"}}:std::move(result);}else if(result.is_null())notice="offline";else if(result.contains("error")){notice=result["error"].get<std::string>();hui::sys::log("Raff action failed");}else if(r.path!="/library/view")notice=r.path=="/queue"?"added":"saved";pthread_mutex_unlock(&mutex);}
 auto result=http("/status");if(result.is_object()&&result.contains("jobs")){failures=0;pthread_mutex_lock(&mutex);latest=std::move(result);pthread_mutex_unlock(&mutex);}else{if(failures==0&&result.is_null())boot_service();failures++;pthread_mutex_lock(&mutex);latest["ready"]=false;pthread_mutex_unlock(&mutex);if(failures>=15)failures=0;}hui::sys::sleep_us(800000);}return nullptr;}
void start_network(){
 // Native console threads have a small default stack. Inlining the former
 // 64 KiB payload buffer alone made worker's entry frame exceed that limit.
 pthread_attr_t attributes;
 int rc=pthread_attr_init(&attributes);
 hui::sys::log("Raff network v0.5.1 attr init rc=%d",rc);
 if(rc!=0)return;
 rc=pthread_attr_setstacksize(&attributes,2*1024*1024);
 if(rc==0)rc=pthread_attr_setdetachstate(&attributes,PTHREAD_CREATE_DETACHED);
 if(rc==0){
  pthread_t thread;
  hui::sys::log("Raff creating network thread with 2 MiB stack");
  rc=pthread_create(&thread,&attributes,worker,nullptr);
  hui::sys::log("Raff network thread create rc=%d",rc);
 }
 pthread_attr_destroy(&attributes);
 if(rc!=0)hui::sys::log("Raff network worker failed rc=%d",rc);
}
Json status(){pthread_mutex_lock(&mutex);Json copy=latest;pthread_mutex_unlock(&mutex);return copy;}
void request(const std::string&t,const std::string&p,const Json&b){pthread_mutex_lock(&mutex);auto generation=++generations[t];for(auto it=requests.begin();it!=requests.end();)if(!t.empty()&&it->tag==t)it=requests.erase(it);else ++it;if(requests.size()<32)requests.push_back({p,t,b,generation});pthread_mutex_unlock(&mutex);}
void command(const std::string&p,const Json&b){request("",p,b);}
bool take_result(const std::string&t,Json&out){pthread_mutex_lock(&mutex);auto it=results.find(t);bool found=it!=results.end();if(found){out=std::move(it->second);results.erase(it);}pthread_mutex_unlock(&mutex);return found;}
std::string take_notice(){pthread_mutex_lock(&mutex);std::string result=std::move(notice);notice.clear();pthread_mutex_unlock(&mutex);return result;}
}
