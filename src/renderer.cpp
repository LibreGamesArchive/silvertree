#include "floating_label.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "frame_rate_utils.hpp"
#include "frustum.hpp"
#include "map_avatar.hpp"
#include "renderer.hpp"
#include "surface_cache.hpp"
#include "tile.hpp"
#include "tile_logic.hpp"

namespace graphics {

namespace {
    hex::frustum view_volume;

    const static int hex_texture_width = 128;
    const static int hex_texture_height = 128;
    const static int hex_border_left = 6;
    const static int hex_border_right = 0;
    const static int hex_tess_border_left = 39;
    const static int hex_tess_border_right = 33;
    const static int hex_border_top = 10;
    const static int hex_border_bottom = 12;
    const static int hex_border_width = hex_border_left + hex_border_right;
    const static int hex_border_height = hex_border_top + hex_border_bottom;
    const static int hex_tess_border_width = hex_tess_border_left + hex_tess_border_right;
    const static int hex_real_width = hex_texture_width - hex_border_width;
    const static int hex_real_height = hex_texture_height - hex_border_height;
    const static int hex_tess_height = hex_texture_height - hex_border_top - hex_border_bottom;
    const static int hex_tess_width = hex_texture_width - hex_tess_border_right + hex_border_left + 2;
}

decal::decal(const std::string& s, int priority) :
    texture_(graphics::texture::get(graphics::surface_cache::get(s))),
    priority_(priority) {
    init();
}

decal::decal(const texture& t, int priority) : 
    texture_(t), priority_(priority) {
    init();
}

void decal::init() {
    int radius_x, radius_y;
    radius_x = 
        texture_.width() / hex_texture_width + 
        (texture_.width() % hex_texture_width > 0 ? 1 : 0);
    radius_y = 
        texture_.height() / hex_texture_height + 
        (texture_.height() % hex_texture_height > 0 ? 1 : 0);

    radius_ = std::max(radius_x, radius_y);

    scale_x_ = texture_.ratio_w() * static_cast<GLfloat>(hex_texture_width)/texture_.width();
    scale_y_ = texture_.ratio_h() * static_cast<GLfloat>(hex_texture_height)/(texture_.height());
    normed_width_ = texture_.ratio_w()*hex_tess_width/static_cast<GLfloat>(hex_texture_width);
    normed_height_ = texture_.ratio_h()*hex_tess_height/static_cast<GLfloat>(hex_texture_height);

    woff_ = (texture_.width() - hex_texture_width)/2.0;
    woff_ /= static_cast<GLfloat>(hex_texture_width);
    hoff_ = (texture_.height()- hex_texture_height)/2.0;
    hoff_ /= static_cast<GLfloat>(hex_texture_height);
    
}

void decal::draw(const hex::tile& tile, const hex::gamemap& gmap) const {
    if(radius_ == 0) {
        return;
    }

    std::vector<const hex::tile*> tiles;
    {
        std::vector<hex::location> locs;

	get_locations_in_radius(tile.loc(), radius_-1, locs);
        foreach(hex::location& loc, locs) {
			if(!gmap.is_loc_on_map(loc)) {
				continue;
			}

            tiles.push_back(&(gmap.get_tile(loc)));
        }
    }

    const GLfloat tile_x = hex::tile::translate_x(tile.loc());
    const GLfloat tile_y = hex::tile::translate_y(tile.loc());

    glPushAttrib(GL_TEXTURE_BIT);

    texture_.set_as_current_texture();
    
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glScalef(scale_x_,scale_y_, 1);
    glTranslatef(woff_, hoff_, 0);
    glScalef(1.0/texture_.ratio_w(), 1.0/texture_.ratio_h(), 1);

    foreach(const hex::tile* t, tiles) {
        glPushMatrix();
        const GLfloat off_x = hex::tile::translate_x(t->loc()) - tile_x;
        const GLfloat off_y = hex::tile::translate_y(t->loc()) - tile_y;

        glTranslatef(off_x*normed_width_, -off_y*normed_height_, 0);
        
        glMatrixMode(GL_MODELVIEW);
        t->draw(texture_);
        glMatrixMode(GL_TEXTURE);
        glPopMatrix();
    }

    glPopMatrix();
    glPopAttrib();
    glMatrixMode(GL_MODELVIEW);
}

void renderer::set_pos(const GLfloat pos[3], const hex::location& loc) {
    bool changed = false;
    for(int i=0;i<3;++i) {
        GLfloat x = 0;
        if(pos[i] != 0) {
            x = pos[i];
        }
        if(centre_of_view_real_[i] != x) {
            centre_of_view_real_[i] = pos[i];
            changed = true;
        }
    }
    if(centre_of_view_map_ != loc) {
        centre_of_view_map_ = loc;
        changed = true;
    }

    if(changed) {
        camera_moving_ = true;
    }
}

void renderer::fill_tiles(std::vector<const hex::tile*>& tiles, const std::vector<hex::location>& locs) {
    foreach(hex::location loc, locs) {
        if(map_.is_loc_on_map(loc)) {
            tiles.push_back(&(map_.get_tile(loc)));
        }
    }
}

void renderer::fill_tiles(std::set<const hex::tile*>& tiles, const std::set<hex::location>& locs) {
    foreach(hex::location loc, locs) {
        if(map_.is_loc_on_map(loc)) {
            tiles.insert(tiles.begin(),&(map_.get_tile(loc)));
        }
    }
}

void renderer::fill_tiles(std::set<const hex::tile*>& tiles, const std::vector<hex::location>& locs) {
    foreach(hex::location loc, locs) {
        if(map_.is_loc_on_map(loc)) {
            tiles.insert(tiles.begin(),&(map_.get_tile(loc)));
        }
    }
}

void renderer::set_path(const std::vector<hex::location>& path) {
    clear_path();

    fill_tiles(path_tiles_, path);
}

void renderer::clear_path() {
    path_tiles_.clear();
}

void renderer::clear_highlights() {
    highlighted_tiles_.clear();
}

void renderer::add_highlights(const std::vector<hex::location>& hl) {
    fill_tiles(highlighted_tiles_, hl);
}

void renderer::add_highlight(const hex::location& loc) {
    if(map_.is_loc_on_map(loc)) {
        highlighted_tiles_.insert(highlighted_tiles_.begin(),&(map_.get_tile(loc)));
    }
}

void renderer::clear_decals() {
    decals_.clear();
}

void renderer::add_decal(const hex::location& loc, const decal& dec) {
    if(map_.is_loc_on_map(loc)) {
        const hex::tile* tile = &(map_.get_tile(loc));
        std::map<const hex::tile*,decal>::iterator itor = decals_.find(tile);
        if(itor == decals_.end() || itor->second.priority() < dec.priority()) {
            decals_.insert(decals_.begin(), std::pair<const hex::tile*,decal>(tile,dec)) ;
        }
    }
}

void renderer::add_decals(const std::vector<hex::location>& locs, const decal& dec) {
    foreach(const hex::location loc, locs) {
        if(map_.is_loc_on_map(loc)) {
            const hex::tile* tile = &(map_.get_tile(loc));
            std::map<const hex::tile*,decal>::iterator itor = decals_.find(tile);
            if(itor == decals_.end() || itor->second.priority() < dec.priority()) {
                decals_.insert(decals_.begin(), std::pair<const hex::tile*,decal>(tile,dec)) ;
            }
        }
    }
}

void renderer::add_sight_line(const GLfloat from[3], const GLfloat to[3], const GLfloat color[4]) {
    boost::shared_ptr<sight_line> sl(new sight_line());  
    for(int i=0;i<3;++i) {
        sl->from[i] = from[i];
        sl->to[i] = to[i];
        sl->color[i] = color[i];
    }
    sl->color[3] = color[3];
    sight_lines_.push_back(sl);
}

void renderer::clear_sight_lines() {
    sight_lines_.clear();
}

void renderer::reset_timing() {
    skippy_.reset();
    fps_track_.reset();
}

void renderer::reset_state() {
    clear_highlights();
    clear_path();
    clear_avatars();
    clear_sight_lines();
    clear_lights();
    clear_decals();
}

void renderer::add_avatar(hex::const_map_avatar_ptr avatar, int id) {
    if(avatar) {
        avatars_.push_back(avatar);
        avatar_ids_.push_back(id);
    }
}

void renderer::clear_avatars() {
    avatars_.clear();
    avatar_ids_.clear();
}

bool renderer::draw() const
{
    if(skippy_.skip_frame()) {
        fps_track_.register_frame(false);
        return false;
    }

    fps_track_.register_frame(true);

    camera_.set_pan(centre_of_view_real_);
    
    camera_.prepare_frame();
    if(camera_moving_  || 
       camera_.moved_since_last_check()) {
        rebuild_drawing_caches();
    } 
    
    camera_moving_ = false;
    
    set_lighting();
    
    hex::tile::setup_drawing();
    foreach(const hex::tile* t, tiles_) {
        t->draw();
    }
    
    foreach(const hex::tile* t, tiles_) {
        t->draw_cliffs();
    }
    
    foreach(const hex::tile* t, tiles_) {
        t->draw_model();
    }
    hex::tile::finish_drawing();
    
    foreach(const hex::tile* t, tiles_) {
        t->emit_particles(particle_system_);
    }
    
    if(show_grid_) {
        glDisable(GL_LIGHTING);
        foreach(const hex::tile* t, tiles_) {
            t->draw_grid();
        }
        glEnable(GL_LIGHTING);
    }
    
    foreach(const hex::tile* t, highlighted_tiles_) {
        t->draw_highlight();
    }

    for(std::map<const hex::tile*,decal>::const_iterator itor = decals_.begin();
        itor != decals_.end();
        ++itor) {
        itor->second.draw(*(itor->first), map_);
    }

    if(path_tiles_.empty() == false) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_LINE_STRIP);
        glColor4f(1.0,1.0,1.0,0.5);
        foreach(const hex::tile* t, path_tiles_) {
            t->draw_center();
        }
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
    }
    
    foreach(hex::const_map_avatar_ptr a, avatars_) {
        a->draw();
    }
    
    if(!sight_lines_.empty()) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glBegin(GL_LINES);
        foreach(boost::shared_ptr<sight_line> sl, sight_lines_) {
            glColor4fv(sl->color);
            glVertex3fv(sl->from);
            glVertex3fv(sl->to);
        }
        glEnd();
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
    }

    graphics::floating_label::draw_labels();
    particle_system_.draw();

    return true;
}
    
const std::string renderer::status_text() {
    return fps_track_.msg() + (formatter() << " " << tiles_.size()).str();
}

const hex::tile* renderer::get_tile_including_outside(const hex::location& loc) const
{
    if(map_.is_loc_on_map(loc)) {
        return &map_.get_tile(loc);
    } else {
        // the location is off the map. See if there is a default tile to use.
        if(border_tile_.empty()) {
            return NULL;
        } else {
            boost::shared_ptr<hex::tile>& ptr = outside_tiles_[loc];
            if(!ptr) {
                std::cerr << "init tiles..\n";
                ptr.reset(new hex::tile(loc, border_tile_));
                ptr->init_corners();
                ptr->init_normals();
            }
            
            return ptr.get();
        }
    }
}

void renderer::rebuild_drawing_caches() const
{
    hex::frustum::initialize();
    view_volume.set_volume_clip_space(-1, 1, -1, 1, -1, 1);
    tiles_.clear();
    int tiles_tried = 0;
    
    //find the initial ring which is within view
    hex::location hex_dir[6];
    for(int n = 0; n != 6; ++n) {
        hex_dir[n] = 
            hex::tile_in_direction(centre_of_view_map_, 
                                   static_cast<hex::DIRECTION>(n));
    }
    
    int core_radius = 1;
    bool done = false;
    while(!done) {
        for(int n = 0; n != 6; ++n) {
            hex_dir[n] = hex::tile_in_direction(hex_dir[n], static_cast<hex::DIRECTION>(n));
            const hex::tile* t = get_tile_including_outside(hex_dir[n]);
            if(!t || !view_volume.intersects(*t)) {
                done = true;
                break;
            }
        }
        
        ++core_radius;
    }
    
    done = false;
    std::vector<hex::location> hexes;
    for(int radius = 0; !done; ++radius) {
        hexes.clear();
        hex::get_tile_ring(centre_of_view_map_, radius, hexes);
        tiles_tried += hexes.size();
        done = true;
        foreach(const hex::location& loc, hexes) {
            const hex::tile* tile_ptr = get_tile_including_outside(loc);
            if(!tile_ptr) {
                continue;
            }
            
            const hex::tile& t = *tile_ptr;
            if(radius >= core_radius && !view_volume.intersects(t)) {
                //see if this tile has a cliff which is visible, in which case we should draw it.
                const hex::tile* cliffs[6];
                const int num_cliffs = t.neighbour_cliffs(cliffs);
                bool found = false;
                for(int n = 0; n != num_cliffs; ++n) {
                    if(view_volume.intersects(*cliffs[n])) {
                        found = true;
                        break;
                    }
                }
                
                if(!found) {
                    continue;
                }
            } else {
                done = false;
            }
            tiles_.push_back(&t);
            tiles_.back()->load_texture();
        }
    }
    
    std::sort(tiles_.begin(),tiles_.end(), hex::tile::compare_texture());
}

const hex::location& renderer::get_selected_hex() const
{
    if(map_.is_loc_on_map(selected_hex_)) {
        prepare_selection();
        glLoadName(0);
        hex::tile old_tile = map_.get_tile(selected_hex_);
        old_tile.draw();
        //old_tile.draw_cliffs();
        if(finish_selection() == 0) {
            return selected_hex_;
        }
        
    }
    
    prepare_selection();
    GLuint select_name = 0;
    foreach(const hex::tile* t, tiles_) {
        glLoadName(select_name);
        t->draw();
        ++select_name;
    }
    select_name = 0;
    foreach(const hex::tile* t, tiles_) {
        glLoadName(select_name);
        t->draw_cliffs();
        ++select_name;
    }
    
    std::vector<GLuint> selection;
    
    select_name = camera_.finish_selection(&selection);
    if(selection.size() > 1) {
        int high = -100000;
        foreach(GLuint n, selection) {
            if(tiles_[n]->height() > high) {
                select_name = n;
                high = tiles_[n]->height();
            }
        }
    }
    
    if(select_name == GLuint(-1)) {
        selected_hex_ = hex::location();
    } else {
        selected_hex_ = tiles_[select_name]->loc();
    }
    
    return selected_hex_;
}

int renderer::get_selected_avatar() const {
    prepare_selection();
    GLuint select_name = 0;
    foreach(hex::const_map_avatar_ptr a, avatars_) {
        glLoadName(select_name++);
        a->draw(true);
    }
    
    select_name = finish_selection();
    if(select_name == GLuint(-1)) {
        return -1;
    }
    return avatar_ids_[select_name];
}

void renderer::set_sky_color(GLfloat color[4]) {
    for(int i=0;i<4;++i) {
        sky_color_[i] = color[i];
    }
}

void renderer::add_light(int index, GLfloat position[4], 
                         GLfloat ambient[4], GLfloat diffuse[4],
                         GLfloat constant_attenuation,
                         GLfloat quadratic_attenuation) {
    light lt;
    lt.index = GL_LIGHT0+index;
    for(int i=0;i<4;++i) {
        lt.position[i] = position[i];
        lt.ambient[i]  = ambient[i];
        lt.diffuse[i]  = diffuse[i];
        lt.constant_attenuation = constant_attenuation;
        lt.quadratic_attenuation = quadratic_attenuation;
    }

    lights_.push_back(lt);
}

void renderer::clear_lights() {
    foreach(light lt, lights_) {
        glDisable(lt.index);
    }
    lights_.clear();
}

void renderer::set_lighting() const
{
    camera_.set_background_color(sky_color_);
    
    foreach(light lt, lights_) {
        glEnable(lt.index);
        glLightfv(lt.index, GL_POSITION, lt.position);
        glLightfv(lt.index, GL_AMBIENT, lt.ambient);
        glLightfv(lt.index, GL_DIFFUSE, lt.diffuse);
        glLightf(lt.index, GL_CONSTANT_ATTENUATION, 
                 lt.constant_attenuation);
        glLightf(lt.index, GL_QUADRATIC_ATTENUATION,
                 lt.quadratic_attenuation);
    }
}

void renderer::prepare_selection() const
{
    int mousex, mousey;
    SDL_GetMouseState(&mousex, &mousey);
    return camera_.prepare_selection(mousex,mousey);
}

unsigned int renderer::finish_selection() const
{
    return camera_.finish_selection();
}

}
