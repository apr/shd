
#include <map>
#include <string>

#include "ini-file-parser.h"

#include <gtest/gtest.h>


namespace ini {

namespace {

bool check_kv(const kv_map_t &vals,
              const std::string &key,
              const std::string &value)
{ 
    kv_map_t::const_iterator it = vals.find(key);

    if(it == vals.end()) {
        return false;
    }

    return it->second == value;
}

}


TEST(IniFileParserTest, EmptyFile)
{
    kv_map_t vals;
    parse_ini_file("", &vals);
    EXPECT_TRUE(vals.empty());
}


TEST(IniFileParserTest, SimleKeyValuePair)
{
    kv_map_t vals;
    parse_ini_file("key = value", &vals);
    EXPECT_TRUE(check_kv(vals, "key", "value"));
}


TEST(IniFileParserTest, EmptyLines)
{
    kv_map_t vals;
    parse_ini_file("\r", &vals);
    EXPECT_TRUE(vals.empty());
    parse_ini_file(" \r", &vals);
    EXPECT_TRUE(vals.empty());
    parse_ini_file(" ", &vals);
    EXPECT_TRUE(vals.empty());
    parse_ini_file("\r \r\r ", &vals);
    EXPECT_TRUE(vals.empty());
}


TEST(IniFileParserTest, MultipleKVLines)
{
    kv_map_t vals;
    parse_ini_file("key1 = val1\rkey2 = val2\nkey3 = val3", &vals);
    EXPECT_EQ(3, vals.size());
    EXPECT_EQ("val1", vals["key1"]);
    EXPECT_EQ("val2", vals["key2"]);
    EXPECT_EQ("val3", vals["key3"]);
}


}

