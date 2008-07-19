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
#include <fontconfig/fontconfig.h>
#include <pango/pangoft2.h>
#include <boost/lexical_cast.hpp>
#include <string>
#include <sstream>

#include "filesystem.hpp"
#include "pango_text.hpp"

using std::string;
using std::ostringstream;
using std::endl;

namespace
{

FT_Library library;
PangoFT2FontMap* font_map;
PangoContext* context;
PangoFontDescription* game_font;

gboolean filter_background(PangoAttribute *attribute, gpointer data)
{
	if(attribute->klass->type == PANGO_ATTR_BACKGROUND) {
		return TRUE;
	}
	return FALSE;
}

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
    FcConfig* fc_config = FcConfigGetCurrent();
    FcConfigAppFontAddFile(fc_config, (const FcChar8*)sys::find_file("FreeSans.ttf").c_str());
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
	::layout layout(font_size);
	layout.set_text(text);
	pango_layout_set_width(layout.get(), width * PANGO_SCALE);

	PangoLayoutIter* iter = pango_layout_get_iter(layout.get());
	ostringstream formatted_text;

	do {
		PangoLayoutLine* line = pango_layout_iter_get_line(iter);
		formatted_text << text.substr(line->start_index, line->length);
		if(!pango_layout_iter_at_last_line(iter))
			formatted_text << endl;
	} while(pango_layout_iter_next_line(iter));

	pango_layout_iter_free(iter);

	return formatted_text.str();
}
void renderer::get_text_size(const std::string& text, 
                             int font_size,
                             int *width, int *height) {
	::layout layout(font_size);
	layout.set_text(text);
	PangoRectangle rect(layout.logical_extents());
	*width = rect.width; *height = rect.height;
}

rendered_text_ptr renderer::render(const std::string& text, int size, const SDL_Color& color, bool markup)
{
	::layout layout(size);
	layout.set_text(text, markup);
    
	PangoRectangle rect(layout.logical_extents());

	ft_bitmap_ptr ptr(new ft_bitmap_handle(rect.width, rect.height));
	FT_Bitmap& bitmap = ptr->bitmap_;

	PangoAttrList* attrs = pango_layout_get_attributes(layout.get());
	PangoAttrList* bg_attrs = pango_attr_list_copy(attrs);
	if(attrs)
		pango_attr_list_filter(attrs, filter_background, NULL);
	pango_layout_set_attributes(layout.get(), attrs);
	pango_ft2_render_layout(&bitmap, layout.get(), 0, 0);
	pango_layout_set_attributes(layout.get(), bg_attrs);

	int image_size = rect.width * rect.height * (markup ? 4 : 1);
	boost::shared_array<unsigned char> pixels(new unsigned char[image_size]);

	if(!markup)
		memcpy(pixels.get(), bitmap.buffer, image_size);
	else {
		memset(pixels.get(), 0x00, image_size);

		PangoLayoutIter* iter = pango_layout_get_iter(layout.get());
		do {
			PangoRectangle ink_rect, logical_rect;
			PangoLayoutRun* run = pango_layout_iter_get_run(iter);
			if(run) {
				GSList *attrs = run->item->analysis.extra_attrs;
				PangoColor fgcolor = {}, bgcolor = {};
				bool have_fg = false, have_bg = false;
				while (attrs) {
					PangoAttribute *attr = (PangoAttribute*)attrs->data;
					if(attr->klass->type == PANGO_ATTR_FOREGROUND) {
						fgcolor = ((PangoAttrColor*)attr)->color;
						std::cout << "color\n";
						have_fg = true;
					}
					if(attr->klass->type == PANGO_ATTR_BACKGROUND) {
						bgcolor = ((PangoAttrColor*)attr)->color;
						have_bg = true;
					}
					attrs = attrs->next;
				}
				unsigned char fg[3] = { 0xFF, 0xFF, 0xFF };
				if(have_fg) {
					fg[0] = fgcolor.red / 0x100;
					fg[1] = fgcolor.green / 0x100;
					fg[2] = fgcolor.blue / 0x100;
				}
				unsigned char bg[3] = { 0xFF, 0xFF, 0xFF };
				if(have_bg) {
					bg[0] = bgcolor.red / 0x100;
					bg[1] = bgcolor.green / 0x100;
					bg[2] = bgcolor.blue / 0x100;
				}
				pango_layout_iter_get_run_extents(iter, &ink_rect, &logical_rect);
				int x =  ink_rect.x / PANGO_SCALE, y = ink_rect.y / PANGO_SCALE;
				int width = ink_rect.width / PANGO_SCALE, height = ink_rect.height / PANGO_SCALE;
				for(int row = y; row < y + height; row++) {
					for(int col = x; col < x + width; col++) {
						int pos = col + rect.width * row;
						if(!have_bg) {
							memcpy(&pixels[pos * 4], fg, 3);
							pixels[pos * 4 + 3] = bitmap.buffer[pos];
						} else {
							float alpha = bitmap.buffer[pos]/255.;
							pixels[pos * 4]     = static_cast<unsigned char>(fg[0] * alpha + bg[0] * (1 - alpha));
							pixels[pos * 4 + 1] = static_cast<unsigned char>(fg[1] * alpha + bg[1] * (1 - alpha));
							pixels[pos * 4 + 2] = static_cast<unsigned char>(fg[2] * alpha + bg[2] * (1 - alpha));
							pixels[pos * 4 + 3] = 0xFF;
						}
					}
				}
			}
		} while(pango_layout_iter_next_run(iter));
	}
	return rendered_text_ptr(new rendered_text(pixels, rect.width, rect.height, markup, color));
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
    if(!colored_) {
    	glPixelTransferf(GL_RED_BIAS, (float)color_.r/255.);
    	glPixelTransferf(GL_GREEN_BIAS, (float)color_.g/255.);
    	glPixelTransferf(GL_BLUE_BIAS, (float)color_.b/255.);
    }
    glDrawPixels(width_, height_, colored_ ? GL_RGBA : GL_ALPHA, GL_UNSIGNED_BYTE, pixels_.get());
    glEnable(GL_TEXTURE_2D);
    glPopAttrib();
}

graphics::texture rendered_text::as_texture()
{
    graphics::texture texture = graphics::texture::build(width_, height_);
    
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glPushAttrib(GL_CURRENT_BIT | GL_PIXEL_MODE_BIT);
    if(!colored_) {
    	glPixelTransferf(GL_RED_BIAS, (float)color_.r/255.);
    	glPixelTransferf(GL_GREEN_BIAS, (float)color_.g/255.);
    	glPixelTransferf(GL_BLUE_BIAS, (float)color_.b/255.);
    }
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width_, height_, 
                    colored_ ? GL_RGBA : GL_ALPHA, GL_UNSIGNED_BYTE, pixels_.get());
    //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, hnd_->bitmap_.width, hnd_->bitmap_.rows, 0, 
//                 GL_ALPHA, GL_UNSIGNED_BYTE, hnd_->bitmap_.buffer);
    glPopAttrib();
    
    return texture;
}

} // namespace pango

} // namespace text
