
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <algorithm>
#include <assert.h>
#include <string.h>

#include "sdl_algo.hpp"

namespace graphics {

namespace {
struct surface_lock {
	explicit surface_lock(const surface& surf) : surface_(surf), locked_(false)
	{
		if(SDL_MUSTLOCK(surface_)) {
			const int res = SDL_LockSurface(surface_);
			if(res == 0) {
				locked_ = true;
			}
		}
	}

	~surface_lock()
	{
		if(locked_) {
			SDL_UnlockSurface(surface_);
		}
	}

	Uint32* pixels() { return reinterpret_cast<Uint32*>(surface_->pixels); }

private:
	const surface surface_;
	bool locked_;
};
}

surface get_surface_portion(surface surf, const SDL_Rect& rect)
{
	surface res(SDL_CreateRGBSurface(SDL_SWSURFACE,rect.w,rect.h,surf->format->BitsPerPixel,surf->format->Rmask,surf->format->Gmask,surf->format->Bmask,surf->format->Amask));
	assert(res.get());
	surface_lock lock(surf);
	surface_lock dst_lock(res);
	const Uint32* beg = lock.pixels() + surf->w*rect.y;
	const Uint32* end = beg + surf->w*rect.h;

	int row = 0;
	while(beg != end) {
		const Uint32* beg_row = beg + rect.x;
		const Uint32* end_row = beg_row + rect.w;
		Uint32* dst = dst_lock.pixels() + row*res->w;
		memcpy(dst,beg_row,(end_row-beg_row)*sizeof(*dst));
		beg += surf->w;
		++row;
	}

	return res;
}

surface get_non_transparent_portion(surface surf)
{
	if(!surf.get()) {
		return surf;
	}

	SDL_Rect rect = get_non_transparent_rect(surf);
	if(rect.w == surf->w && rect.h == surf->h) {
		return surf;
	}

	return get_surface_portion(surf,rect);
}

SDL_Rect get_non_transparent_rect(surface surf)
{
	if(surf->format->BitsPerPixel != 32) {
		surf = surf.convert_opengl_format();
	}

	assert(surf->format->BitsPerPixel == 32);
	const Uint32 amask = surf->format->Amask;
	
	surface_lock lock(surf);
	const Uint32* data = lock.pixels();
	const Uint32* end_data = data + surf->w*surf->h;
	while(data != end_data) {
		if((*data)&amask) {
			break;
		}

		++data;
	}

	const Uint32 first_row = std::min((data - lock.pixels())/surf->w,surf->h-1);
	
	const Uint32* data_rev = end_data-1;
	while(data_rev > data) {
		if((*data_rev)&amask) {
			break;
		}

		--data_rev;
	}

	const Uint32 last_row = std::min((data_rev - lock.pixels())/surf->w,surf->h-1);
	int left = -1;
	int right = -1;

	for(Uint32 row = first_row; row <= last_row; ++row) {
		const Uint32* beg = lock.pixels() + row*surf->w;
		const Uint32* end = lock.pixels() + (row+1)*surf->w;
		const Uint32* i = beg;
		while(i != end) {
			if((*i)&amask) {
				break;
			}
			++i;
		}

		if(left == -1 || (i - beg) < left) {
			left = i - beg;
		}

		i = end-1;
		while(i > beg) {
			if((*i)&amask) {
				break;
			}
			--i;
		}

		if(right == -1 || (i - beg) > right) {
			right = i - beg;
		}
	}
	
	SDL_Rect rect = {left, first_row, (right - left)+1, (last_row - first_row)+1};
	return rect;
}

}
