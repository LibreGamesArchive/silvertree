#include "openal.hpp"
#include "openalc.hpp"
#include "../string_utils.hpp"

#include <iostream>
#include <sstream>

namespace openalc {

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
std::vector<std::string> device::get_extensions() const {
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

void device::detect_version() {
    if(openal::get_renderer() == "OpenAL Soft") {
        alc_implementation_ = OPENAL_SOFT;
        return;
    } 
#ifdef linux
    if(openal::get_vendor() == "OpenAL Community") {
        alc_implementation_ = OPENAL_RI_LINUX;
        return;
    }
#endif
#ifdef WIN32
    if(openal::get_vendor() == "Creative Labs Inc. ") {
        alc_implementation_ = OPENAL_CREATIVE_WINDOWS;
        return;
    }
#endif
    alc_implementation_ = OPENAL_OTHER;
}

ALCcontext *device::add_context(const context *c, bool& shared) {
    ALCcontext *context;

    /*
       ri only processes 1 context (you can have more, they just never reach
       the mixing loop)
    */
    if(alc_implementation_ == OPENAL_RI_LINUX) {
        context = alcGetCurrentContext();
        check_error(device_, "alcGetCurrentContext (device::add_context)");
        shared = true;
        ++shared_context_count_ ;
    } else {
        context = alcCreateContext(device_, NULL);
        push_throws(false);
        check_error(device_, "alcCreateContext (device::add_context)");
        pop_throws();
    }
    
    if(alc_implementation_ != OPENAL_RI_LINUX && context && !has_error()) {
        ++total_contexts_;

        /* catch 22 - unless the call succeeds we cant get the version
           -  if the call doesnt succeed, we need the version
           infact the ri will only return meaningful values if a 
           context has been set as current
        */
        
        /* this is really brutal hackery */
        if(alc_implementation_ == OPENAL_UNKNOWN) {
            ALCcontext *old = alcGetCurrentContext();
            check_error(device_, "alcGetCurrentContext (device::add_context) [2]");
            alcMakeContextCurrent(context);
            check_error(device_, "alcMakeContextCurrent (device::add_context)");
            detect_version();
            /* watch out for the fact that restoring the old null context after
               having set a non-null context will bomb the ri */
            if(alc_implementation_ != OPENAL_RI_LINUX) {
                alcMakeContextCurrent(old);
                check_error(device_, "alcMakeContextCurrent (device::add_context) [2]");
            }
        }
    }

    /* generally, an openal implementation may support as few contexts as it
       wishes (see programmer's guide). INVALID_VALUE means "no more contexts for you"
     */
    if(get_error_code() == ALC_INVALID_VALUE) {
        context = alcGetCurrentContext();
        shared = true;
        ++shared_context_count_ ;
    } else if(has_error()) {
        /* prefer to die here so we can accumulate a list of AL versions with
           context issues and manually check for them, reducing the risk we just
           ignore valid and important errors from well written implementations */
        std::cerr << "Your version of OpenAL maxed out at "<<(total_contexts_)<<" contexts.\n"
                  << "Please report the following details to the Silvertree maintainers:\n"
                  << "Vendor: "<<openal::get_vendor()<<"\n"
                  << "Renderer: "<<openal::get_renderer()<<"\n"
                  << "Device: "<<get_specifier()<<"\n"
                  << "AL Version: "<<openal::get_version()<<"\n"
                  << "ALC Version: "<<get_version()<<"\n"
                  << "Reported error: "<<get_error()<<"\n";
        
        if(will_throw()) {
            throw exception(get_error_code());
        }
    } 

    return context;
}

void device::cleanup_contexts() {
    if(alc_implementation_ != OPENAL_RI_LINUX) {
        return;
    }
    /* we only have one context and we couldn't risk destroying it
       before... we can afford to do it now in the destructor */
    ALCcontext *context = alcGetCurrentContext();
    check_error(device_, "alcGetCurrentContext (device::cleanup_contexts)");
    if(context) {
        alcMakeContextCurrent(NULL);
        check_error(device_, "alcMakeContextCurrent (device::cleanup_contexts)");
        alcDestroyContext(context);
        check_error(device_, "alcDestroyContext (device::cleanup_contexts)");
    }
}

void device::destroy_context(context *c) {
    if(c->isShared()) {
        --shared_context_count_;
    } else {
        dead_real_contexts_.push_back(c->context_);
    }

    if(shared_context_count_ == 0 && alc_implementation_ != OPENAL_RI_LINUX) {
        bool changed;
        do {
            changed = false;
            for(std::vector<ALCcontext*>::iterator itor = dead_real_contexts_.begin();
                itor != dead_real_contexts_.end(); ++itor) {
                
                ALCcontext *cur;
                cur = alcGetCurrentContext();
                check_error(device_, "alcGetCurrentContext (device::destroy_context)");
                
                if(cur == *itor) {
                    alcMakeContextCurrent(NULL);
                    check_error(device_, "alcMakeContextCurrent (device::destroy_context)");
                }
                alcDestroyContext(*itor);
                check_error(device_, "alcDestroyContext (device::destroy_context)");
                --total_contexts_;
                dead_real_contexts_.erase(itor);
                changed = true;
                break;
            }
        } while(changed);
    }
}

context::context(device& dev) : dev_(dev), shared_(false) {
    context_ = dev_.add_context(this, shared_);
}

context::~context() {
    set_throws(false);
    {
        dev_.push_throws(false);
        dev_.destroy_context(this);
        dev_.pop_throws();
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
    if(dev_.alc_implementation_ != device::OPENAL_RI_LINUX) {
        alcMakeContextCurrent(context_);
        check_error(dev_.device_, "alcMakeContextCurrent (context::setAsCurrent)");
    }
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
