#include "openalc.hpp"
#include "../string_utils.hpp"

#include <iostream>
#include <sstream>

namespace openalc {

namespace {
int shared_context_count = 0;
std::vector<ALCcontext*> dead_real_contexts;
}

bool wrapper::check_error(ALCdevice *dev, const std::string& op) const {
    wrapper *non_const_this = const_cast<wrapper*>(this);
    if(dev == NULL) {
        non_const_this->err_ = ALC_INVALID_DEVICE;
    } else {
        non_const_this->err_ = alcGetError(dev);
    }
    //std::cout << "ALC check_error = "<<get_error_message(err_)<<"\n";

    if(err_ != ALC_NO_ERROR) {
        std::cout << get_error_message(err_);
        if(!op.empty()) {
            std::cout << " in "<<op;
        }
        std::cout <<"\n";
        if(throws_) {
            throw exception(err_);
        }
    }
    return has_error();
}

std::string device::get_default_device() { 
    const ALCchar *chars = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    /* see discussion below of just how broken
       the SI is on linux - especially exciting in this case is that
       even if something is returned we still set INVALID_DEVICE!
    */
    alcGetError(NULL);
    if(chars) {
        return chars;
    }  
    return "";
}

std::vector<std::string> device::get_available_devices() {
    std::vector<std::string> devices;
    const ALCchar *chars;

    chars = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    /* nb this would be meaningless under the openal spec
       but luckily the linux si bundled with e.g. debian
       not only returns null when asked for a device list
       it also sets an error without there being a device
       to set it on. Thus we call here to clear the 
       non existant error on a nonexistant device.
    */
    alcGetError(NULL);

    if(chars) {
        /* these strings are seperated by nulls,
           with double null to terminate */
        for(const ALCchar *p = chars;*p;++p) {
            std::string dev = (const char*)p;
            devices.push_back(dev);
            p += dev.size()+1; 
        }
    }
    return devices;
}

bool device::has_extension(const std::string& name) const {
    ALCboolean ret = alcIsExtensionPresent(device_, name.c_str());
    check_error(device_, "alcIsExtensionPresent (device::has_extension)");
    return ret == ALC_TRUE;
}
void *device::get_extension(const std::string& name) const {
    if(!has_extension(name)) {
        return NULL;
    }
    void* ret = alcGetProcAddress(device_, name.c_str());
    check_error(device_, "alcGetProcAddress (device::get_extension)");
    return ret;
}
ALCenum device::get_enum(const std::string& name) const {
    ALCenum ret = alcGetEnumValue(device_, name.c_str());
    check_error(device_, "alcGetEnumValue (device::get_enum)");
    return ret;
}
std::string device::get_specifier() const {
    return get_string(ALC_DEVICE_SPECIFIER);
}
std::vector<std::string> device::get_extensions() {
    std::string extension_string = get_string(ALC_EXTENSIONS);
    if(!extension_string.empty()) {
        return util::split(extension_string, ' ');
    } else {
        std::vector<std::string> ret;
        return ret;
    }
}

std::string device::get_string(ALCenum token) const {
    const ALCchar *chars;
    
    chars = alcGetString(device_, token);
    check_error(device_, "alcGetString (device::get_string)");

    if(chars) {
        return chars;
    }
    return "";
}    

context::context(device& dev) : dev_(dev), shared_(false) {
    context_ = alcCreateContext(dev_.device_, NULL);
    {
        bool threw = will_throw();
        set_throws(false);
        check_error(dev_.device_, "alcCreateContext (context::context)");
        set_throws(threw);
    }
    /* some openal implementations only support 1 context 
       so we have to subvert our own system briefly
    */
    if(get_error_code() == ALC_INVALID_VALUE) {
        context_ = alcGetCurrentContext();
        shared_ = true;
        ++shared_context_count;
    } else if(has_error() && will_throw()) {
        throw exception(get_error_code());
    }
}

context::~context() {
    set_throws(false);
    if(isShared()) {
        --shared_context_count;
    } else {
        dead_real_contexts.push_back(context_);
    }

    if(shared_context_count == 0) {
        for(std::vector<ALCcontext*>::iterator itor = dead_real_contexts.begin();
            itor != dead_real_contexts.end(); ++itor) {

            ALCcontext *cur;
            cur = alcGetCurrentContext();
            check_error(dev_.device_, "alcGetCurrentContext (context::~context)");

            if(cur == *itor) {
                alcMakeContextCurrent(NULL);
                check_error(dev_.device_, "alcMakeContextCurrent (context::~context)");
            }
            alcDestroyContext(*itor);
            check_error(dev_.device_, "alcDestroyContext (context::~context)");
        }
        dead_real_contexts.clear();
    }
}
void context::process() {
    alcProcessContext(context_);
    check_error(dev_.device_, "alcProcessContext (context::process)");
}
void context::suspend() {
    alcSuspendContext(context_);
    check_error(dev_.device_, "alcSuspendContext (context::suspend)");
}
bool context::isCurrent() {
    bool ret = context_ == alcGetCurrentContext();
    check_error(dev_.device_, "alcGetCurrentContext (context::isCurrent)");
    return ret;
}
void context::setAsCurrent() {
    alcMakeContextCurrent(context_);
    check_error(dev_.device_, "alcMakeContextCurrent (context::setAsCurrent)");
}

bool context::isShared() {
    return shared_;
}

const std::string get_error_message(const ALCenum& err) {
    std::string prefix = "OpenALC error: ";
    std::string detail;
    switch(err) {
    case ALC_NO_ERROR:
        detail = "no error";
        break;
    case ALC_INVALID_DEVICE:
        detail = "invalid device";
        break;
    case ALC_INVALID_CONTEXT:
        detail = "invalid context";
        break;
    case ALC_INVALID_ENUM:
        detail = "invalid enum";
        break;
    case ALC_INVALID_VALUE:
        detail = "invalid value";
        break;
    case ALC_OUT_OF_MEMORY:
        detail = "out of memory";
        break;
    default:
        detail = "unknown error";
        break;
    }
    return prefix + detail;
}

std::string get_version() {
    std::stringstream s;

    ALCint vers[2];
    alcGetIntegerv(NULL, ALC_MAJOR_VERSION, 1, vers);
    alcGetIntegerv(NULL, ALC_MINOR_VERSION, 1, vers+1);
    
    s << vers[0] << "." <<vers[1];
    return s.str();
}

}
