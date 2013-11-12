
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "plm-connection.h"
#include "plm-util.h"
#include "select-server.h"


class cmd_executor : public plm::plm_command_listener {
public:
    cmd_executor(const std::string &cmd, net::select_server *ss)
        : cmd_(cmd), ss_(ss),
          fd_("/dev/ttyUSB0"),
          conn_(&fd_, ss, ss),
          resend_alarm_(0), ack_timeout_(0)
    {
        conn_.add_listener(this);
        conn_.start();

        if(!conn_.is_ok()) {
            printf("Cannot open device, error: %s\n",
                strerror(conn_.error()));
            exit(1);
        }

        on_resend_timeout();
    }

    virtual void on_command(const std::string &data)
    {
        if(data[0] != 0x50) {
            return;
        }

        // cmd_[5] has the insteon command (0x12 or 0x13; on or off).
        if(data[8] != cmd_[5]) {
            // Response to a wrong command.
            return;
        }

        ack_timeout_->stop();
        ack_timeout_ = 0;

        char flags = data[7];

        if((flags & 0xf0) == 0x20) {
            // ACK
            exit(0);
        }

        printf("NACK from the device\n");
        on_resend_timeout();
    }

private:
    void on_response(plm::plm_connection::plm_response r)
    {
        if(r.status != plm::plm_connection::plm_response::ACK) {
            printf("Command response: %d, resending\n", r.status);
            resend_alarm_ = ss_->schedule_alarm(
                make_callback(this, &cmd_executor::on_resend_timeout),
                1000);
            return;
        }

        ack_timeout_ = ss_->schedule_alarm(
            make_callback(this, &cmd_executor::on_ack_timeout),
            5000);
    }

    void on_resend_timeout()
    {
        resend_alarm_ = 0;
        conn_.send_command(cmd_,
            make_callback(this, &cmd_executor::on_response));
    }

    void on_ack_timeout()
    {
        ack_timeout_ = 0;
        on_resend_timeout();
    }

private:
    std::string cmd_;
    net::select_server *ss_;
    plm::plm_fd fd_;
    plm::plm_connection conn_;

    net::alarm *resend_alarm_;
    net::alarm *ack_timeout_;
};


int main(int argc, char *argv[])
{
    if(argc < 2 ||
       (std::string("on") != argv[1] &&
        std::string("off") != argv[1]))
    {
        printf("Usage: on-off <on|off>\n");
        return 1;
    }

    std::string cmd(argv[1]);
    std::string addr(plm::hex_to_bin("226A8F"));

    std::string full_cmd = char(0x62) + addr +
        (cmd == "on" ? plm::hex_to_bin("0F12FF") : plm::hex_to_bin("0F1300"));

    net::select_server ss;
    cmd_executor executor(full_cmd, &ss);

    ss.loop();
}

