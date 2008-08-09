#ifndef RENDERER_HPP_INCLUDED
#define RENDERER_HPP_INCLUDED

#include <GL/glew.h>

#include <map>
#include <vector>
#include <set>

#include "camera.hpp"
#include "frame_rate_utils.hpp"
#include "gamemap.hpp"
#include "map_avatar.hpp"
#include "particle_system.hpp"
#include "preferences.hpp"

namespace graphics {

struct light {
    GLint index;
    GLfloat position[4];
    GLfloat diffuse[4];
    GLfloat ambient[4];
    GLfloat constant_attenuation;
    GLfloat quadratic_attenuation;
};

struct sight_line {
    GLfloat from[3];
    GLfloat to[3];
    GLfloat color[4];
};

class decal {
public:
    // ALLOW implicit please
    decal(const texture& t, int priority=0);
    explicit decal(const std::string& s, int priority=0);
    void draw(const hex::tile& pos, const hex::gamemap& gmap) const;
    int priority() const { return priority_; }
private:
    void init();

    texture texture_;
    int radius_;
    int priority_;
    GLfloat scale_x_, scale_y_, normed_width_, normed_height_, hoff_, woff_;
};

class renderer {
public:
    explicit renderer(const hex::gamemap& gmap, hex::camera& camera) 
        : map_(gmap), show_grid_(false), skippy_(50, preference_maxfps()), camera_(camera)  { }

    virtual ~renderer() {}

    typedef std::vector<hex::location> loc_list;

    void set_pos(const GLfloat pos[3], const hex::location& loc);
    const std::string status_text();
    void add_highlight(const hex::location& highlight);
    void add_highlights(const std::vector<hex::location>& highlights);
    void clear_highlights();
    void add_decal(const hex::location& loc, const decal& dec);
    void add_decals(const std::vector<hex::location>& locs, const decal& dec);
    void clear_decals();
    void set_path(const std::vector<hex::location>& path);
    void clear_path();
    void add_avatar(hex::const_map_avatar_ptr avatar, int id=-1);
    void clear_avatars();
    bool draw() const;
    const hex::location& get_selected_hex() const;
    int get_selected_avatar() const;
    void set_sky_color(GLfloat color[4]);
    void add_light(int index, GLfloat position[4], 
                   GLfloat ambient[4], GLfloat diffuse[4],
                   GLfloat constant_attenuation,
                   GLfloat quadratic_attenuation);
    void clear_lights();
    void add_sight_line(const GLfloat from[3], 
                        const GLfloat to[3], 
                        const GLfloat color[4]);
    void clear_sight_lines();
    void reset_state();
    void reset_timing();
    void set_show_grid(bool show) { show_grid_ = show; }
    bool get_show_grid() { return show_grid_; }
private:
    void prepare_selection() const;
    unsigned int finish_selection() const;
    void fill_tiles(std::vector<const hex::tile*>& tiles,
                    const std::vector<hex::location>& locs);
    void fill_tiles(std::set<const hex::tile*>& tiles,
                    const std::set<hex::location>& locs);
    void fill_tiles(std::set<const hex::tile*>& tiles,
                    const std::vector<hex::location>& locs);
    void rebuild_drawing_caches() const;
    void set_lighting() const;
    const hex::tile* get_tile_including_outside(const hex::location& loc) const;
    
    std::string border_tile_;
    typedef std::map<hex::location,boost::shared_ptr<hex::tile> > outside_tiles_map;
    hex::location centre_of_view_map_;
    GLfloat centre_of_view_real_[3];
    std::vector<const hex::tile*> visible_tiles_;
    std::set<const hex::tile*> highlighted_tiles_;
    std::vector<const hex::tile*> path_tiles_;
    std::map<const hex::tile*,decal> decals_;
    const hex::gamemap& map_;
    std::vector<hex::const_map_avatar_ptr> avatars_;
    std::vector<int> avatar_ids_;
    std::vector<boost::shared_ptr<sight_line> > sight_lines_;
    bool show_grid_;
    std::vector<light> lights_;
    GLfloat sky_color_[4];
    mutable frame_skipper skippy_;
    mutable frame_rate_tracker fps_track_;
    mutable hex::location selected_hex_;
    mutable std::vector<const hex::tile*> tiles_;
    mutable hex::camera& camera_;
    mutable bool camera_moving_;
    mutable particle_system particle_system_;
    mutable outside_tiles_map outside_tiles_;
};

}

#endif
