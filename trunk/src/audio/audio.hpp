#ifndef AUDIO_HPP_INCLUDED
#define AUDIO_HPP_INCLUDED

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "scoped_shared_ptr.hpp"

/* avoid pulling all this into every file */
namespace openal {
    class sample;
    class stream;
    class source;
}
namespace openalc {
    class context;
    class device;
}

namespace audio {

typedef boost::shared_ptr<openal::sample> sample_ptr;
typedef boost::shared_ptr<openal::stream> stream_ptr;
typedef boost::weak_ptr<openal::stream> stream_weak_ptr;
typedef boost::shared_ptr<openal::source> source_ptr;
typedef boost::weak_ptr<openal::source> source_weak_ptr;

class audio_context {
public:
    audio_context();
    virtual ~audio_context();
    stream_ptr make_stream(const std::string& filename);
    sample_ptr make_sample(const std::string& filename);
    source_ptr make_source();
    void pump_sound();
    void freeze();
    void resume();
private:
    void clean_up();
    std::vector<stream_weak_ptr> streams_;
    std::vector<source_weak_ptr> sources_;
    boost::shared_ptr<openalc::context> context_;
    bool frozen_;
};

typedef boost::shared_ptr<audio_context> audio_context_ptr;
typedef util::scoped_shared_ptr<audio_context> scoped_audio_context_ptr;

void init_audio(const std::string& device);
void init_audio();

}

#endif
