#pragma once
#include <string>
#include <vector>
namespace raff {
struct Artwork { std::string key; int width=0,height=0,contentY=0,contentHeight=0; std::vector<unsigned char> pixels; };
void start_artwork();
void request_artwork(const std::string& key);
bool take_artwork(Artwork& result);
}
