
#ifndef PLM_CONNECTIION_H_
#define PLM_CONNECTIION_H_

#include <exception>
#include <set>
#include <string>
#include <vector>

#include "buffered-connection.h"
#include "select-server.h"


class callback;


namespace plm {


// TODO
struct plm_command_listener {
    virtual ~plm_command_listener() {}

    // This function is called when the PLM receives a command with the given
    // PLM command number and the command data. The data is in binary form.
    virtual void on_command(char cmd, const std::string &data) = 0;
};


class plm_exception : public std::exception {
public:
    explicit plm_exception(int err);
    virtual const char *what() const throw();
private:
    int err_;
};


// Low level connection to the PLM that is responsible for sending and
// receiving PLM commands without interpreting them in any way.
class plm_connection : public net::buffered_connection {
public:
    plm_connection(const std::string &serial_device, net::select_server *ss);
    ~plm_connection();

    struct plm_response {
        enum status_t {
            ACK,
            NACK,
            ERROR   // unrecoverable error from the connection
        };

        status_t status;
        std::string data;
    };

    virtual int get_fd();

    // The connection registeres itself for read and is ready to receive
    // commands after this call. The connection does not start itself in the
    // constructor to give the chance to the user to register listeners.
    void start();

    // Closes the connection and resets the connection's state. If there was
    // command in fligh it will be failed.
    void stop();

    // TODO
    void send_command(char cmd,
                      const std::string &data,
                      callback1<plm_response> *done);

    // TODO
    void add_listener(plm_command_listener *listener);
    void remove_listener(plm_command_listener *listener);

    // Returns true if the given PLM command is supported by the connection.
    static bool is_known_command(char cmd);

private:
    plm_connection(const plm_connection &);
    plm_connection &operator= (const plm_connection &);

    void on_cmd_write_done();

    void on_stx_receive();
    void on_cmd_num_receive();
    void on_cmd_data_receive();

    // If the received command was originated at a remote device call all the
    // listeners.
    void maybe_notify_listeners();

private:
    // Not owned.
    net::select_server *ss_;

    std::string serial_device_;
    int fd_;

    std::set<plm_command_listener *> listeners_;

    std::string cmd_out_buf_;
    callback1<plm_response> *cmd_send_done_;

    char stx_buf_;
    char cmd_num_buf_;
    std::vector<char> cmd_data_;
};


}

#endif

