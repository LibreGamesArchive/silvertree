CC=g++
CFLAGS=-c -O2 -g `sdl-config --cflags` -I/usr/local/include/boost-1_34 -I/System/Library/Frameworks/OpenGL.framework/Headers -I/sw/include -I/Library/Frameworks/SDL_ttf.framework/Headers
LDFLAGS=-O2 `sdl-config --libs` -lGL -lGLU -lSDL_image -lSDL_ttf -L/System/Library/Frameworks/OpenGL.framework/Libraries -lboost_regex -L/Users/jbjerk/Documents/SDL_ttf-2.0.9/.libs
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
EDITOR_OBJECTS=editor/main.o gamemap.o wml_node.o wml_parser.o camera.o tile.o base_terrain.o model.o string_utils.o tile_logic.o parse3ds.o parseark.o terrain_feature.o material.o font.o texture.o filesystem.o raster.o surface_cache.o surface.o sdl_algo.o
EXECUTABLE=game

all: $(SOURCES) $(EXECUTABLE)

clean:
	rm *.o editor/*.o editor/edit game

cleanvi:
	rm .*swp

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

edit: $(EDITOR_OBJECTS)
	$(CC) $(LDFLAGS) $(EDITOR_OBJECTS) -o edit

formula_test: formula.cpp formula_tokenizer.cpp
	g++ formula.cpp formula_tokenizer.cpp -DUNIT_TEST_FORMULA $(LDFLAGS) -o formula_test

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
