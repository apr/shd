
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
    ss_->deregister_connection(this);
    close(fd_);
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


void plm_connection::send_command(
    char cmd,
    const std::string &data,
    callback1<plm_response> *done)
{
    cmd_send_done_ = done;
    cmd_out_buf_ = hex_to_bin(data);
    buffered_connection::write(
        cmd_out_buf_.data(),
        cmd_out_buf_.length(),
        make_callback(this, &plm_connection::on_cmd_write_done, done));
}


void plm_connection::on_cmd_write_done(callback1<plm_response> *done)
{
    if(!is_ok()) {
        plm_response r;
        r.status = plm_response::ERROR;
        // TODO should be run_later
        done->run(r);
        return;
    }
printf("WRITTEN\n");
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

    read(&cmd_num_buf_, 1,
         make_callback(this, &plm_connection::on_cmd_num_receive));
}


void plm_connection::on_cmd_num_receive()
{
    // TODO conn error check
    // TODO what to do on unknown command?
    printf("CMD: 0x%x\n", cmd_num_buf_);

    int len = command_data_size(cmd_num_buf_);

    if(cmd_num_buf_ == 0x60) {
        len = 7;
    }

    if(cmd_num_buf_ == 0x62) {
        len = cmd_out_buf_.size() - 2 + 1;
    }

    if(cmd_num_buf_ == 0x50) {
        len = 9;
    }
printf("C len: %d\n", int(cmd_out_buf_.size()));
    

    if(len == -1) {
        // TODO log error and restart waiting for 0x02
        printf("LEN == -1\n");
        return;
    }

    cmd_data_.resize(len);
    read(&cmd_data_[0], len,
         make_callback(this, &plm_connection::on_cmd_data_receive));
}


void plm_connection::on_cmd_data_receive()
{
    // TODO conn error check

    printf("DATA: ");
    for(int i = 0; i < cmd_data_.size(); ++i) {
        printf("0x%x ", unsigned(cmd_data_[i]));
    }
    printf("\n");

    // TODO
    if(cmd_send_done_ != 0) {
        plm_response r;
        // TODO really parse the data
        r.status = plm_response::ACK;
        cmd_send_done_->run(r);
        cmd_send_done_ = 0;
    }

    // TODO
    read(&stx_buf_, 1,
         make_callback(this, &plm_connection::on_stx_receive));
}

}

