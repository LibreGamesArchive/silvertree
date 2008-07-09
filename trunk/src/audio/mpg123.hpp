#ifndef MPG123_HPP_INCLUDED
#define MPG123_HPP_INCLUDED

#include <mpg123.h>

#include "openal.hpp"

namespace mpg123 {

const std::string get_error_message(int error);

class exception {
public:
    exception(int err) : err_(err) {}
    std::string get_error() const {
        return get_error_message(err_);
    }
    int get_error_code() const {
        return err_;
    }
private:
    int err_;
};

class stream: public openal::stream {
public:
    stream(int buffers=4, bool throws = true);
    stream(const std::string& decoder, int buffers=4, bool throws=true);
    ~stream();
    std::string get_mpg_error() const;
    bool has_mpg_error() const;
    void open(const std::string& filename);
    void close();
protected:
    bool fill_buffer(ALuint buf);
    bool can_loop() { return true; }
private:
    void reopen();
    void init();
    void stream_call(int error_return, const std::string& op);
    bool opened_;
    mpg123_handle* mh_;
    int err_;
    bool format_change_;
    ALint al_encoding_;
    int channels_;
    long rate_;
    int mpg_encoding_;
    boost::shared_array<unsigned char> raw_buffer_;
    size_t buffer_size_;
    std::string filename_;
};

}

#endif
