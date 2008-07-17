#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "filesystem.hpp"
#include "raster.hpp"
#include "surface.hpp"
#include "texture.hpp"
#include "world.hpp"

#include <SDL_image.h>

#include "audio/audio.hpp"
#include "audio/openal.hpp"

namespace title {

static void draw(game_logic::world_ptr w, const graphics::texture& title, Uint8 alpha) {
    const static SDL_Rect r = { 0,0,graphics::screen_width(), graphics::screen_height() };
    const SDL_Color c = { 0,0,0,0};

    bool was_blending = glIsEnabled(GL_BLEND);
    bool drew_frame = true;
    glEnable(GL_BLEND);

    glClearColor(0.0,0.0,0.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(alpha < 255) {
        drew_frame = w->draw();
    }
    if(drew_frame) {
        graphics::prepare_raster();
        
        if(alpha > 0) {
            graphics::draw_rect(r, c, alpha);
        }
        graphics::blit_texture(title, (graphics::screen_width() - title.width()) / 2,
                               (graphics::screen_height() - title.height())/2);
        SDL_GL_SwapBuffers();
    }
    if(!was_blending) {
        glDisable(GL_BLEND);
    }
}
void show(game_logic::world_ptr w, const std::string& logo, 
          const std::string& music_file, int fade_time) {
    const std::string fname = "./images/"+logo;
    graphics::texture title(graphics::texture::get_no_cache(graphics::surface(IMG_Load(sys::find_file(fname).c_str()))));

#ifdef AUDIO
    audio::audio_context_ptr ac;
    audio::stream_ptr music;
    audio::source_ptr source;
    if(audio::audio_available()) {
        ac.reset(new audio::audio_context());
        music = ac->make_stream(music_file);
        source = ac->make_source();
        
        source->set_sound(music.get());
        source->play();
        ac->pump_sound();
    }
#endif
    
    SDL_Event e;
    bool done = false;
    bool quit = false;

    /* this might look bizarre but it worksl ike this:
       the first bit draws something so we have something
       to look at. the world is not drawn since the background
       is opaque.
       the second bit forces the world to init itself (which
       is very very slow) so it doesn't screw up our
       fade speed calculations
     */
    draw(w, title, 255);
    w->draw();

    Uint32 end_time = SDL_GetTicks() + fade_time;
    Uint32 time;
    Uint8 alpha;

    w->renderer().reset_timing();
    while(!done) {
#ifdef AUDIO
        if(audio::audio_available()) {
            ac->pump_sound();
        }
#endif

        alpha = 127;
        if((time = SDL_GetTicks()) < end_time) {
            alpha += (Uint8)(128*(end_time - time) / fade_time);
        } 
        
        draw(w, title, alpha);

        while(SDL_PollEvent(&e)) {
            switch(e.type) {
            case SDL_QUIT:
                quit = true;
                done = true;
                break;
            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                default:
                    break;
                }
                done = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                done = true;
                break;
            default:
                break;
            }
        }
    }

    if(quit) {
        throw game_logic::world::quit_exception();
    }
    end_time = SDL_GetTicks()+fade_time;

    while((time = SDL_GetTicks()) < end_time) {
        Uint8 alpha = (Uint8)(128*(end_time - time) / fade_time);
        draw(w, title, alpha);
#ifdef AUDIO
        if(audio::audio_available()) {
            source->set_gain((end_time - time)/(float)fade_time);
            ac->pump_sound();
        }
#endif
    }

}

}
