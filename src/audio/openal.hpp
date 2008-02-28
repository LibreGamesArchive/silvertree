#ifndef OPENAL_HPP_INCLUDED
#define OPENAL_HPP_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef AUDIO

#include <AL/al.h>
#include <string>
#include <vector>
#include <boost/shared_array.hpp>

#include <iostream>

namespace openal {

std::string get_error_message(const ALenum& err);

class exception {
public:
    exception(ALenum err) : err_(err) {}
       
    ALenum get_error_code() const {
        return err_;
    }
    std::string get_error() const {
        return get_error_message(err_);
    }
private:
    ALenum err_;
};

class wrapper {
public:
    wrapper(bool throws) : err_(AL_NO_ERROR), throws_(throws) {}
    std::string get_error() {
        return get_error_message(err_);
    }
    bool has_error() {
        return err_ != AL_NO_ERROR;
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
    bool check_error(const std::string& op);
private:
    ALenum err_;
    bool throws_;
    std::vector<bool> throws_stack_;
};

class source;

class sound: public wrapper {
public:
    sound(bool throws) : wrapper(throws) {}
    virtual ~sound() {}
    virtual void apply(source& source)=0;
    virtual void deapply(source& source)=0;
};

class source: public wrapper {
public:
    source(bool throws=false);
    virtual ~source();
    void set_sound(sound *sound);
    void clear_sound();
    void play() {
        alSourcePlay(name_);
        check_error("alSourcePlay (source::play)");
    }
    void pause() {
        alSourcePause(name_);
        check_error("alSourcePause (source::pause)");
    }
    void stop() {
        alSourceStop(name_);
        check_error("alSourceStop (source::stop)");
    }
    void rewind() {
        alSourceRewind(name_);
        check_error("alSourceRewind (source::rewind)");
    }
    void set_position(float x, float y, float z) {
        set3f(AL_POSITION, x,y,z);
    }
    void set_velocity(float x, float y, float z) {
        set3f(AL_VELOCITY, x,y,z);
    }
    void set_direction(float x, float y, float z) {
        set3f(AL_DIRECTION, x,y,z);
    }
    void set_cone_outer_angle(float angle) {
        setf(AL_CONE_OUTER_ANGLE, angle);
    }
    void set_cone_inner_angle(float angle) {
        setf(AL_CONE_INNER_ANGLE, angle);
    }
    void set_cone_outer_gain(float gain) {
        setf(AL_CONE_OUTER_GAIN, gain);
    }
    void set_gain(float gain) {
        setf(AL_GAIN, gain);
    }
    void set_minimum_gain(float gain) {
        setf(AL_MIN_GAIN, gain);
    }
    void set_maximum_gain(float gain) {
        setf(AL_MAX_GAIN, gain);
    }
    void set_relative(bool relative) {
        setb(AL_SOURCE_RELATIVE, relative);
    }
    void set_looping(bool looping) {
        setb(AL_LOOPING, looping);
    }
    void set_reference_distance(float distance) {
        setf(AL_REFERENCE_DISTANCE, distance);
    }
    void set_rolloff_factor(float factor) {
        setf(AL_ROLLOFF_FACTOR, factor);
    }
    void set_maximum_distance(float distance) {
        setf(AL_MAX_DISTANCE, distance);
    }
    void set_pitch(float pitch) {
        setf(AL_PITCH, pitch);
    }
    ALuint get_name() {
        return name_;
    }
protected:
    void set3f(ALenum key, float x1, float x2, float x3) {
        alSource3f(name_, key, x1, x2, x3);
        check_error("alSource3f (source::set3f)");
    }
    void setf(ALenum key, float val) {
        alSourcef(name_, key, val);
        check_error("alSourcef (source::setf)");
    }
    void seti(ALenum key, int val) {
        alSourcei(name_, key, val);
        check_error("alSourcei (source::seti)");
    }
    void setb(ALenum key, bool val) {
        alSourcei(name_, key, val ? AL_TRUE : AL_FALSE);
        check_error("alSourcei (source::setb)");
    }
    
private:
    ALuint name_;
    bool init_;
    sound* sound_;
};

    /* FIXME: this class has many behaviours it should 
       share with stream - unpatched for all the now
       fixed stream bugs */
class sample: public sound {
public:
    sample(bool throws) : sound(throws), init_(false), filled_(false) {
        alGenBuffers(1, &name_);
        init_ = true;
        check_error("alGenBuffers (sample::sample)");
    }
    virtual ~sample() {
        set_throws(false);
        if(init_) {
            alDeleteBuffers(1, &name_);
            check_error("alDeleteBuffers (sample:;~sample)");
        }
    }
    void apply(source& source);
    void deapply(source& source);        
protected:
    virtual bool fill_buffer(ALuint buf)=0;
private:
    ALuint name_;
    bool init_, filled_;
};    

class stream: public sound {
public:
    stream(int buffers, bool throws);
    virtual ~stream();
    void apply(source& source);
    void update();
    void deapply(source& source);
    bool looping();
    void set_looping(bool looping);
protected:
    virtual bool fill_buffer(ALuint buf) = 0;
    virtual bool can_loop()=0;
private:
    enum state { START, INIT, FILLING, FILLED, ENDING, ENDED  };
    void fill();
    void refresh_buffer(int buf);
    boost::shared_array<ALuint> buffers_;
    int num_buffers_;
    std::vector<source*> users_;
    ALint buffer_playing_;
    state state_;
    boost::shared_array<bool> buffer_live_;
    bool looping_;
};

std::string get_version();
std::string get_renderer();
std::string get_vendor();
std::vector<std::string> get_extensions();

}
#endif
#endif
