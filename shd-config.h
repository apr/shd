
#ifndef SHD_CONFIG_
#define SHD_CONFIG_

#include <exception>
#include <string>
#include <vector>


class shd_config_exception : public std::exception {
public:
    explicit shd_config_exception(const std::string &msg);
    ~shd_config_exception() throw();
    virtual const char *what() const throw();
private:
    std::string msg_;
};


// Reads the SHD daemon configuration file, parses it and provides convinient
// access to the configuration values. The constructors may throw
// shd_config_exception if the file could not be read or parsed.
class shd_config {
public:
    // Reads the file in the standard location: $HOME/.shdrc
    shd_config();
    explicit shd_config(const std::string &file_path);

    std::string serial_device() const;

    double longitude() const;
    double latitude() const;

    // A list of addresses for outside lights. The addresses have already been
    // converted to binary form.
    const std::vector<std::string> &outside_lights() const;

private:
    shd_config(const shd_config &);
    shd_config &operator= (const shd_config &);

    void read_config(const std::string &file_path);

private:
    std::string serial_device_;
    double longitude_;
    double latitude_;
    std::vector<std::string> outside_lights_;
};


#endif

