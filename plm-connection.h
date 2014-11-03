
#ifndef PLM_CONNECTIION_H_
#define PLM_CONNECTIION_H_

#include <exception>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "buffered-connection.h"


namespace plm {


// TODO
struct plm_command_listener {
    virtual ~plm_command_listener() {}

    // This function is called when the PLM receives a command with the given
    // PLM command number and the command data. The data is in binary form.
    virtual void on_command(const std::string &data) = 0;
};


class plm_exception : public std::exception {
public:
    explicit plm_exception(const std::string &msg);
    ~plm_exception() throw();
    virtual const char *what() const throw();
private:
    std::string msg_;
};


// File descriptor interface for the serial device connected to the PLM modem.
// All operations may throw net::fd_exception on errors.
class plm_fd : public net::fd_interface {
public:
    explicit plm_fd(const std::string &serial_device);

    void open() override;
    void close() override;
    int get_fd() override;
    int read(void *buf, int count) override;
    int write(const void *buf, int count) override;

private:
    plm_fd(const plm_fd &);
    plm_fd &operator= (const plm_fd &);

    std::string serial_device_;
    int fd_;
};


// Low level connection to the PLM that is responsible for immediate
// communication with the modem. The connection can send commands and respond
// with either ACK or NACK from the modem (note that this is not an ACK from
// the device which is a separate command in its own right). It can also listen
// for commands sent by the modem.
class plm_connection : public net::buffered_connection {
public:
    plm_connection(net::fd_interface* fd, net::event_manager *em, net::executor *ex);
    ~plm_connection();

    struct plm_response {
        enum status_t {
            ACK,
            NACK,
            ERROR   // unrecoverable error from the connection
        };

        static plm_response ack(const std::string &data);
        static plm_response nack();
        static plm_response error();

        status_t status;
        std::string data;
    };

    // The connection registeres itself for read and is ready to receive
    // commands after this call. The connection does not start itself in the
    // constructor to give the chance to the user to register listeners.
    void start();

    // Closes the connection and resets the connection's state. If there was
    // command in fligh it will be failed.
    void stop();

    // Sends a command to the modem. Please note that the 'cmd' is the modem
    // command, and not just an INSTEON instruction. The operation is async and
    // will call the given callback once a response from the modem is received
    // (either ACK or NACK) or there was an error.
    //
    // Only one command can be sent at a time. Will throw a plm_exception if
    // called during execution of a previous command. If an exception is thrown
    // the callback will be deleted.
    void send_command(const std::string &cmd,
                      const std::function<void(plm_response)> &done);

    // Manage listeners that listen for commands from other devices or from the
    // modem.
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
    void on_cmd_first_byte_receive();
    void on_cmd_data_receive();

    // If the received command was originated at a remote device call all the
    // listeners.
    void maybe_notify_listeners();

    void wait_for_stx();
    void send_response(const plm_response &response);

private:
    // Not owned.
    net::executor *executor_;

    std::set<plm_command_listener *> listeners_;

    std::string cmd_out_buf_;
    std::function<void(plm_response)> cmd_send_done_;
    bool cmd_in_progress_;

    char stx_buf_;
    std::vector<char> cmd_data_;
    int cmd_len_;
};


}

#endif

