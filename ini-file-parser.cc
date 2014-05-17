
#include "ini-file-parser.h"

#include <stdexcept>
#include <utility>


namespace ini {

namespace {

// TODO make trim() handle other space characters like tabs.
inline std::string trim(std::string &str)
{
    str.erase(0, str.find_first_not_of(' '));
    str.erase(str.find_last_not_of(' ') + 1);
    return str;
}


// TODO track line number
class ini_parser {
public:
    explicit ini_parser(const std::string &ini_file);

    void reset();
    void parse(kv_map_t *out);

private:
    ini_parser(const ini_parser &);
    ini_parser &operator= (const ini_parser &);

    bool is_eol() const;
    char cur_char() const;

    void parse_kv_line(kv_map_t *out);

private:
    enum state_t {
        LINE_START, IN_KEY, IN_VALUE
    };

    std::string buf_;
    size_t pos_;
    state_t state_;
};


ini_parser::ini_parser(const std::string &file)
    : buf_(file)
{
    reset();
}


void ini_parser::reset()
{
    pos_ = 0;
    state_ = LINE_START;
}


// TODO make it handle "\r\n"
inline bool ini_parser::is_eol() const
{
    if(pos_ >= buf_.size()) {
        return true;
    }

    return buf_[pos_] == '\r' || buf_[pos_] == '\n';
}


inline char ini_parser::cur_char() const {
    if(pos_ >= buf_.size()) {
        throw std::out_of_range("Indexing beyond the buffer");
    }

    return buf_[pos_];
}


void ini_parser::parse_kv_line(kv_map_t *out)
{
    std::string key;
    std::string value;

    state_ = IN_KEY;

    while(!is_eol()) {
        const char c = cur_char();

        ++pos_;

        if(c == '=' && state_ == IN_KEY) {
            state_ = IN_VALUE;
            continue;
        }

        if(c == ';' && state_ == IN_KEY) {
            throw parse_exception("Comments should start at "
                                  "the beginning of a line");
        }

        if(state_ == IN_KEY) {
            key.append(1, c);
        } else {
            value.append(1, c);
        }
    }

    if(pos_ < buf_.size()) {
        // Skip EOL.
        ++pos_;
    }

    key = trim(key);
    value = trim(value);

    if(key.empty() && !value.empty()) {
        throw parse_exception("A key cannot be empty");
    }

    if(!out->insert(std::make_pair(key, value)).second) {
        throw parse_exception(
            "Duplicate key are not allowed, offending key: " + key);
    }
}


void ini_parser::parse(kv_map_t *out)
{
    reset();

    while(!is_eol()) {
        if(state_ == LINE_START) {
            parse_kv_line(out);
        } else {
            // TODO produce a better error
            throw parse_exception("Unexpected: " + buf_[pos_]);
        }
    }
}

} // namespace


parse_exception::parse_exception(const std::string &msg)
    : msg_(msg)
{
}


parse_exception::~parse_exception() throw()
{
}


const char *parse_exception::what() const throw()
{
    return msg_.c_str();
}


void parse_ini_file(const std::string &ini_file, kv_map_t *out)
{
    ini_parser parser(ini_file);
    parser.parse(out);
}


}

