
#include <string.h>

#include <algorithm>
#include <list>
#include <utility>

#include "io-buffer.h"


namespace net {

class io_buffer::block {
public:
    explicit block(int capacity)
        : capacity_(capacity),
          buf_(new char[capacity]),
          read_offset_(0),
          write_offset_(0)
    {
    }

    ~block() {
        delete[] buf_;
    }

    char *read_pointer() {
        return buf_ + read_offset_;
    }

    char *write_pointer() {
        return buf_ + write_offset_;
    }

    void advance_read_pointer(int size) {
        read_offset_ += size;
    }

    void advance_write_pointer(int size) {
        write_offset_ += size;
    }

    int read_size() const {
        return write_offset_ - read_offset_;
    }

    int write_size() const {
        return capacity_ - write_offset_;
    }

private:
    block(const block &);
    block &operator= (const block &);

private:
    int capacity_;
    char *buf_;
    int read_offset_;
    int write_offset_;
};


io_buffer::io_buffer(int capacity)
    : capacity_(capacity)
{
}


io_buffer::~io_buffer()
{
    std::list<block *>::iterator it = blocks_.begin();
    for(; it != blocks_.end(); ++it) {
        delete *it;
    }
}


int io_buffer::read(char *buf, int len)
{
    int written = 0;

    while(!blocks_.empty() && written != len) {
        block *b = blocks_.front();
        int to_copy = std::min(len - written, b->read_size());

        memcpy(buf + written, b->read_pointer(), to_copy);
        b->advance_read_pointer(to_copy);
        written += to_copy;

        if(b->read_size() == 0) {
            delete b;
            blocks_.pop_front();
        }
    }

    return written;
}


int io_buffer::read_size() const
{
    int ret = 0;

    std::list<block *>::const_iterator it = blocks_.begin();
    for(; it != blocks_.end(); ++it) {
        ret += (*it)->read_size();
    }

    return ret;
}


bool io_buffer::empty() const
{
    std::list<block *>::const_iterator it = blocks_.begin();
    for(; it != blocks_.end(); ++it) {
        if((*it)->read_size() > 0) {
            return false;
        }
    }

    return true;
}


std::pair<char *, int> io_buffer::get_raw_write_buffer()
{
    check_write_space();
    return std::make_pair(blocks_.back()->write_pointer(),
                          blocks_.back()->write_size());
}


void io_buffer::advance_write_pointer(int size)
{
    if(!blocks_.empty()) {
        blocks_.back()->advance_write_pointer(size);
    }
}


void io_buffer::check_write_space()
{
    if(blocks_.empty() || blocks_.back()->write_size() == 0) {
        blocks_.push_back(new block(capacity_));
        return;
    }
}

}

