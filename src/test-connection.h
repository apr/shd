
#ifndef TEST_CONNECTION_H_
#define TEST_CONNECTION_H_

#include <exception>
#include <list>
#include <string>

#include "select-server.h"
#include "buffered-connection.h"


class test_error : std::exception {
public:
    explicit test_error(int err_no);
    test_error(const std::string &msg, int err_no);
    virtual ~test_error() throw() {}

    virtual const char *what() const throw() {
        return msg_.c_str();
    }

private:
    std::string msg_;
};


class test_connection : public net::connection {
public:
    explicit test_connection(net::select_server *ss);
    ~test_connection();

    virtual int get_fd();

    virtual void on_read();
    virtual void on_write();
    virtual void on_error();

private:
    net::select_server *ss_;  // not owned
    int fd_;
};


class test_echo_connection : public net::connection {
public:
    test_echo_connection(int fd, net::select_server *ss);
    ~test_echo_connection();

    virtual int get_fd();

    virtual void on_read();
    virtual void on_write();
    virtual void on_error();

private:
    int fd_;
    net::select_server *ss_;

    struct block {
        block() : pos_(0), size_(0) {}

        char buf_[1024];
        int pos_;
        int size_;
    };

    std::list<block *> blocks_;
};


class test_echo_connection2 : public net::buffered_connection {
public:
    test_echo_connection2(int fd, net::select_server *ss);

    virtual int get_fd();

private:
    void read_done();
    void write_done();

private:
    int fd_;
    net::select_server *ss_;
    char buf_[10];
};

#endif

