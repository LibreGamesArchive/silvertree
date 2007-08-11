CC=g++
CFLAGS=-c -g `sdl-config --cflags` -I/usr/include/GL -I/usr/local/include/boost-1_34 -I/System/Library/Frameworks/OpenGL.framework/Headers -I/sw/include -I/usr/include/qt4
LDFLAGS=`sdl-config --libs` -lGL -lGLU -lSDL_image -lSDL_ttf -L/System/Library/Frameworks/OpenGL.framework/Libraries -lboost_regex-mt
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o) xml.o
EDITOR_OBJECTS=editor/main.o editor/editorglwidget.o gamemap.o wml_node.o wml_parser.o camera.o tile.o base_terrain.o model.o string_utils.o tile_logic.o parse3ds.o parseark.o parsedae.o terrain_feature.o material.o font.o texture.o filesystem.o raster.o surface_cache.o surface.o sdl_algo.o xml.o
EXECUTABLE=game

all: $(SOURCES) $(EXECUTABLE)

clean:
	rm *.o editor/*.o editor/edit game

cleanvi:
	rm .*swp

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

editor/editorglwidget.moc: editor/editorglwidget.hpp
	cd editor ; moc editorglwidget.hpp > editorglwidget.moc

edit: editor/editorglwidget.moc $(EDITOR_OBJECTS)
	$(CC) $(LDFLAGS) $(EDITOR_OBJECTS) -lQtCore -lQtGui -lQtOpenGL -o edit

formula_test: formula.cpp formula_tokenizer.cpp variant.cpp
	g++ formula.cpp formula_tokenizer.cpp variant.cpp -DUNIT_TEST_FORMULA $(LDFLAGS) -o formula_test

xml_test: xml/xml.c
	gcc xml/xml.c -g -DUNIT_TEST_XML $(LDFLAGS) -o xml_test

xml.o: xml/xml.c
	gcc -c xml/xml.c -o xml.o

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
