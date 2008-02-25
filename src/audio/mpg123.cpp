#include "mpg123.hpp"

/* for debug */
#include <iostream>

namespace mpg123 {
namespace {

class module {
public:
    module() { 
        err_ = mpg123_init();
    }
    ~module() {
        if(is_init()) {
            mpg123_exit();
        }
    }
    bool is_init() const {
        return err_ == MPG123_OK;
    }
    std::string get_error() const {
        return mpg123_plain_strerror(err_);
    }
private:
    int err_;
};

/* static module var - constructor inits, destructor exits */
static module libmpg123;

/* debug function */
#if 0
#define NUM_FLAGS 10
static std::string encoding_to_string(int enc) {
    const static std::string flag_text[NUM_FLAGS] = {
        "MPG123_ENC_16",
        "MPG123_ENC_SIGNED",
        "MPG123_ENC_8",
        "MPG123_ENC_SIGNED_16",
        "MPG123_ENC_UNSIGNED_16",
        "MPG123_ENC_UNSIGNED_8",
        "MPG123_ENC_SIGNED_8",
        "MPG123_ENC_ULAW_8",
        "MPG123_ENC_ALAW_8",
        "MPG123_ENC_ANY"
    };
    const static mpg123_enc_enum flags[NUM_FLAGS] = {
        MPG123_ENC_16,
        MPG123_ENC_SIGNED,
        MPG123_ENC_8,
        MPG123_ENC_SIGNED_16,
        MPG123_ENC_UNSIGNED_16,
        MPG123_ENC_UNSIGNED_8,
        MPG123_ENC_SIGNED_8,
        MPG123_ENC_ULAW_8,
        MPG123_ENC_ALAW_8,
        MPG123_ENC_ANY
    };
    std::string ret;
    bool first = true;

    for(int i = 0;i < NUM_FLAGS;++i) {
        if(flags[i] & enc) {
            if(!first) {
                ret += " ";
            }
            ret += flag_text[i];
            first = false;
        }
    }
    return ret;
}
#undef NUM_FLAGS

static std::string channels_to_string(int channels) {
    std::string ret;

    if(channels & MPG123_MONO) {
        if(channels & MPG123_STEREO) {
            ret = "MONO STEREO";
        } else {
            ret = "MONO";
        }
    } else if(channels & MPG123_STEREO) {
        ret = "STEREO";
    }
    return ret;
}
#endif

}

inline void stream::stream_call(int err, const std::string& op) {
    err_ = err;
    if(has_mpg_error()) {
        std::cout << mpg123_plain_strerror(err);
        if(!op.empty()) {
            std::cout << " in " << op;
        } 
        std::cout <<"\n";
        if(will_throw()) {
            throw exception(err_);
        }
    }
}

stream::stream(int buffers, bool throws) : 
    openal::stream(buffers, throws), mh_(mpg123_new(NULL, &err_))  {
    stream_call(err_, "mpg123_new (mpg123::stream::stream)");
    init();
}

stream::stream(const std::string& decoder, int buffers, bool throws) : 
    openal::stream(buffers, throws), mh_(mpg123_new(decoder.c_str(), &err_)) {

    stream_call(err_, "mpg123_new (mpg123::stream::stream(decoder))");
    init();
}

stream::~stream() {
    set_throws(false);
    close();
    /* void return despite library docs */
    mpg123_delete(mh_);
}

void stream::init() {
    buffer_size_ = mpg123_outblock(mh_ ) * 4;
    raw_buffer_.reset(new unsigned char[buffer_size_]);
}

std::string stream::get_mpg_error() const {
    return mpg123_plain_strerror(err_);
}

bool stream::has_mpg_error() const {
    switch(err_) {
    case MPG123_OK:
    case MPG123_DONE:
    case MPG123_NEW_FORMAT:
    case MPG123_NEED_MORE:
        return false;
    default:
        return true;
    }
}
            
void stream::open(const std::string& filename) {
    if(opened_) {
        close();
    }
    filename_ = filename;

    stream_call(mpg123_open(mh_, (char *)filename.c_str()),
                "mpg123_open (mpg123::stream::open)");    
    stream_call(mpg123_getformat(mh_, &rate_, &channels_, &mpg_encoding_),
                "mpg123_getformat (mpg123::stream::open)");
    stream_call(mpg123_format_none(mh_),
                "mpg123_format_none (mpg123::stream::open)");
    stream_call(mpg123_format(mh_, rate_, MPG123_STEREO, MPG123_ENC_UNSIGNED_16),
                "mpg123_format (mpg123::stream::open)");
    format_change_ = true;
}

void stream::close() {
    if(opened_) {
        stream_call(mpg123_close(mh_), "mpg123_close (mpg123::stream::close)");
    }
}

void stream::reopen() {
    close();
    open(filename_);
}

bool stream::fill_buffer(ALuint buffer) {
    size_t bytes_read;

    if(format_change_) {
        format_change_ = false;
        stream_call(mpg123_getformat(mh_, &rate_, &channels_, &mpg_encoding_),
                "mpg123_getformat (mpg123::stream::fillBuffer)");
        /* very counterintuitive behaviour here */
        if(mpg_encoding_ & MPG123_ENC_UNSIGNED_16) {
            if(channels_ & MPG123_STEREO) {
                al_encoding_ = AL_FORMAT_STEREO16;
            } else {
                al_encoding_ = AL_FORMAT_MONO16;
            }
        } else if(mpg_encoding_ & MPG123_ENC_SIGNED_8) {
            if(channels_ & MPG123_STEREO) {
                al_encoding_ = AL_FORMAT_STEREO8;
            } else {
                al_encoding_ = AL_FORMAT_MONO8;
            }
        } else {
            stream_call(MPG123_BAD_OUTFORMAT,
                        "invalid format change (mpg123::stream::fillBuffer)");
            return false;
        }
    }
    
    stream_call(mpg123_read(mh_, raw_buffer_.get(), buffer_size_, &bytes_read),
                "mpg123_read (mpg123::stream::fillBuffer)");
    
    if(err_ == MPG123_NEW_FORMAT) {
        format_change_ = true;
    } else if(err_ == MPG123_DONE) {
        if(looping()) {
            reopen();
        } else {
            return false;
        }
    }        

    alBufferData(buffer, al_encoding_, raw_buffer_.get(), bytes_read, rate_);
    return !check_error("alBufferData (mpg123::stream::fillBuffer)");
}

}
