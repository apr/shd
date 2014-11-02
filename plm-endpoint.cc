
#include <functional>

#include "alarm-manager.h"
#include "plm-endpoint.h"
#include "plm-util.h"


namespace plm {

// TODO make this a parameter
// TODO in fact there are two different timeouts - a timeout for the response
// from the modem and a timeout for the response from a device
static const int ACK_TIMEOUT = 5000;  // msecs


class plm_endpoint::plm_listener_proxy : public plm_command_listener {
public:
    explicit plm_listener_proxy(plm_endpoint *obj)
        : obj_(obj) {
    }

    virtual void on_command(const std::string &data) {
        obj_->on_plm_command(data);
    }

private:
    plm_endpoint *obj_;
};


plm_endpoint::plm_endpoint(
        net::fd_interface* fd,
        net::alarm_manager* alarm_manager,
        net::event_manager *em,
        net::executor *ex)
    : conn_(fd, em, ex),
      plm_listener_proxy_(new plm_listener_proxy(this)),
      alarm_manager_(alarm_manager)
{
    conn_.add_listener(plm_listener_proxy_.get());
}


plm_endpoint::~plm_endpoint()
{
    conn_.remove_listener(plm_listener_proxy_.get());
}


void plm_endpoint::start()
{
    conn_.start();
}


void plm_endpoint::stop()
{
    conn_.stop();
    clear_command_queue(response_t(response_t::ERROR));
}


void plm_endpoint::send_light_on(
    const std::string &device_addr,
    const std::function<void(response_t)> &done)
{
    std::string cmd = char(0x62) + device_addr + "\x0f\x12\xff";

    command_queue_.push(command_t(cmd, done));

    if(command_queue_.size() == 1) {
        send_top_command();
    }
}


void plm_endpoint::send_light_off(
    const std::string &device_addr,
    const std::function<void(response_t)> &done)
{
    std::string cmd = char(0x62) + device_addr +
        std::string("\x0f\x13\x00", 3);  // Be careful about trailing \nul.

    command_queue_.push(command_t(cmd, done));

    if(command_queue_.size() == 1) {
        send_top_command();
    }
}


void plm_endpoint::on_plm_command(const std::string &data)
{
    if(data[0] != 0x50) {
        return;
    }

    // Check that the response is for the right command.
    if(data[8] != top_command().command[5]) {
        return;
    }

    top_command().stop_alarm();

    char flags = data[7];
    if((flags & 0xf0) == 0x20) {
        // ACK
        if(top_command().state == command_t::WAIT_DEV) {
            top_command().state = command_t::DONE;
            top_command().done(response_t(response_t::OK));
            command_queue_.pop();
        }

        return;
    }

    // Resend the command on NACK.
    send_top_command();
}


void plm_endpoint::on_command_sent(plm_connection::plm_response r)
{
    top_command().stop_alarm();

    if(r.status == plm_connection::plm_response::NACK) {
        // The modem was not ready, resend.
        top_command().state = command_t::NEED_RESEND;
    }

    if(r.status == plm_connection::plm_response::ERROR) {
        top_command().state = command_t::DONE;
        top_command().done(response_t(response_t::ERROR));
        command_queue_.pop();
        return;
    }

    if(top_command().state == command_t::NEED_RESEND) {
        send_top_command();
        return;
    }

    // The modem has acknowledged the command, now wait for the device
    // confirmation.
    top_command().state = command_t::WAIT_DEV;
    top_command().timeout_alarm = alarm_manager_->schedule_alarm(
        std::bind(&plm_endpoint::on_device_timeout, this), ACK_TIMEOUT);
}


void plm_endpoint::on_modem_timeout()
{
    // Modem timeouts are not really expected unless the connection somehow got
    // out of sync or there was some physical break, in either case recourse is
    // limited.
    top_command().timeout_alarm = 0;
    reset_connection();
    send_top_command();
}


void plm_endpoint::on_device_timeout()
{
    top_command().timeout_alarm = 0;
    send_top_command();
}


void plm_endpoint::send_top_command()
{
    if(!conn_.is_ok()) {
        clear_command_queue(response_t(response_t::ERROR));
        return;
    }

    if(command_queue_.empty()) {
        return;
    }

    if(top_command().state == command_t::SENT) {
        // The connection is still busy, wait for the callback, then process
        // the resend.
        top_command().state = command_t::NEED_RESEND;
        return;
    }

    top_command().timeout_alarm = alarm_manager_->schedule_alarm(
        std::bind(&plm_endpoint::on_modem_timeout, this), ACK_TIMEOUT);
    conn_.send_command(top_command().command,
        make_callback(this, &plm_endpoint::on_command_sent));

    top_command().state = command_t::SENT;
}


void plm_endpoint::reset_connection()
{
    if(!conn_.is_closed()) {
        conn_.stop();
        conn_.start();
    }
}


void plm_endpoint::clear_command_queue(response_t resp)
{
    while(!command_queue_.empty()) {
        top_command().stop_alarm();
        top_command().done(resp);
        command_queue_.pop();
    }
}


}

