#include "text.hpp"
#ifdef USE_PANGO
#include "pango_text.hpp"
#else
#include "ttf_text.hpp"
#endif

namespace text {

renderer_ptr instance_;

renderer_ptr renderer::instance() {
    if(!instance_) {
#ifdef USE_PANGO
        instance_.reset(new pango::renderer());
#else
        instance_.reset(new ttf::renderer());
#endif
    }
    return instance_;
}

}
