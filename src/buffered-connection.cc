
#include <errno.h>
#include <stdio.h>  // for debug
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "buffered-connection.h"
#include "io-buffer.h"


namespace net {


// TODO make buffer capacity a parameter.
buffered_connection::buffered_connection(select_server *ss)
    : not_ok_(false),
      errno_(0),
      is_closed_(false),
      ss_(ss),
      read_buffer_(256)
{
}


buffered_connection::~buffered_connection()
{
    ss_->deregister_connection(this);
    clear_queues(false);
}


void buffered_connection::read(char *buf, int len, callback *done)
{
    if(not_ok_) {
        ss_->run_later(done);
        return;
    }

    int bytes_read = read_buffer_.read(buf, len);

    if(bytes_read == len) {
        ss_->run_later(done);
        return;
    }

    io_op *op = new io_op(buf + bytes_read, len - bytes_read, done);
    read_ops_.push(op);
}


void buffered_connection::write(const char *buf, int len, callback *done)
{
    if(not_ok_) {
        ss_->run_later(done);
    }

    int r = ::write(get_fd(), buf, len);

    if(r == -1 && errno != EAGAIN) {
        set_error();
        ss_->run_later(done);
        return;
    }

    if(r == -1 && errno == EAGAIN) {
        r = 0;
    }

    io_op *op = new io_op((char *)buf + r, len - r, done);
    write_ops_.push(op);

    ss_->register_for_write(this);
}


bool buffered_connection::is_ok() const
{
    return !not_ok_;
}


bool buffered_connection::is_closed() const
{
    return is_closed_;
}


int buffered_connection::error() const
{
    return errno_;
}


void buffered_connection::on_read()
{
    while(true) {
        std::pair<char *, int> write_buf = read_buffer_.get_raw_write_buffer();

        int r = ::read(get_fd(), write_buf.first, write_buf.second);

        if(r == -1 && errno == EAGAIN) {
            break;
        }

        if(r == -1) {
            set_error();
            return;
        }

        if(r == 0) {
            set_closed();
            return;
        }

        read_buffer_.advance_write_pointer(r);
    }

    while(!read_ops_.empty() && !read_buffer_.empty()) {
        io_op *op = read_ops_.front();

        int bytes_read = read_buffer_.read(op->buf, op->len);

        if(bytes_read == op->len) {
            ss_->run_later(op->done);
            delete op;
            read_ops_.pop();
        } else {
            op->buf += bytes_read;
            op->len -= bytes_read;
        }
    }
}


void buffered_connection::on_write()
{
    while(!write_ops_.empty()) {
        io_op *op = write_ops_.front();

        int r = ::write(get_fd(), op->buf, op->len);

        if(r == -1 && errno == EAGAIN) {
            return;
        }

        if(r == -1) {
            set_error();
            return;
        }

        if(r == op->len) {
            ss_->run_later(op->done);
            delete op;
            write_ops_.pop();
        } else {
            op->buf += r;
            op->len -= r;
        }
    }

    if(write_ops_.empty()) {
        ss_->deregister_for_write(this);
    }
}


void buffered_connection::clear_queues(bool call_callbacks)
{
    std::vector<io_op *> ops;

    while(!read_ops_.empty()) {
        ops.push_back(read_ops_.front());
        read_ops_.pop();
    }

    while(!write_ops_.empty()) {
        ops.push_back(write_ops_.front());
        write_ops_.pop();
    }

    for(int i = 0; i < ops.size(); ++i) {
        io_op *op = ops[i];

        if(call_callbacks) {
            ss_->run_later(op->done);
        } else {
            delete op->done;
        }

        delete op;
    }
}


void buffered_connection::set_error()
{
    not_ok_ = true;
    errno_ = errno;

    ss_->deregister_connection(this);
    clear_queues(true);
}


void buffered_connection::set_closed()
{
    not_ok_ = true;
    is_closed_ = true;

    ss_->deregister_connection(this);
    clear_queues(true);
}

}

