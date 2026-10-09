#pragma once
#include "vendor/json.hpp"
#include <string>
namespace raff{
using Json=nlohmann::json;
void start_network();
Json status();
void command(const std::string&path,const Json&body);
void request(const std::string&tag,const std::string&path,const Json&body=Json::object());
bool take_result(const std::string&tag,Json&value);
std::string take_notice();
}
