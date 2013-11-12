
#include "io-buffer.h"

#include <utility>

#include <gtest/gtest.h>


namespace net {

TEST(IoBufferTest, Empty)
{
    io_buffer buf(10);
    std::pair<char *, int> write_buf = buf.get_raw_write_buffer();
    EXPECT_TRUE(write_buf.first != 0);
    EXPECT_EQ(10, write_buf.second);
    EXPECT_EQ(0, buf.read_size());
    EXPECT_TRUE(buf.empty());

    char read_buf[5];
    int bytes_read = buf.read(read_buf, 5);
    EXPECT_EQ(0, bytes_read);
}


TEST(IoBufferTest, WriteTest)
{
    io_buffer buf(10);
    std::pair<char *, int> write_buf1 = buf.get_raw_write_buffer();
    buf.advance_write_pointer(6);
    std::pair<char *, int> write_buf2 = buf.get_raw_write_buffer();
    EXPECT_EQ(4, write_buf2.second);
    EXPECT_EQ(6, write_buf2.first - write_buf1.first);
    EXPECT_EQ(6, buf.read_size());
    EXPECT_FALSE(buf.empty());
}


TEST(IoBufferTest, ReadTest)
{
    io_buffer buf(10);

    std::pair<char *, int> write_buf = buf.get_raw_write_buffer();
    ASSERT_EQ(10, write_buf.second);

    for(char i = 0; i < 6; ++i) {
        *(write_buf.first + i) = i;
    }

    buf.advance_write_pointer(6);

    char read_buf[4];
    int bytes_read = buf.read(read_buf, 4);
    EXPECT_EQ(4, bytes_read);
    EXPECT_EQ(0, read_buf[0]);
    EXPECT_EQ(1, read_buf[1]);
    EXPECT_EQ(2, read_buf[2]);
    EXPECT_EQ(3, read_buf[3]);
    EXPECT_FALSE(buf.empty());

    bytes_read = buf.read(read_buf, 4);
    EXPECT_EQ(2, bytes_read);
    EXPECT_EQ(4, read_buf[0]);
    EXPECT_EQ(5, read_buf[1]);
    EXPECT_TRUE(buf.empty());
}


TEST(IoBufferTest, BufferOverCapacity)
{
    io_buffer buf(2);

    std::pair<char *, int> write_buf1 = buf.get_raw_write_buffer();
    *write_buf1.first = 1;
    *(write_buf1.first + 1) = 2;
    buf.advance_write_pointer(2);

    EXPECT_EQ(2, buf.read_size());

    std::pair<char *, int> write_buf2 = buf.get_raw_write_buffer();
    *write_buf2.first = 3;
    *(write_buf2.first + 1) = 4;
    buf.advance_write_pointer(2);

    EXPECT_EQ(4, buf.read_size());

    char read_buf[4];
    int bytes_read = buf.read(read_buf, 4);
    EXPECT_EQ(4, bytes_read);
    EXPECT_EQ(1, read_buf[0]);
    EXPECT_EQ(2, read_buf[1]);
    EXPECT_EQ(3, read_buf[2]);
    EXPECT_EQ(4, read_buf[3]);
    EXPECT_TRUE(buf.empty());
}


TEST(IoBufferTest, SingleEOF)
{
    io_buffer buf(2);
    buf.write_eof();
    EXPECT_FALSE(buf.empty());
    char read_buf[4];
    int bytes_read = buf.read(read_buf, 4);
    EXPECT_EQ(0, bytes_read);
    EXPECT_TRUE(buf.empty());
}


TEST(IoBufferTest, ReadWithEOF)
{
    io_buffer buf(2);
    std::pair<char *, int> write_buf1 = buf.get_raw_write_buffer();
    buf.advance_write_pointer(2);
    std::pair<char *, int> write_buf2 = buf.get_raw_write_buffer();
    buf.advance_write_pointer(1);
    buf.write_eof();

    *write_buf1.first = 1;
    *write_buf2.first = 1;
    EXPECT_EQ(3, buf.read_size());

    char read_buf[3];
    int bytes_read = buf.read(read_buf, sizeof(read_buf));

    EXPECT_EQ(3, bytes_read);
    EXPECT_EQ(0, buf.read_size());
    EXPECT_FALSE(buf.empty());

    bytes_read = buf.read(read_buf, sizeof(read_buf));

    EXPECT_EQ(0, bytes_read);
    EXPECT_TRUE(buf.empty());
}

}

