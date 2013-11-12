
#ifndef PLM_UTIL_H_
#define PLM_UTIL_H_

#include <string>


namespace plm {

// Converts binary data to their character representaion in the hex form, e.g.
// 0x10 -> 10, 0x01 -> 01.
std::string bin_to_hex(const std::string &bin);


// Converts from ascii hex representation to binary data.
std::string hex_to_bin(const std::string &hex);

}

#endif

