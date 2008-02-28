#include "audio.hpp"

#include <iostream>
#include "../foreach.hpp"

#include "openal.hpp"
#include "openalc.hpp"
#include "mpg123.hpp"

namespace audio {

std::vector<audio_context*> context_stack;

typedef boost::shared_ptr<mpg123::stream> mpg123_stream_ptr;

class audio_wrapper {
public:
    audio_wrapper() {}
    ~audio_wrapper() {}
    audio_wrapper(const std::string& device) : dev_(device) {}
    openalc::device& get_device() { return dev_; }
private:
    openalc::device dev_;
};

/* yet another global to init/deinit device */
boost::shared_ptr<audio_wrapper> wrapper;

bool audio_available() {
#ifdef AUDIO
    return wrapper;
#else
    return false;
#endif
}

/* module functions */
bool init_audio(const std::string& device) {
    try {
        if(device.empty()) {
            wrapper.reset(new audio_wrapper());
        } else {
            wrapper.reset(new audio_wrapper(device));
        }
        return true;
    } catch(openalc::exception e) {
        std::cerr << "Warning: OpenAL could not be initialised on your system.\n" 
                  << "Check your soundcard is configured correctly.\n";
        return false;
    }
}

/* audio context members */
audio_context::audio_context() : frozen_(false) {
    /* this looks pathological but we just can't avoid making
       an ac when we have no device sometimes - luckily, we can
       avoid ever using it */
    if(!audio_available()) return;

    context_.reset(new openalc::context(wrapper->get_device()));
    if(!context_stack.empty()) {
        context_stack.back()->freeze();
    }
    context_stack.push_back(this);
    context_->setAsCurrent();
}

audio_context::~audio_context() {
    if(!audio_available()) return;
        
    context_stack.pop_back();
    if(!context_stack.empty()) {
        context_stack.back()->resume();
    }
}

stream_ptr audio_context::make_stream(const std::string& filename) {
    std::string ext = filename.substr(filename.size()-4);
    if(ext != ".mp3") {
        stream_ptr ret;
        std::cerr << "unsupported file type "<<ext<<"\n";
        return ret;
    }
    
    mpg123_stream_ptr ret;
    ret.reset(new mpg123::stream());
    ret->open(filename);
    streams_.push_back(ret);
    return ret;
}

source_ptr audio_context::make_source() {
    source_ptr source(new openal::source());
    clean_up();
    sources_.push_back(source);
    return source;
}

void audio_context::freeze() {
    if(frozen_) {
        return;
    }
    frozen_ = true;
    clean_up();
    for(std::vector<source_weak_ptr>::iterator itor = sources_.begin();
        itor != sources_.end(); ++itor) {
        source_ptr locked_source = itor->lock();
        if(locked_source) {
            locked_source->pause();
        }
    }
}

void audio_context::resume() {
    if(!frozen_) {
        return;
    }
    frozen_ = false;
    context_->setAsCurrent();

    clean_up();
    for(std::vector<source_weak_ptr>::iterator itor = sources_.begin();
        itor != sources_.end(); ++itor) {
        source_ptr locked_source = itor->lock();
        if(locked_source) {
            locked_source->play();
        }
    }
}

void audio_context::pump_sound() {
    if(frozen_) {
        resume();
    }
    clean_up();
    foreach(stream_weak_ptr stream, streams_) {
        stream_ptr p = stream.lock();
        if(p) {
            p->update();
        }
    }
}

template <class T> void clean_array(std::vector<boost::weak_ptr<T> >& array) {
    typedef std::vector<boost::weak_ptr<T> > Tvec;

    bool changed;
    do {
        changed = false;
        typename Tvec::iterator itor;
        for(itor = array.begin();
            itor != array.end(); ++itor) {
            if(itor->expired()) {
                array.erase(itor);
                changed = true;
                break;
            }
        }
    } while(changed);
}
    
void audio_context::clean_up() {
    clean_array(streams_);
    clean_array(sources_);
}

}
