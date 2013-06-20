
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "test-connection.h"


class test_echo_connection;


test_error::test_error(int err_no)
{
    msg_ = strerror(err_no);
}


test_error::test_error(const std::string &msg, int err_no)
    : msg_(msg)
{
    msg_ += ": ";
    msg_ += strerror(err_no);
}


test_connection::test_connection(net::select_server *ss)
    : ss_(ss)
{
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(0, "3210", &hints, &res) == -1) {
        throw test_error("getaddrinfo()", errno);
    }

    fd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(fd_ == -1) {
        throw test_error("socket()", errno);
    }

    int optval = 1;
    if(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                  &optval, sizeof(optval)) == -1)
    {
        throw test_error("setsockopt()", errno);
    }

    if(bind(fd_, res->ai_addr, res->ai_addrlen) == -1) {
        throw test_error("bind()", errno);
    }

    // NOTE: rigt now this mem will leak if there was any error.
    freeaddrinfo(res);

    if(listen(fd_, 5) == -1) {
        throw test_error("listen()", errno);
    }
}


test_connection::~test_connection()
{
    close(fd_);
}


int test_connection::get_fd()
{
    return fd_;
}


void test_connection::on_read()
{
    int new_fd = accept(fd_, 0, 0);
#if 0
    test_echo_connection *conn = new test_echo_connection(new_fd, ss_);
    ss_->register_for_read(conn);
#else
    test_echo_connection2 *conn = new test_echo_connection2(new_fd, ss_);
#endif
}


void test_connection::on_write()
{
}


void test_connection::on_error()
{
}


test_echo_connection::test_echo_connection(int fd, net::select_server *ss)
    : fd_(fd),
      ss_(ss)
{
    if(fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) {
        throw test_error("fcntl()", errno);
    }
}


test_echo_connection::~test_echo_connection()
{
    for(std::list<block *>::iterator it = blocks_.begin();
        it != blocks_.end(); ++it)
    {
        delete *it;
    }

    close(fd_);
printf("closing conn: %d\n", fd_);
}


int test_echo_connection::get_fd()
{
    return fd_;
}


void test_echo_connection::on_read()
{
    block *b = new block;

    b->size_ = read(fd_, b->buf_, 1024);

    if(b->size_ == -1) {
        delete b;
        perror("read()");
        return;
    }

    if(b->size_ == 0) {
        delete b;
        ss_->deregister_connection(this);
        ss_->register_for_delete(this);
        return;
    }

    blocks_.push_back(b);
    ss_->register_for_write(this);
}


void test_echo_connection::on_write()
{
    block *b = blocks_.front();
    int to_write = b->size_ - b->pos_;
    int len = write(fd_, b->buf_ + b->pos_, to_write);

    if(len == -1) {
        perror("write()");
        return;
    }

    if(len < to_write) {
        b->pos_ += len;
    } else {
        delete b;
        blocks_.pop_front();
    }

    if(blocks_.empty()) {
        ss_->deregister_for_write(this);
    }
}


void test_echo_connection::on_error()
{
}


test_echo_connection2::test_echo_connection2(int fd, net::select_server *ss)
    : buffered_connection(ss), fd_(fd), ss_(ss)
{
    if(fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) {
        throw test_error("fcntl()", errno);
    }

    write_done();
}


int test_echo_connection2::get_fd()
{
    return fd_;
}


void test_echo_connection2::read_done()
{
    if(!is_ok()) {
        close(fd_);
        printf("not ok, done\n");
        ss_->register_for_delete(this);
        return;
    }

    printf("ok, read 10 chars: ");
    for(int i = 0; i < 10; ++i) {
        printf("0x%x ", buf_[i]);
    }
    printf("\n");

    write(buf_, 10, make_callback(this, &test_echo_connection2::write_done));
}


void test_echo_connection2::write_done()
{
    read(buf_, 10, make_callback(this, &test_echo_connection2::read_done));
}

