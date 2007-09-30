CFLAGS = -Wall -Werror -g `sdl-config --cflags` -I/usr/include/GL \
	 -I/usr/local/include/boost-1_34 \
	 -I/System/Library/Frameworks/OpenGL.framework/Headers \
	 -I/sw/include
LDFLAGS= `sdl-config --libs` \
	 -L/System/Library/Frameworks/OpenGL.framework/Libraries \
	 -lSDL_image -lSDL_ttf -lGL -lGLU 
SOURCES=version_finder.c
PROG=version_finder

all: $(PROG)

$(PROG): $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm $(PROG)

.PHONY: all clean

