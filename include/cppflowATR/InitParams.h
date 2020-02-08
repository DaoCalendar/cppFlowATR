#include <iostream>
#include <fstream>
#include <iomanip>
#include <json.hpp>
#include <map>
#include <string>
#include <vector>

#define dict std::map<std::string, std::string>

using json = nlohmann::json;

class InitParams
{
public:
    InitParams(std::string filepath);

    dict info;
    dict run_params;
    std::vector<dict> models;

protected:
    json j;
};