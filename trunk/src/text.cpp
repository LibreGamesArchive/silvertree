#include "text.hpp"
#ifdef USE_PANGO
#include "pango_text.hpp"
#else
#include "ttf_text.hpp"
#endif

namespace text {

boost::scoped_ptr<renderer> renderer::renderer_;

renderer& renderer::instance() {
    if(!renderer_) {
#ifdef USE_PANGO
        renderer_.reset(new pango::renderer());
#else
        renderer_.reset(new ttf::renderer());
#endif
    }
    return *renderer_;
}

}
