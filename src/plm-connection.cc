
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


plm_exception::plm_exception(int err)
    : err_(err)
{
}


const char *plm_exception::what() const throw()
{
    return strerror(err_);
}


plm_connection::plm_connection(
        const std::string &serial_device, net::select_server *ss)
    : buffered_connection(ss),
      serial_device_(serial_device),
      ss_(ss),
      fd_(-1),
      cmd_send_done_(0)
{
}


int plm_connection::get_fd()
{
    return fd_;
}


plm_connection::~plm_connection()
{
    stop();
}


void plm_connection::start()
{
    fd_ = open(serial_device_.c_str(), O_RDWR | O_NOCTTY);

    if(fd_ == -1) {
        throw plm_exception(errno);
    }

    struct termios tio;

    if(tcgetattr(fd_, &tio) == -1) {
        throw plm_exception(errno);
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
        throw plm_exception(errno);
    }

    if(tcsetattr(fd_, TCSANOW, &tio) == -1) {
        throw plm_exception(errno);
    }

    if(fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) {
        throw plm_exception(errno);
    }

    ss_->register_for_read(this);
    read(&stx_buf_, 1,
         make_callback(this, &plm_connection::on_stx_receive));
}


void plm_connection::stop()
{
    ss_->deregister_connection(this);
    close(fd_);
    fd_ = -1;

    // TODO send error to the pending send command.
}


void plm_connection::send_command(
    char cmd,
    const std::string &data,
    callback1<plm_response> *done)
{
    // TODO check command, change the data to be just the data
    // TODO only one command at a time, check that

    cmd_out_buf_.clear();
    cmd_out_buf_ += 0x02;
    cmd_out_buf_ += cmd;
    cmd_out_buf_ += data;

    out_cmd_ = cmd;
    cmd_send_done_ = done;

    buffered_connection::write(
        cmd_out_buf_.data(),
        cmd_out_buf_.length(),
        make_callback(this, &plm_connection::on_cmd_write_done));
}


void plm_connection::on_cmd_write_done()
{
    if(!is_ok()) {
        plm_response r;
        r.status = plm_response::ERROR;
        // TODO should be run_later
        cmd_send_done_->run(r);
        cmd_send_done_ = 0;
        return;
    }
}


void plm_connection::on_stx_receive()
{
    // TODO conn error check

    if(stx_buf_ != 0x02) {
        // TODO log error
        read(&stx_buf_, 1,
             make_callback(this, &plm_connection::on_stx_receive));
        return;
    }

    // TODO set timeout
    read(&cmd_num_buf_, 1,
         make_callback(this, &plm_connection::on_cmd_num_receive));
}


void plm_connection::on_cmd_num_receive()
{
    // TODO conn error check

    if(!is_known_command(cmd_num_buf_)) {
        // TODO log unknown command
        // TODO reset the connection, falling back to getting stx might be too
        // error prone because 0x02 can be part of the payload on his unknown
        // command.
        read(&stx_buf_, 1,
             make_callback(this, &plm_connection::on_stx_receive));
        return;
    }

    // Note that the leading 0x02 and the command number have been read
    // already. The length should only include the payload and the ACK/NACK
    // byte.
    int len = 0;
    switch(cmd_num_buf_) {
        case 0x60:
            len = 7;
            break;

        case 0x62:
            len = cmd_out_buf_.size() - 2 + 1;
            break;

        case 0x50:
            len = 9;
            break;

        default:
            // TODO log the error
            read(&stx_buf_, 1,
                 make_callback(this, &plm_connection::on_stx_receive));
            return;
    }
printf("CMD: 0x%x, len: %d\n", cmd_num_buf_, len);

    cmd_data_.resize(len);
    // TODO set timeout
    read(&cmd_data_[0], len,
         make_callback(this, &plm_connection::on_cmd_data_receive));
}


void plm_connection::on_cmd_data_receive()
{
    // TODO conn error check

    int data_len = cmd_data_.size();
    plm_response response;

    response.status = plm_response::ACK;

    // Only 0x6x and 0x7x commands have ACK/NACK byte.
    if((cmd_num_buf_ & 0xf0) == 0x60 ||
       (cmd_num_buf_ & 0xf0) == 0x70)
    {
        --data_len;
        if(cmd_data_[data_len] == 0x15) {
            response.status = plm_response::NACK;
        }
    }

    response.data.assign(cmd_data_.begin(), cmd_data_.begin() + data_len);

    if(cmd_send_done_ != 0 && out_cmd_ == cmd_num_buf_) {
        // TODO send later
        cmd_send_done_->run(response);
        cmd_send_done_ = 0;
    }

    maybe_notify_listeners();

    read(&stx_buf_, 1,
         make_callback(this, &plm_connection::on_stx_receive));
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
    if((cmd_num_buf_ & 0x50) != 0x50) {
        return;
    }

    std::string data(cmd_data_.begin(), cmd_data_.end());

    for(std::set<plm_command_listener *>::iterator it = listeners_.begin();
        it != listeners_.end(); ++it)
    {
        (*it)->on_command(cmd_num_buf_, data);
    }
}


bool plm_connection::is_known_command(char cmd)
{
    return cmd == 0x60 || cmd == 0x62 || cmd == 0x50;
}

}

