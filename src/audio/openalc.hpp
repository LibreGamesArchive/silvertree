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
    void push_throws(bool throws) { 
        throws_stack_.push_back(throws_); 
        set_throws(throws); 
    }
    void pop_throws() {
        if(!throws_stack_.empty()) {
            set_throws(throws_stack_.back());
            throws_stack_.pop_back(); 
        }
    }
protected:
    bool check_error(ALCdevice *device, const std::string& op) const;
private:
    ALCenum err_;
    bool throws_;
    std::vector<bool> throws_stack_;
};              

/* forward decl */
class context;

class device: public wrapper {
public:
    friend class context;
    enum implementation {
        OPENAL_UNKNOWN,
        OPENAL_RI_LINUX,
        OPENAL_SOFT,
        OPENAL_OTHER,
        OPENAL_CREATIVE_WINDOWS
    };
    static std::vector<std::string> get_available_devices();
    static std::string get_default_device();

    device(const std::string& name="") 
        : device_(alcOpenDevice(name.empty() ? NULL : name.c_str())),
          shared_context_count_(0), alc_implementation_(OPENAL_UNKNOWN),
          total_contexts_(0)
    {
        check_error(device_, "alcOpenDevice (device::device)");
        
    }

    ~device() {
        set_throws(false);
        cleanup_contexts();
        alcCloseDevice(device_);
    }
    
    bool has_extension(const std::string& name) const;
    void *get_extension(const std::string& name) const;
    ALCenum get_enum(const std::string& name) const;
    std::string get_specifier() const;
    std::vector<std::string> get_extensions() const;

    ALCcontext *add_context(const context *c, bool& shared);
    void destroy_context(context *c);
protected:
    std::string get_string(ALCenum token) const;
private:
    void detect_version();
    void cleanup_contexts();
    ALCdevice *device_;

    /* all of this to try to get buggy openal working
       under linux */
    int shared_context_count_;
    std::vector<ALCcontext*> dead_real_contexts_;
    implementation alc_implementation_;    
    int total_contexts_;
};


class context: public wrapper {
public:
    friend class device;
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
