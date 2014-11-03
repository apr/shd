
#include <functional>
#include <memory>
#include <string>

#include "buffered-connection.h"
#include "event-manager.h"
#include "executor.h"
#include "mock-event-manager.h"
#include "mock-executor.h"

#include <gtest/gtest.h>

namespace net {


// TODO simulate EAGAINs and errors in general
// TODO the name could be better
class one_char_fd : public fd_interface {
public:
    one_char_fd()
        : read_pos_(0),
          is_closed_(true),
          do_read_again_(false),
          do_write_again_(false) {
    }

    void set_read_buf(const std::string &buf) {
        read_buf_ = buf;
        read_pos_ = 0;
    }

    virtual void open() { is_closed_ = false; }
    virtual void close() { is_closed_ = true; }

    virtual int get_fd() { return 1; }

    virtual int read(void *buf, int count) {
        if(do_read_again_) {
            return -1;
        }

        if(read_pos_ == read_buf_.size()) {
            return 0;
        }

        *static_cast<char *>(buf) = read_buf_[read_pos_++];
        return 1;
    }

    virtual int write(const void *buf, int count) {
        if(do_write_again_) {
            return -1;
        }

        write_buf_.append(static_cast<const char *>(buf), 1);

        return 1;
    }

    bool is_closed() const { return is_closed_; }

    std::string get_write_buf() const { return write_buf_; }

    void set_read_again() { do_read_again_ = true; }
    void clear_read_again() { do_read_again_ = false; }
    void set_write_again() { do_write_again_ = true; }
    void clear_write_again() { do_write_again_ = false; }

private:
    size_t read_pos_;
    bool is_closed_;
    std::string read_buf_;
    std::string write_buf_;
    bool do_read_again_;
    bool do_write_again_;
};


class BufferedConnectionTest : public testing::Test {
public:
    virtual void SetUp() {
        executor_.reset(new mock_executor);
        event_manager_.reset(new mock_event_manager);
        fd_.reset(new one_char_fd);
        conn_.reset(new buffered_connection(
            fd_.get(), event_manager_.get(), executor_.get()));
        done_ = false;
    }

    void loop_once() {
        done_ = false;
        event_manager_->send_signal();
        executor_->run_all_now();
    }

    void done_callback() {
        done_ = true;
    }

    std::unique_ptr<mock_executor> executor_;
    std::unique_ptr<mock_event_manager> event_manager_;
    std::unique_ptr<one_char_fd> fd_;
    std::unique_ptr<buffered_connection> conn_;
    bool done_;
};


TEST_F(BufferedConnectionTest, OpenClose)
{
    EXPECT_TRUE(conn_->is_ok());
    EXPECT_TRUE(conn_->is_closed());

    conn_->start();
    EXPECT_TRUE(conn_->is_ok());
    EXPECT_FALSE(conn_->is_closed());
    EXPECT_FALSE(fd_->is_closed());

    conn_->stop();
    EXPECT_TRUE(conn_->is_ok());
    EXPECT_TRUE(conn_->is_closed());
    EXPECT_TRUE(fd_->is_closed());
}

TEST_F(BufferedConnectionTest, SimpleRead)
{
    char buf[3];
    fd_->set_read_buf("abc");
    conn_->start();
    EXPECT_TRUE(conn_->is_ok());

    buf[0] = buf[1] = buf[2] = 0;
    conn_->read(buf, sizeof(buf),
        std::bind(&BufferedConnectionTest::done_callback, this));
    loop_once();

    EXPECT_EQ('a', buf[0]);
    EXPECT_EQ('b', buf[1]);
    EXPECT_EQ('c', buf[2]);
    EXPECT_TRUE(done_);
    EXPECT_TRUE(conn_->is_ok());

    buf[0] = buf[1] = buf[2] = 0;
    conn_->read(buf, sizeof(buf),
        std::bind(&BufferedConnectionTest::done_callback, this));
    loop_once();

    EXPECT_EQ(0, buf[0]);
    EXPECT_EQ(0, buf[1]);
    EXPECT_EQ(0, buf[2]);
    EXPECT_TRUE(done_);
    EXPECT_TRUE(conn_->is_ok());
    EXPECT_TRUE(conn_->is_closed());
}

TEST_F(BufferedConnectionTest, SimpleWrite)
{
    char buf[3];

    buf[0] = 'a';
    buf[1] = 'b';
    buf[2] = 'c';

    conn_->start();
    EXPECT_TRUE(conn_->is_ok());

    conn_->write(buf, 1,
        std::bind(&BufferedConnectionTest::done_callback, this));
    loop_once();

    EXPECT_TRUE(conn_->is_ok());
    EXPECT_TRUE(done_);
    EXPECT_EQ("a", fd_->get_write_buf());

    conn_->write(buf, 3,
        std::bind(&BufferedConnectionTest::done_callback, this));
    loop_once();

    EXPECT_TRUE(conn_->is_ok());
    EXPECT_TRUE(done_);
    EXPECT_EQ("aabc", fd_->get_write_buf());
}

TEST_F(BufferedConnectionTest, InterruptedRead)
{
    char buf[3];
    fd_->set_read_buf("abc");
    conn_->start();
    EXPECT_TRUE(conn_->is_ok());

    buf[0] = buf[1] = buf[2] = 0;
    fd_->set_read_again();
    conn_->read(buf, sizeof(buf),
        std::bind(&BufferedConnectionTest::done_callback, this));
    loop_once();

    EXPECT_TRUE(conn_->is_ok());
    EXPECT_FALSE(done_);

    loop_once();

    EXPECT_TRUE(conn_->is_ok());
    EXPECT_FALSE(done_);

    fd_->clear_read_again();
    loop_once();

    EXPECT_TRUE(conn_->is_ok());
    EXPECT_TRUE(done_);
    EXPECT_EQ('a', buf[0]);
    EXPECT_EQ('b', buf[1]);
    EXPECT_EQ('c', buf[2]);
}

TEST_F(BufferedConnectionTest, InterruptedWrite)
{
    char buf[] = "abc";

    conn_->start();
    EXPECT_TRUE(conn_->is_ok());

    fd_->set_write_again();
    conn_->write(buf, 3,
        std::bind(&BufferedConnectionTest::done_callback, this));
    loop_once();

    EXPECT_TRUE(conn_->is_ok());
    EXPECT_FALSE(done_);

    loop_once();

    EXPECT_TRUE(conn_->is_ok());
    EXPECT_FALSE(done_);

    fd_->clear_write_again();
    loop_once();

    EXPECT_TRUE(conn_->is_ok());
    EXPECT_TRUE(done_);
    EXPECT_EQ("abc", fd_->get_write_buf());
}

}

