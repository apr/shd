
#ifndef PLM_ENDPOINT_H_
#define PLM_ENDPOINT_H_

#include <memory>
#include <queue>
#include <string>

#include "alarm-manager.h"
#include "plm-connection.h"


namespace plm {


// A PLM modem manager. Creates and manages a connection to the physical modem.
// Manages command execution, timeouts, etc.
class plm_endpoint {
public:
    plm_endpoint(net::fd_interface* fd,
                 net::alarm_manager* alarm_manager,
                 net::event_manager *em,
                 net::executor *ex);
    ~plm_endpoint();

    void start();

    // Closes the connection and calls all callbacks for all enqueued commands
    // with an error.
    void stop();


    // Response to a command.
    struct response_t {
        enum status_t {
            OK, ERROR, TIMEOUT
        };

        explicit response_t(status_t s) : status(s) {}

        bool is_ok() const {
            return status == OK;
        }

        bool is_error() const {
            return status == ERROR;
        }

        bool is_timeout() const {
            return status == TIMEOUT;
        }

        // TODO add error information
        status_t status;
    };


    bool is_ok() const { return conn_.is_ok(); }
    bool is_closed() const { return conn_.is_closed(); }


    // Commands.

    void send_light_on(const std::string &device_addr,
                       callback1<response_t> *done);
    void send_light_off(const std::string &device_addr,
                        callback1<response_t> *done);

private:
    plm_endpoint(const plm_endpoint &);
    plm_endpoint &operator= (const plm_endpoint &);

    // TODO limit number of send attempts
    struct command_t {
        enum state_t {
            INIT, SENT, NEED_RESEND, WAIT_DEV, DONE
        };

        command_t(const std::string &cmd, callback1<response_t> *callaback)
            : state(INIT), command(cmd), done(callaback), timeout_alarm(0)
        {}

        inline bool has_alarm() const { return timeout_alarm != 0; }
        inline void stop_alarm() {
            if(has_alarm()) {
                timeout_alarm->stop();
                timeout_alarm = 0;
            }
        }

        state_t state;
        std::string command;
        callback1<response_t> *done;
        net::alarm *timeout_alarm;
    };

    // Called when the modem receives a command from a remote device.
    void on_plm_command(const std::string &data);

    // Called when the modem accepts the command.
    void on_command_sent(plm_connection::plm_response r);

    // Called when communication with the modem times out.
    void on_modem_timeout();

    // Called when we expected a response from the device but it did not come
    // in time.
    void on_device_timeout();

    void send_top_command();
    void reset_connection();

    // Clears the queue and send the given response to all command's callbacks.
    void clear_command_queue(response_t resp);

    inline command_t &top_command() {
        return command_queue_.front();
    }

private:
    plm_connection conn_;

    class plm_listener_proxy;
    std::auto_ptr<plm_listener_proxy> plm_listener_proxy_;

    net::alarm_manager *alarm_manager_;  // not owned

    // TODO the current implementation is simple and allows only one command in
    // flight until it is acknoledged by the device (or until a timeout
    // occurs).  Improve it by allowing concurrent execution of commands.
    std::queue<command_t> command_queue_;
};


}

#endif

