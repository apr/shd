
#ifndef BUFFERED_CONNECTION_H_
#define BUFFERED_CONNECTION_H_

#include <queue>

#include "callback.h"
#include "io-buffer.h"
#include "select-server.h"


namespace net {

class io_buffer;


class buffered_connection : public connection {
public:
    explicit buffered_connection(select_server *ss);
    ~buffered_connection();

    void read(char *buf, int len, callback *done);

    // TODO doc, the buf should be alive until 'done' is called.
    void write(const char *buf, int len, callback *done);

    // Returns true if the connection is healthy, returns false on error or if
    // the connection was closed.
    bool is_ok() const;

    // TODO consider registering a callback for the close event

    // True if the connection was closed.
    bool is_closed() const;

    // If not_ok() and not closed will return the last error.
    int error() const;

private:
    buffered_connection(const buffered_connection &);
    buffered_connection &operator= (const buffered_connection &);

    virtual void on_read();
    virtual void on_write();
    virtual void on_error() {}

    // Purges all outstanding io ops. If 'call_callbacks' is true the io ops
    // callbacks will be enqueued in the select server for execution, otherwise
    // they will be deleted.
    void clear_queues(bool call_callbacks);

    // Puts this connection into the error state and saves the current errno.
    // Will also clear all io queues.
    void set_error();

    // Puts this connection into the closed state. Will also clear all io queues.
    void set_closed();

private:
    // Once the connection enters this state all further operations will cease
    // immediately, i.e. upon submissions callbacks will be run and nothing
    // will be attempted.
    bool not_ok_;

    int errno_;
    bool is_closed_;

    // Not owned.
    select_server *ss_;

    struct io_op {
        io_op(char *b, int l, callback *c)
            : buf(b), len(l), done(c)
        {
        }

        char *buf;
        int len;
        callback *done;
    };

    io_buffer read_buffer_;
    std::queue<io_op *> read_ops_;
    std::queue<io_op *> write_ops_;
};

}

#endif

