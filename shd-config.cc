
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ini-file-parser.h"
#include "plm-util.h"
#include "shd-config.h"


// May throw shd_config_exception.
static std::string read_file_content(const std::string &file_path)
{
    FILE *file = fopen(file_path.c_str(), "r");

    if(file == 0) {
        throw shd_config_exception(std::string("Cannot open file '") +
            file_path + "': " + strerror(errno));
    }

    std::string ret;

    while(!feof(file) && !ferror(file)) {
        char buf[1024];
        size_t len = fread(buf, 1, sizeof(buf), file);

        if(len > 0) {
            ret.append(buf, len);
        }
    }

    int error = ferror(file);

    fclose(file);

    if(error > 0) {
        throw shd_config_exception(std::string("Error reading file '") +
            file_path + "': " + strerror(error));
    }

    return ret;
}


shd_config_exception::shd_config_exception(const std::string &msg)
    : msg_(msg)
{
}


shd_config_exception::~shd_config_exception() throw()
{
}


const char *shd_config_exception::what() const throw()
{
    return msg_.c_str();
}


shd_config::shd_config()
    : longitude_(0), latitude_(0)
{
    char *home = getenv("HOME");

    if(!home) {
        throw shd_config_exception("Cannot read .shdrc, "
            "environment variable HOME is not set.");
    }

    read_config(std::string(home) + "/.shdrc");
}


shd_config::shd_config(const std::string &file_path)
    : longitude_(0), latitude_(0)
{
    read_config(file_path);
}


std::string shd_config::serial_device() const
{
    return serial_device_;
}


double shd_config::longitude() const
{
    return longitude_;
}


double shd_config::latitude() const
{
    return latitude_;
}


const std::vector<std::string> &shd_config::outside_lights() const
{
    return outside_lights_;
}


void shd_config::read_config(const std::string &file_path)
{
    ini::kv_map_t vals;
    std::string content = read_file_content(file_path);

    try {
        ini::parse_ini_file(content, &vals);
    } catch(ini::parse_exception &ex) {
        throw shd_config_exception(
            std::string("Could not parse config: ") + ex.what());
    }

    ini::kv_map_t::iterator it = vals.find("latitude");
    if(it != vals.end()) {
        latitude_ = atof(it->second.c_str());
    }

    it = vals.find("longitude");
    if(it != vals.end()) {
        longitude_ = atof(it->second.c_str());
    }

    it = vals.find("serial-device");
    if(it != vals.end()) {
        serial_device_ = it->second;
    }

    it = vals.find("outside-lights");
    if(it != vals.end()) {
        ini::parse_list(it->second, &outside_lights_);
        for(size_t i = 0; i < outside_lights_.size(); ++i) {
            outside_lights_[i] = plm::hex_to_bin(outside_lights_[i]);
        }
    }
}

