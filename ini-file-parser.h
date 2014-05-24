
#ifndef INI_PARSER_H_
#define INI_PARSER_H_

#include <exception>
#include <map>
#include <string>
#include <vector>


namespace ini {

class parse_exception : public std::exception {
public:
    explicit parse_exception(const std::string &msg);
    ~parse_exception() throw();

    virtual const char *what() const throw();

private:
    std::string msg_;
};


typedef std::map<std::string, std::string> kv_map_t;


// Parses a simple ini file and returns a map from keys to values. The ini file
// format is very simple and does not include sections. Empty lines are
// skipped, comments should start at the beginning of a line with ';' and run
// to the end of the line. The rest of the lines should have the following
// form:
//
//   key = value
//
// Keys and values will be trimmed on the left and on the right.
//
// If there was any error, parse_exception will be thrown.
void parse_ini_file(const std::string &ini_file, kv_map_t *out);


// Parses a comma-separated list and populates the given vector. Every element
// of the list is trimmed. The parser is very simple and there is not way to
// escape the commas.
void parse_list(const std::string &str, std::vector<std::string> *out);


}


#endif

