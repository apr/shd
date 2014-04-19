
#include <algorithm>
#include <memory>
#include <string>

#include <string.h>

#include "plm-connection.h"
#include "mock-event-manager.h"
#include "mock-executor.h"

#include <gtest/gtest.h>


namespace plm {


class test_fd : public net::fd_interface {
public:
    test_fd() : is_closed_(true), read_pos_(0) {}

    virtual void open() {
        is_closed_ = false;
    }

    virtual void close() {
        is_closed_ = true;
    }

    virtual int get_fd() { return 1; }

    virtual int read(void *buf, int count) {
        size_t len = std::min((size_t)count, read_buf_.size() - read_pos_);

        if(len > 0) {
            memcpy(buf, read_buf_.c_str() + read_pos_, len);
            read_pos_ += len;
        }

        // If there is no more data in the buffer for now return EAGAIN.
        return len > 0 ? len : -1;
    }

    virtual int write(const void *buf, int count) {
        write_buf_.append((const char *)buf, count);
        return count;
    }

    std::string get_write_buf() const {
        return write_buf_;
    }

    void set_read_buf(const std::string &buf) {
        read_buf_ = buf;
        read_pos_ = 0;
    }

    bool is_closed() const { return is_closed_; }

private:
    bool is_closed_;
    std::string write_buf_;
    std::string read_buf_;
    int read_pos_;
};


class PlmConnectionTest : public testing::Test {
public:
    virtual void SetUp() {
        executor_.reset(new mock_executor);
        event_manager_.reset(new mock_event_manager);
        fd_.reset(new test_fd);
        conn_.reset(new plm_connection(
            fd_.get(), event_manager_.get(), executor_.get()));
        done_ = false;
    }


    void loop_once() {
        done_ = false;
        event_manager_->send_signal();
        executor_->run_until_empty();
    }


    void done_callback(plm_connection::plm_response r) {
        response_ = r;
        done_ = true;
    }


    std::auto_ptr<mock_executor> executor_;
    std::auto_ptr<mock_event_manager> event_manager_;
    std::auto_ptr<test_fd> fd_;
    std::auto_ptr<plm_connection> conn_;

    bool done_;
    plm_connection::plm_response response_;
};


TEST_F(PlmConnectionTest, OpenClose)
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


TEST_F(PlmConnectionTest, SendCommand)
{
    conn_->start();
    EXPECT_TRUE(conn_->is_ok());

    // The modem is expected to send ACK/NACK, so preset the fd read buffer.
    fd_->set_read_buf("\x02\x62\x01\x01\x01\x0f\x12\xff\x06");
    conn_->send_command("\x62\x01\x01\x01\x0f\x12\xff",
        make_callback((PlmConnectionTest *)this,
                      &PlmConnectionTest::done_callback));

    loop_once();

    EXPECT_TRUE(done_);
    EXPECT_EQ(plm_connection::plm_response::ACK, response_.status);
    EXPECT_EQ("\x62\x01\x01\x01\x0f\x12\xff", response_.data);

    // Now simulate a NACK.
    fd_->set_read_buf("\x2\x62\x15");
    conn_->send_command("\x62\x01\x01\x01\x0f\x12\xff",
        make_callback((PlmConnectionTest *)this,
                      &PlmConnectionTest::done_callback));

    loop_once();

    EXPECT_TRUE(done_);
    EXPECT_EQ(plm_connection::plm_response::NACK, response_.status);
}

// TODO
// tests for
// - fd error
// - listener

}

