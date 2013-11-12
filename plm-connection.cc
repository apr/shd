
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// For debug only.
#include <stdio.h>

#include "plm-connection.h"
#include "plm-util.h"
#include "select-server.h"


namespace plm {


plm_exception::plm_exception(const std::string &msg)
    : msg_(msg)
{
}


plm_exception::~plm_exception() throw()
{
}


const char *plm_exception::what() const throw()
{
    return msg_.c_str();
}


plm_fd::plm_fd(const std::string &serial_device)
    : serial_device_(serial_device), fd_(-1)
{
}


void plm_fd::open()
{
    fd_ = ::open(serial_device_.c_str(), O_RDWR | O_NOCTTY);

    if(fd_ == -1) {
        throw net::fd_exception(errno);
    }

    struct termios tio;

    if(tcgetattr(fd_, &tio) == -1) {
        throw net::fd_exception(errno);
    }

    tio.c_cflag = B19200 |
                  CS8 |
                  CLOCAL |
                  CREAD;

    tio.c_iflag = IGNBRK | IGNPAR;
    tio.c_oflag = ONLRET;
    tio.c_lflag = 0;

    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    if(tcflush(fd_, TCIOFLUSH) == -1) {
        throw net::fd_exception(errno);
    }

    if(tcsetattr(fd_, TCSANOW, &tio) == -1) {
        throw net::fd_exception(errno);
    }

    if(fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) {
        throw net::fd_exception(errno);
    }
}


void plm_fd::close()
{
    ::close(fd_);
}


int plm_fd::get_fd()
{
    return fd_;
}


int plm_fd::read(void *buf, int count)
{
    int ret = ::read(fd_, buf, count);

    if(ret == -1 && errno != EAGAIN) {
        throw net::fd_exception(errno);
    }
    
    return ret;
}


int plm_fd::write(const void *buf, int count)
{
    int ret = ::write(fd_, buf, count);

    if(ret == -1 && errno != EAGAIN) {
        throw net::fd_exception(errno);
    }

    return ret;
}


plm_connection::plm_response plm_connection::plm_response::ack(
    const std::string &data)
{
    plm_response ret;
    ret.status = ACK;
    ret.data = data;
    return ret;
}


plm_connection::plm_response plm_connection::plm_response::nack()
{
    plm_response ret;
    ret.status = NACK;
    return ret;
}


plm_connection::plm_response plm_connection::plm_response::error()
{
    plm_response ret;
    ret.status = ERROR;
    return ret;
}


plm_connection::plm_connection(
        net::fd_interface* fd, net::event_manager *em, net::executor *ex)
    : buffered_connection(fd, em, ex),
      executor_(ex),
      cmd_send_done_(0),
      cmd_len_(0)
{
    // The longest command is 23 bytes, reserve some extra space.
    cmd_data_.resize(60);
}


plm_connection::~plm_connection()
{
    try {
        stop();
    } catch(...) {
    }
}


void plm_connection::start()
{
    buffered_connection::start();

    if(is_ok()) {
        wait_for_stx();
    }
}


void plm_connection::stop()
{
    buffered_connection::stop();
    send_response(plm_response::error());
}


void plm_connection::send_command(
    const std::string &cmd,
    callback1<plm_response> *done)
{
    if(cmd_send_done_ != 0) {
        delete done;
        throw plm_exception("Can send only one command at a time");
    }

    cmd_send_done_ = done;

    if(!is_ok()) {
        send_response(plm_response::error());
        return;
    }

    cmd_out_buf_.clear();
    cmd_out_buf_ += 0x02;
    cmd_out_buf_ += cmd;

    buffered_connection::write(
        cmd_out_buf_.data(),
        cmd_out_buf_.length(),
        make_callback(this, &plm_connection::on_cmd_write_done));
}


void plm_connection::on_cmd_write_done()
{
    if(!is_ok()) {
        send_response(plm_response::error());
    }
}


void plm_connection::on_stx_receive()
{
    if(!is_ok()) {
        send_response(plm_response::error());
        return;
    }

    if(stx_buf_ != 0x02) {
        // TODO log error, also send an error to the caller?
        wait_for_stx();
        return;
    }

    // TODO set timeout? what to do on timeout? leave it all to the caller?
    read(&cmd_data_[0], 1,
         make_callback(this, &plm_connection::on_cmd_num_receive));
}


void plm_connection::on_cmd_num_receive()
{
    if(!is_ok()) {
        send_response(plm_response::error());
        return;
    }

    if(!is_known_command(cmd_data_[0])) {
        // TODO log unknown command
        // TODO reset the connection, falling back to getting stx might be too
        // error prone because 0x02 can be part of the payload of this unknown
        // command.
        wait_for_stx();
        return;
    }

    // Note that the leading 0x02 and the command number have been read
    // already. The length should only include the payload and the ACK/NACK
    // byte.
    cmd_len_ = 0;
    switch(cmd_data_[0]) {
        case 0x60:
            cmd_len_ = 7;
            break;

        case 0x62:
            cmd_len_ = cmd_out_buf_.size() - 2 + 1;
            break;

        case 0x50:
            cmd_len_ = 9;
            break;

        default:
            // TODO log the error
            wait_for_stx();
            return;
    }
printf("CMD: 0x%x, len: %d\n", cmd_data_[0], cmd_len_);

    // TODO set timeout?
    read(&cmd_data_[1], 1,
         make_callback(this, &plm_connection::on_cmd_first_byte_receive));
}


void plm_connection::on_cmd_first_byte_receive()
{
    if(!is_ok()) {
        send_response(plm_response::error());
        return;
    }

    // Did we get a NACK from the PLM for command waiting for the response?
    if(cmd_send_done_ != 0 &&
       cmd_out_buf_[1] == cmd_data_[0] &&
       cmd_data_[1] == 0x15)
    {
        send_response(plm_response::nack());
        wait_for_stx();
        return;
    }

    // Retrieve the rest of the command.
    read(&cmd_data_[2], cmd_len_ - 1,
         make_callback(this, &plm_connection::on_cmd_data_receive));
}


void plm_connection::on_cmd_data_receive()
{
    if(!is_ok()) {
        send_response(plm_response::error());
        return;
    }

    // Account for the PLM command number.
    int data_len = cmd_len_ + 1;
    bool has_ack = false;

    if((cmd_data_[0] & 0xf0) == 0x60 ||
       (cmd_data_[0] & 0xf0) == 0x70)
    {
        // Only 0x6x and 0x7x commands have ACK byte, skip it.
        --data_len;
        has_ack = true;
    }

    std::string data(cmd_data_.begin(), cmd_data_.begin() + data_len);

    // If this is an acknolegment from the modem, respond to the sender.
    // Note that the cmd_out_buf_ stores STX(0x02) as the first character
    // followed by the command whereas the receive buffer immediately starts
    // with the command.
    if(has_ack && cmd_send_done_ != 0 && cmd_out_buf_[1] == cmd_data_[0]) {
        send_response(plm_response::ack(data));
    }

    maybe_notify_listeners();
    wait_for_stx();
}


void plm_connection::add_listener(plm_command_listener *listener)
{
    listeners_.insert(listener);
}


void plm_connection::remove_listener(plm_command_listener *listener)
{
    listeners_.erase(listener);
}


void plm_connection::maybe_notify_listeners()
{
    if((cmd_data_[0] & 0x50) != 0x50) {
        return;
    }

    std::string data(cmd_data_.begin(), cmd_data_.begin() + cmd_len_ + 1);

    for(std::set<plm_command_listener *>::iterator it = listeners_.begin();
        it != listeners_.end(); ++it)
    {
        (*it)->on_command(data);
    }
}


bool plm_connection::is_known_command(char cmd)
{
    return cmd == 0x60 || cmd == 0x62 || cmd == 0x50;
}


void plm_connection::wait_for_stx()
{
    read(&stx_buf_, 1,
         make_callback(this, &plm_connection::on_stx_receive));
}


void plm_connection::send_response(const plm_response &response)
{
    if(!cmd_send_done_) {
        return;
    }

    // TODO this way of sending a response has a risk of leaking the callback
    // if the wrapping callback will be deleted and not executed, fix this.
    executor_->run_later(make_callback(
        this,
        &plm_connection::send_response_helper,
        cmd_send_done_,
        response));

    cmd_send_done_ = 0;
}


void plm_connection::send_response_helper(
    callback1<plm_response> *done, plm_connection::plm_response r)
{
    if(done != 0) {
        done->run(r);
    }
}


}

