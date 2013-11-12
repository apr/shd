
#include "plm-util.h"

#include <gtest/gtest.h>


namespace plm {


TEST(plm_util_test, bin_to_hex)
{
    std::string data;

    EXPECT_EQ("", bin_to_hex(""));

    data += 0x01;
    EXPECT_EQ("01", bin_to_hex(data));

    data += 0x10;
    EXPECT_EQ("0110", bin_to_hex(data));

    data += 0xfa;
    EXPECT_EQ("0110FA", bin_to_hex(data));
}


TEST(plm_util_test, hex_to_bin)
{
    std::string expected;

    EXPECT_EQ("", hex_to_bin(""));

    expected += 0x01;
    EXPECT_EQ(expected, hex_to_bin("01"));

    expected += 0x10;
    EXPECT_EQ(expected, hex_to_bin("0110"));

    expected += 0xfa;
    EXPECT_EQ(expected, hex_to_bin("0110FA"));
    EXPECT_EQ(expected, hex_to_bin("0110fa"));

    expected += 0xff;
    EXPECT_EQ(expected, hex_to_bin("0110FAFF"));
    EXPECT_EQ(expected, hex_to_bin("0110faff"));
}


}

