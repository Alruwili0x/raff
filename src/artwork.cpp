// Decode local artwork away from the frame loop. GL upload stays on that loop.
#include "artwork.hpp"
#include "vendor/stb_image.h"
#include "platform/ps5/system.hpp"
#include <pthread.h>
#include <deque>
#include <algorithm>
namespace raff {
static pthread_mutex_t guard=PTHREAD_MUTEX_INITIALIZER;
static std::deque<std::string> waiting;
static std::deque<Artwork> completed;
static void* loader(void*) {
 for(;;) {
  std::string key;
  pthread_mutex_lock(&guard);
  if(!waiting.empty()&&completed.size()<6){key=waiting.front();waiting.pop_front();}
  pthread_mutex_unlock(&guard);
  if(key.empty()){hui::sys::sleep_us(10000);continue;}
  Artwork result;result.key=key;int channels;
  std::string path=key.rfind("media-",0)==0?"/app0/assets/media/"+key+".png":"/app0/assets/covers/"+key+".jpg";
  int probeW=0,probeH=0,probeC=0;bool safe=key.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-_")==std::string::npos&&stbi_info(path.c_str(),&probeW,&probeH,&probeC)&&probeW>0&&probeH>0&&probeW<=2048&&probeH<=2048;
  auto* pixels=safe?stbi_load(path.c_str(),&result.width,&result.height,&channels,4):nullptr;
  if(pixels&&result.width>0&&result.height>0&&result.width<=2048&&result.height<=2048){
   const float maxSize=key.rfind("hero-",0)==0?1280.f:512.f;
   if(result.width>maxSize||result.height>maxSize){int oldW=result.width,oldH=result.height;float scale=std::min(maxSize/oldW,maxSize/oldH);result.width=std::max(1,int(oldW*scale));result.height=std::max(1,int(oldH*scale));result.pixels.resize(size_t(result.width)*result.height*4);for(int y=0;y<result.height;y++)for(int x=0;x<result.width;x++){int sx=x*oldW/result.width,sy=y*oldH/result.height;for(int c=0;c<4;c++)result.pixels[(y*result.width+x)*4+c]=pixels[(sy*oldW+sx)*4+c];}}
   else result.pixels.assign(pixels,pixels+size_t(result.width)*result.height*4);
   result.contentHeight=result.height;
   // Our JPEG cache pads square source icons to 300x400. Trim only that
   // known padding color so the store can show the artwork at its full size.
   if(result.width==300&&result.height==400){
    auto padding=[&](int y){for(int x=3;x<result.width-3;x+=5){auto*p=pixels+(y*result.width+x)*4;if(std::abs(int(p[0])-18)>8||std::abs(int(p[1])-25)>8||std::abs(int(p[2])-40)>8)return false;}return true;};
    int top=0,bottom=399;while(top<100&&padding(top))top++;while(bottom>299&&padding(bottom))bottom--;
    result.contentY=top;result.contentHeight=bottom-top+1;
   }
  }
  stbi_image_free(pixels);
  pthread_mutex_lock(&guard);completed.push_back(std::move(result));pthread_mutex_unlock(&guard);
 }
 return nullptr;
}
void start_artwork(){
 pthread_attr_t attr;int rc=pthread_attr_init(&attr);if(rc)return;
 rc=pthread_attr_setstacksize(&attr,2*1024*1024);
 if(!rc)rc=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
 if(!rc){pthread_t thread;rc=pthread_create(&thread,&attr,loader,nullptr);}
 pthread_attr_destroy(&attr);hui::sys::log("Raff artwork worker rc=%d",rc);
}
void request_artwork(const std::string&key){
 if(key.empty())return;
 pthread_mutex_lock(&guard);
 if(std::find(waiting.begin(),waiting.end(),key)==waiting.end()){
  if(waiting.size()>=96){Artwork dropped;dropped.key=waiting.back();waiting.pop_back();completed.push_back(std::move(dropped));}waiting.push_front(key);
 }
 pthread_mutex_unlock(&guard);
}
bool take_artwork(Artwork&out){
 pthread_mutex_lock(&guard);bool found=!completed.empty();
 if(found){out=std::move(completed.front());completed.pop_front();}
 pthread_mutex_unlock(&guard);return found;
}
}
