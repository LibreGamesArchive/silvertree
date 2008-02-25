#include "openal.hpp"
#include "../string_utils.hpp"
#include <iostream>

namespace openal {
bool wrapper::check_error(const std::string& op) {
    err_ = alGetError();
    if(err_ != AL_NO_ERROR) {
        std::cout << get_error_message(err_);
        if(!op.empty()) {
            std::cout <<" in "<<op;
        } 
        std::cout <<"\n";
        if(throws_) {
            throw exception(err_);
        }
    }
    return has_error();
}

source::source(bool throws) : wrapper(throws), init_(false), sound_(NULL) {
    alGenSources(1, &name_);
    init_ = true;
    check_error("alGenSources (source::source)");
}

source::~source() {
    set_throws(false);
    if(init_) {
        if(sound_) {
            bool threw = sound_->will_throw();
            sound_->set_throws(false);
            sound_->deapply(*this);
            sound_->set_throws(threw);
        }
        alDeleteSources(1, &name_);
        check_error("alDeleteSources (source::~source)");
    }
}

void source::set_sound(sound *sound) {
    if(sound_) {
        sound_->deapply(*this);
    }
    sound_ = sound;
    if(sound_) {
        sound_->apply(*this);
    }
}
void sample::apply(source& source) {
    ALuint sname = source.get_name();
    if(!filled_) {
        if(!fill_buffer(name_)) {
            return;
        }
        filled_ = true;
    }
    alSourceQueueBuffers(sname, 1, &name_);
    check_error("alSourceQueueBuffers (sample::apply)");
}
void sample::deapply(source& source) {
    ALuint sname = source.get_name();
    ALuint bname;
    alGetSourcei(sname, AL_BUFFER, (ALint*)&bname);
    check_error("alGetSourcei(AL_BUFFER) (sample::deapply)");
    if(bname == name_) {
        source.stop();
        alSourcei(sname, AL_BUFFER, AL_NONE);
        check_error("alSourcei(AL_BUFFER) (sample::deapply)");
    }
}

stream::stream(int buffers, bool throws) 
    : sound(throws), 
      buffers_(NULL),
      num_buffers_(buffers),
      buffer_playing_(0),
      state_(START), 
      looping_(false)
{
    buffers_.reset(new ALuint[num_buffers_]);
    buffer_live_.reset(new bool[num_buffers_]);
    alGenBuffers(num_buffers_, buffers_.get());
    state_ = INIT;
    check_error("alGenBuffers (stream::stream)");
    for(int i=0;i<num_buffers_;++i) {
        buffer_live_[i] = false;
    }
}

stream::~stream() {
    set_throws(false);
    while(!users_.empty()) {
        source* user = users_.back();
        bool threw = user->will_throw();
        user->set_throws(false);
        user->set_sound(NULL);
        user->set_throws(threw);
    }
    if(state_ > START) {
        alDeleteBuffers(num_buffers_, buffers_.get());
        check_error("alDeleteBuffers (stream::~stream)");
    }
}

void stream::apply(source& source) {
    if(state_ < FILLED) {
        fill();
    }
    
    ALuint sname = source.get_name();
    
    ALint buffer = buffers_[0];
    ALint offset = 0;
    if(!users_.empty()) {
        alGetSourcei(users_.front()->get_name(), AL_BUFFER, &buffer);
        check_error("alGetSourcei(AL_BUFFER) (stream::apply)");
        alGetSourcei(users_.front()->get_name(), AL_SAMPLE_OFFSET, &offset);
        check_error("alGetSourcei(AL_SAMPLE_OFFSET) (stream::apply)");
    } else {
        buffer_playing_ = 0;
    }

    for(int i=0;i<num_buffers_;++i) {
        int buffer = (buffer_playing_ + i ) % num_buffers_;
        if(buffer_live_[buffer]) {
            alSourceQueueBuffers(sname, 1, &(buffers_[buffer]));
            check_error("alSourceQueueBuffers (stream::apply)");
        }
    }
    
    users_.push_back(&source);
    
    if(offset > 0) {
        alSourcei(sname, AL_SAMPLE_OFFSET, offset);
        check_error("alSourcei(AL_SAMPLE_OFFSET) (stream::apply)");
    }
}

void stream::fill() {
    if(state_ != INIT) {
        return;
    }
    state_ = FILLING;
    for(int i=0;i<num_buffers_;++i) {
        int buffer = (i + buffer_playing_) % num_buffers_;
        if(!buffer_live_[buffer]) {
            refresh_buffer(buffer);
        }
    }
    state_ = FILLED;
}

void stream::refresh_buffer(int buffer) {
    if(state_ != FILLING) {
        for(std::vector<source*>::iterator itor = users_.begin();
            itor != users_.end(); ++itor) {
            alSourceUnqueueBuffers((*itor)->get_name(), 1, &(buffers_[buffer]));
            check_error("alSourceUnqueueBuffers (stream::refresh_buffer)");
        }
    }

    if(state_ == ENDING) { 
        /* we're coming to the end of the stream 
           check this one off the list of buffers to go
           and return
        */
        buffer_live_[buffer] = false;
    } else if(!fill_buffer(buffers_[buffer])) {
        /* fillBuffer failed permanently if it returned false
           this stream is now dead 
           once we've started this process by marking ourselves
           dead, all other requests to this function will just 
           mark themselves dead and return 
        */
        state_ = ENDING;
        buffer_live_[buffer] = false;
    }

    if(state_ != ENDING) {
        for(std::vector<source*>::iterator itor = users_.begin();
            itor != users_.end(); ++itor) {
            alSourceQueueBuffers((*itor)->get_name(), 1, &(buffers_[buffer]));
            check_error("alSourceQueueBuffers (stream::refresh_buffer)");
        }
        buffer_live_[buffer] = true;
    }
}

void stream::update() {
    if(users_.empty() || state_== ENDED) {
        return;
    }
    
    ALint processed;
    alGetSourcei(users_.front()->get_name(), AL_BUFFERS_PROCESSED, &processed);
    check_error("alGetSourcei(AL_BUFFERS_PROCESSED) (stream::update)");
    bool restart = processed == num_buffers_;
    if(processed > 0) {
        for(;processed > 0; --processed) {
            refresh_buffer(buffer_playing_);
            buffer_playing_ = (buffer_playing_ + 1) % num_buffers_;
        }
    }
    if(state_ == ENDING) {
        bool any_live = false;
        for(int i=0;i<num_buffers_;++i) {
            if(buffer_live_[i]) {
                any_live = true;
                break;
            }
        }
        if(!any_live) {
            state_ = ENDED;
        }
    }

    if(restart && state_ != ENDED) {
        for(std::vector<source*>::iterator itor = users_.begin();
            itor != users_.end(); ++itor) {
            (*itor)->play();
        }
    }
}

void stream::deapply(source& src) {
    std::vector<source*>::iterator itor = std::find(users_.begin(), users_.end(), &src);
    if(itor != users_.end()) {
        users_.erase(itor);
        src.stop();
        for(int i=0;i<num_buffers_;++i) {
            if(buffer_live_[i]) {
                alSourceUnqueueBuffers(src.get_name(), 1, &(buffers_[i]));
                check_error("alSourceUnqueueBuffers (stream::deapply)");
            }
        }
    }
}   

bool stream::looping() {
    return looping_;
}

void stream::set_looping(bool looping) {
    if(!can_loop()) {
        return;
    }
    if(looping_ == looping) {
        return;
    }
    looping_ = looping;
    if(looping_ && !users_.empty() && state_ != FILLED) {
        state_ = INIT;
        fill();
    }
}

std::string get_error_message(const ALenum& err) {
    std::string prefix = "OpenAL error: ";
    std::string detail;
    switch(err) {
    case AL_NO_ERROR:
        detail = "no error";
        break;
    case AL_INVALID_NAME:
        detail = "invalid name";
        break;
    case AL_INVALID_ENUM:
        detail = "invalid enum";
        break;
    case AL_INVALID_VALUE:
        detail = "invalid value";
        break;
    case AL_INVALID_OPERATION:
        detail = "invalid operation";
        break;
    case AL_OUT_OF_MEMORY:
        detail = "out of memory";
        break;
    default:
        detail = "unknown error";
        break;
    }
    return prefix + detail;
}

std::string get_string(ALenum tok) {
    const ALchar *chars = alGetString(tok);
    ALenum err = alGetError();
    if(err != AL_NO_ERROR) {
        std::cerr << get_error_message(err) <<"\n";
        return "";
    } else if(!chars) {
        std::cerr << "Warning: alGetString string returned a null, but no error.\n" 
                  << "Your implementation may be broken.\n";
        return "";
    }
    return chars;
}

std::string get_version() {
    return get_string(AL_VERSION);
}
   
std::string get_renderer() {
    return get_string(AL_RENDERER);
}

std::string get_vendor() {
    return get_string(AL_VENDOR);
}
std::vector<std::string> get_extensions() {
    return util::split(get_string(AL_EXTENSIONS), ' ');
}

}
