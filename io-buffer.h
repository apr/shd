
#ifndef IO_BUFFER_H_
#define IO_BUFFER_H_

#include <list>
#include <utility>


namespace net {

class io_buffer {
public:
    explicit io_buffer(int capacity);
    ~io_buffer();

    // Reads up to len bytes into the given buffer and advances the read
    // position. Returns the amount actually written into the buffer. It can
    // return 0 which signifies the end of the input stream.
    int read(char *buf, int len);

    // Total number of bytes available for read from this buffer.
    int read_size() const;

    bool empty() const;

    // Returns a pointer to the raw write buffer and the size of the buffer.
    std::pair<char *, int> get_raw_write_buffer();

    // Advances the pointer into the raw write buffer. It is expected that the
    // user has requested a write buffer first, will do nothing on the
    // completely empty buffer.
    void advance_write_pointer(int size);

    // Appends a marker that signifies the end of the input stream.
    void write_eof();

private:
    io_buffer(const io_buffer &);
    io_buffer &operator= (const io_buffer &);

    struct block;

    // Makes sure that there is some write space in the buffer, allocates a new
    // block if needed.
    void check_write_space();

private:
    const int capacity_;
    std::list<block *> blocks_;
};

}

#endif

