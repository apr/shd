
#ifndef BUFFERED_CONNECTION_H_
#define BUFFERED_CONNECTION_H_

#include <sys/types.h>

#include <exception>
#include <queue>
#include <utility>

#include "event-manager.h"
#include "callback.h"
#include "io-buffer.h"


namespace net {

class executor;
class io_buffer;


// Exception used by the fd_interface methods to signal errors.
class fd_exception : std::exception {
public:
    explicit fd_exception(int error);
    int error() const;
    virtual const char *what() const throw();
private:
    int error_;
};


// File descriptor interface. All methods will throw fd_exception in case of an
// error. Reads and writes must be non-blocking and return -1 if the operation
// would block.
struct fd_interface {
    virtual ~fd_interface() {}

    virtual void open() = 0;
    virtual void close() = 0;

    // Will return a meaningful fd only after successful open().
    virtual int get_fd() = 0;

    virtual int read(void *buf, int count) = 0;
    virtual int write(const void *buf, int count) = 0;
};


// A connection that performs IO operations in async manner. It guarantees that
// completion of the whole operation before signalling.
class buffered_connection : public connection {
public:
    buffered_connection(fd_interface *fd, event_manager *em, executor *ex);
    ~buffered_connection();

    void start();
    void stop();

    // Reads the len amount of bytes into the given buffer and calls the
    // callback. The callback will also be called in case of an error or if the
    // fd was closed.
    void read(char *buf, int len, callback *done);

    // Writes the len amount of bytes from the given buffer and calls the
    // callback. The callback will also be called in case of an error or if the
    // fd was closed.
    void write(const char *buf, int len, callback *done);

    // Returns true if the connection is healthy, returns false on error or if
    // the connection was closed. It is the user's responsibility to check the
    // health of the connection after every operation.
    bool is_ok() const;

    // True if the connection of the file descriptor was closed.
    bool is_closed() const;

    // If not_ok() and not closed will return the last error.
    int error() const;

private:
    buffered_connection(const buffered_connection &);
    buffered_connection &operator= (const buffered_connection &);

    virtual int get_fd() override;
    virtual void on_read() override;
    virtual void on_write() override;

    // Purges all outstanding io ops. If 'call_callbacks' is true the io ops
    // callbacks will be enqueued in the executor for execution, otherwise they
    // will be deleted.
    void clear_queues(bool call_callbacks);

    // Puts this connection into the error state and saves the error.  Will
    // also clear all io queues.
    void set_error(int error);

private:
    // Once the connection enters this state all further operations will cease
    // immediately, i.e. upon submissions callbacks will be run and nothing
    // will be attempted.
    bool not_ok_;

    int errno_;
    bool is_closed_;

    // Not owned.
    fd_interface *fd_;
    event_manager *event_manager_;
    executor *executor_;

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

