/***************************************************************************
 *  Copyright (C) 2008 by Sergey Popov <loonycyborg@gmail.com>             *
 *                                                                         *
 *  This file is part of Silver Tree.                                      *
 *                                                                         *
 *  Silver Tree is free software; you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  Silver Tree is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftbitmap.h>
#include <pango/pangoft2.h>
#include <boost/lexical_cast.hpp>
#include <string>

#include "pango_text.hpp"

using std::string;

namespace
{

FT_Library library;
PangoFT2FontMap* font_map;
PangoContext* context;
PangoFontDescription* game_font;

class layout
{
	PangoLayout* layout_;

	public:
	explicit layout(int size)
	{
		layout_ = pango_layout_new(context);

		pango_font_description_set_absolute_size(game_font, size * PANGO_SCALE);
		pango_layout_set_font_description(layout_, game_font);
	}
	~layout()
	{
		g_object_unref(layout_);
	}
	PangoLayout* get() const
	{
		return layout_;
	}
	void set_text(string text, bool markup = false)
	{
		if(markup)
			pango_layout_set_markup(layout_, text.data(), text.size());
		else
			pango_layout_set_text(layout_, text.data(), text.size());
	}
	void extents(PangoRectangle* ink, PangoRectangle* logical)
	{
		 pango_layout_get_pixel_extents(layout_, ink, logical);
	}
	PangoRectangle ink_extents()
	{
		PangoRectangle rect;
		pango_layout_get_pixel_extents(layout_, &rect, NULL);
		return rect;
	}
	PangoRectangle logical_extents()
	{
		PangoRectangle rect;
		pango_layout_get_pixel_extents(layout_, NULL, &rect);
		return rect;
	}
};

}

namespace text {

namespace pango {

renderer::renderer()
{
    FT_Init_FreeType(&library);
    font_map = (PangoFT2FontMap*)pango_ft2_font_map_new();
    context = pango_ft2_font_map_create_context(font_map);
    pango_ft2_font_map_set_resolution(PANGO_FT2_FONT_MAP(font_map), 96, 96);
    game_font = pango_font_description_new();
    pango_font_description_set_family_static(game_font, "FreeSans");
}

renderer::~renderer()
{
    pango_font_description_free(game_font);
    g_object_unref(context);
    g_object_unref(font_map);
}

rendered_text_ptr renderer::render_complex_text(const std::string& text, int font_size,
                                                const SDL_Color& text_color,
                                                const SDL_Color& caret_fg, const SDL_Color& caret_bg,
                                                bool opaque_caret,
                                                const SDL_Color& selection_fg, const SDL_Color& selection_bg,
                                                bool opaque_selection,
                                                int caret, int selection_start, int selection_end) {
    return legacy_renderer_.render_complex_text(text, font_size, text_color, caret_fg, caret_bg,
                                                opaque_caret, selection_fg, selection_bg, opaque_selection,
                                                caret, selection_start, selection_end);
}

std::string renderer::format(const std::string& text, 
                             int font_size, 
                             int width) {
    return legacy_renderer_.format(text, font_size, width);
}
void renderer::get_text_size(const std::string& text, 
                             int font_size,
                             int *width, int *height) {
	::layout layout(font_size);
	layout.set_text(text);
	PangoRectangle rect(layout.logical_extents());
	*width = rect.width; *height = rect.height;
}

rendered_text_ptr renderer::render(const std::string& text, int size, const SDL_Color& color)
{
	::layout layout(size);
	layout.set_text(text);
    
	PangoRectangle rect(layout.logical_extents());

	ft_bitmap_ptr ptr(new ft_bitmap_handle(rect.width, rect.height));
	FT_Bitmap& bitmap = ptr->bitmap_;

	pango_ft2_render_layout(&bitmap, layout.get(), 0, 0);

	return rendered_text_ptr(new rendered_text(ptr, color));
}

ft_bitmap_handle::ft_bitmap_handle(int width, int height)
{
    bitmap_.width = width;
    bitmap_.rows = height;
    bitmap_.pitch = width;
    bitmap_.num_grays = 256;
    bitmap_.pixel_mode = FT_PIXEL_MODE_GRAY;
    bitmap_.buffer = new unsigned char[bitmap_.pitch * bitmap_.rows];
    memset(bitmap_.buffer, 0x00, bitmap_.pitch * bitmap_.rows);
}

ft_bitmap_handle::~ft_bitmap_handle()
{
    delete[] bitmap_.buffer;
}

void rendered_text::blit(int x, int y)
{
    glPushAttrib(GL_CURRENT_BIT | GL_PIXEL_MODE_BIT);
    glDisable(GL_TEXTURE_2D);
    glRasterPos2i(x,y);
    glPixelZoom(1, -1);
    glPixelTransferf(GL_RED_BIAS, (float)color_.r/255.);
    glPixelTransferf(GL_GREEN_BIAS, (float)color_.g/255.);
    glPixelTransferf(GL_BLUE_BIAS, (float)color_.b/255.);
    glDrawPixels(hnd_->bitmap_.width, hnd_->bitmap_.rows, 
                 GL_ALPHA, GL_UNSIGNED_BYTE, hnd_->bitmap_.buffer);
    glEnable(GL_TEXTURE_2D);
    glPopAttrib();
}

graphics::texture rendered_text::as_texture()
{
    graphics::texture texture = graphics::texture::build(hnd_->bitmap_.width, hnd_->bitmap_.rows);
    
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glPushAttrib(GL_CURRENT_BIT | GL_PIXEL_MODE_BIT);
    glPixelTransferf(GL_RED_BIAS, (float)color_.r/255.);
    glPixelTransferf(GL_GREEN_BIAS, (float)color_.g/255.);
    glPixelTransferf(GL_BLUE_BIAS, (float)color_.b/255.);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA, hnd_->bitmap_.width, hnd_->bitmap_.rows, 
                    GL_ALPHA, GL_UNSIGNED_BYTE, hnd_->bitmap_.buffer);
    //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, hnd_->bitmap_.width, hnd_->bitmap_.rows, 0, 
//                 GL_ALPHA, GL_UNSIGNED_BYTE, hnd_->bitmap_.buffer);
    glPopAttrib();
    
    return texture;
}

} // namespace pango

} // namespace text
