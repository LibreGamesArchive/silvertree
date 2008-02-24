#ifndef OPENALC_HPP_INCLUDED
#define OPENALC_HPP_INCLUDED

#include <AL/alc.h>
#include <string>
#include <vector>

namespace openalc {

const std::string get_error_message(const ALCenum& err);

class exception {
public:
    exception(ALCenum err) : err_(err) {}
    std::string get_error() {
        return get_error_message(err_);
    }
    ALCenum get_error_code() const {
        return err_;
    }
private:
    ALCenum err_;
};

class wrapper {
public:
    wrapper(bool throws=true) : err_(ALC_NO_ERROR), throws_(throws) {}
    std::string get_error() const {
        return get_error_message(err_);
    }
    ALCenum get_error_code() const {
        return err_;
    }
    bool has_error() const {
        return err_ != ALC_NO_ERROR;
    }
    bool will_throw() { return throws_; }
    void set_throws(bool throws) { throws_ = throws; }
protected:
    bool check_error(ALCdevice *device, const std::string& op) const;
private:
    ALCenum err_;
    bool throws_;
};              

class device: public wrapper {
public:
    friend class context;
    static std::vector<std::string> get_available_devices();
    static std::string get_default_device();

    device(const std::string& name) 
        : device_(alcOpenDevice(name.c_str())) {
        check_error(device_, "alcOpenDevice (device::device(string))");
    }
    device() : device_(alcOpenDevice(NULL)) {
        check_error(device_, "alcOpenDevice (device::device)");
    }

    ~device() {
        alcCloseDevice(device_);
    }
    
    bool has_extension(const std::string& name) const;
    void *get_extension(const std::string& name) const;
    ALCenum get_enum(const std::string& name) const;
    std::string get_specifier() const;
    std::vector<std::string> get_extensions();
protected:
    std::string get_string(ALCenum token) const;
private:
    ALCdevice *device_;
};


class context: public wrapper {
public:
    context(device& dev);
    ~context();
    void process();
    void suspend();
    bool isCurrent();
    void setAsCurrent();
    bool isShared();
private:
    ALCcontext *context_;
    device& dev_;
    bool shared_;
};

std::string get_version();

}

#endif
