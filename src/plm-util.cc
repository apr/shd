
#include "plm-util.h"


namespace plm {


std::string bin_to_hex(const std::string &bin)
{
    std::string ret;

    for(int i = 0; i < bin.size(); ++i) {
        char c1 = (bin[i] & 0xf0) >> 4;
        char c2 = bin[i] & 0x0f;
        ret += (c1 > 9 ? c1 + '7' : c1 + '0');
        ret += (c2 > 9 ? c2 + '7' : c2 + '0');
    }

    return ret;
}


std::string hex_to_bin(const std::string &hex)
{
    std::string ret;

    for(int i = 0; i < hex.size(); i += 2) {
        char c1 = (hex[i] - (hex[i] > '9' ? '7' : '0')) << 4;
        char c2 = hex[i + 1] - (hex[i + 1] > '9' ? '7' : '0');
        ret += c1 | c2;
    }

    return ret;
}


int command_data_size(int cmd)
{
    switch(cmd) {
    case 0x50: return 9;
    case 0x60: return 7;
    }

    return -1;
}


}

