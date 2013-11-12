
#include <errno.h>  // for error codes
#include <string.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "buffered-connection.h"
#include "event-manager.h"
#include "executor.h"
#include "io-buffer.h"


namespace net {


fd_exception::fd_exception(int error) : error_(error)
{
}

int fd_exception::error() const {
    return error_;
}

const char *fd_exception::what() const throw() {
    return strerror(error_);
}


// TODO consider making buffer capacity a parameter.
buffered_connection::buffered_connection(
        fd_interface *fd, event_manager *em, executor *ex)
    : not_ok_(false),
      errno_(0),
      is_closed_(true),
      fd_(fd),
      event_manager_(em),
      executor_(ex),
      read_buffer_(256)
{
}


buffered_connection::~buffered_connection()
{
    try {
        stop();
    } catch(...) {
    }
}


void buffered_connection::start()
{
    try {
        fd_->open();
    } catch(fd_exception &ex) {
        set_error(ex.error());
        return;
    }

    event_manager_->register_for_read(this);
    not_ok_ = false;
    is_closed_ = false;
}


void buffered_connection::stop()
{
    is_closed_ = true;
    event_manager_->deregister_connection(this);

    try {
        fd_->close();
    } catch(fd_exception &ex) {
        // ignore
    }

    clear_queues(true);
    not_ok_ = false;
}


void buffered_connection::read(char *buf, int len, callback *done)
{
    if(not_ok_) {
        executor_->run_later(done);
        return;
    }

    int bytes_read = 0;

    if(!read_buffer_.empty()) {
        bytes_read = read_buffer_.read(buf, len);

        if(bytes_read == 0) {
            executor_->run_later(done);
            stop();
            return;
        }

        if(bytes_read == len) {
            executor_->run_later(done);
            return;
        }
    }

    io_op *op = new io_op(buf + bytes_read, len - bytes_read, done);
    read_ops_.push(op);
}


void buffered_connection::write(const char *buf, int len, callback *done)
{
    if(not_ok_) {
        executor_->run_later(done);
    }

    try {
        int r = fd_->write(buf, len);

        if(r == -1) {
            // EAGAIN
            r = 0;
        }

        if(r == len) {
            executor_->run_later(done);
            return;
        }

        io_op *op = new io_op((char *)buf + r, len - r, done);
        write_ops_.push(op);
        event_manager_->register_for_write(this);
    } catch(fd_exception &ex) {
        set_error(ex.error());
        executor_->run_later(done);
    }
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


int buffered_connection::get_fd()
{
    return fd_->get_fd();
}


void buffered_connection::on_read()
{
    while(true) {
        std::pair<char *, int> write_buf = read_buffer_.get_raw_write_buffer();

        try {
            int r = fd_->read(write_buf.first, write_buf.second);

            if(r == -1) {
                break;  // EAGAIN
            }

            if(r == 0) {
                read_buffer_.write_eof();
                break;
            }

            read_buffer_.advance_write_pointer(r);
        } catch(fd_exception &ex) {
            set_error(ex.error());
            return;
        }
    }

    while(!read_ops_.empty() && !read_buffer_.empty()) {
        io_op *op = read_ops_.front();

        int bytes_read = read_buffer_.read(op->buf, op->len);

        if(bytes_read == 0) {
            // EOF, fd was closed.
            stop();
            return;
        }

        if(bytes_read == op->len) {
            executor_->run_later(op->done);
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

        try {
            int r = fd_->write(op->buf, op->len);

            if(r == -1) {
                return;  // EAGAIN
            }

            if(r == op->len) {
                executor_->run_later(op->done);
                delete op;
                write_ops_.pop();
            } else {
                op->buf += r;
                op->len -= r;
            }
        } catch(fd_exception &ex) {
            set_error(ex.error());
            return;
        }
    }

    if(write_ops_.empty()) {
        event_manager_->deregister_for_write(this);
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

    for(size_t i = 0; i < ops.size(); ++i) {
        io_op *op = ops[i];

        if(call_callbacks) {
            executor_->run_later(op->done);
        } else {
            delete op->done;
        }

        delete op;
    }
}


void buffered_connection::set_error(int error)
{
    not_ok_ = true;
    errno_ = error;

    event_manager_->deregister_connection(this);
    clear_queues(true);
}

}

