
#ifndef MOCK_PLM_FD_H_
#define MOCK_PLM_FD_H_


#include <algorithm>
#include <string>

#include <string.h>

#include "buffered-connection.h"


namespace plm {


class mock_plm_fd : public net::fd_interface {
public:
    mock_plm_fd() : is_closed_(true), read_pos_(0) {}

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


}

#endif

