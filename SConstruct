# vi: syntax=python

# To use, type:
# scons crosscompile=1
# scons crosscompile=1 editor=1
#
# Also, for now, it has some hardcoded pathes of where I put my mingw stuff,
# need to find a better way to sort this out if anyone else is going to
# use it for regular cross compilation.

import glob

sources = glob.glob("*.cpp")
sources += ["xml/xml.c"]

crosscompile = ARGUMENTS.get("crosscompile", "")
editor = ARGUMENTS.get("editor", "")

name = "st"
if editor:
    sources.remove("main.cpp")
    editor_sources = glob.glob("editor/*.cpp")
    editor_sources.remove("editor/oldmain.cpp")
    sources.extend(editor_sources)
    name = "edit"

env = Environment()

if crosscompile:
    # The cross compiler to use
    env["CC"] = "i586-mingw32msvc-gcc"
    env["CXX"] = "i586-mingw32msvc-g++"

    # Dependencies are here for me (same folder as for Wesnoth).
    env.Append(CPPPATH = ["../win-deps/include"])
    
    env.Append(CPPPATH = ["/usr/i586-mingw32msvc/include/GL"])
    env.Append(CPPPATH = ["/usr/i586-mingw32msvc/include/SDL"])

    # This is where I put the dependency libs
    env.Append(LIBPATH = ["../win-deps/lib"])
    
    # This is where I placed the Qt stuff for mingw
    if editor:
        env.Append(CPPPATH = ["/home/elias/.wine/drive_c/Qt/4.3.1/include"])
        env.Append(LIBPATH = ["/home/elias/.wine/drive_c/Qt/4.3.1/lib"])
        env.Append(LIBS = ["QtCore4", "QtGui4", "QtOpenGL4"])

    # Compilation settings
    env.Append(CCFLAGS = ["-O2", "-mthreads"])
    env.Append(LINKFLAGS = ["-s", "-mwindows", "-lmingwthrd"])
    env.Append(CPPPATH = ["src", "src/widgets"])
    env.Append(LIBS = ["mingw32", "SDLmain", "SDL", "SDL_net", "SDL_mixer", "SDL_image",
        "SDL_ttf", "opengl32", "glu32", "boost_regex"])

    # Scons stuff
    env.BuildDir("../build/st-mingw", ".")
    env.SConsignFile("sconsign")

    # Windows stuff
    env["PROGSUFFIX"] = ".exe"

    # Compile it!
    env.Program(name, ["../build/st-mingw/" + x for x in sources])

else:
    # assume Linux
    env.ParseConfig("sdl-config --cflags")
    env.ParseConfig("sdl-config --libs")
    env.Append(CPPPATH = ["/usr/include/GL"])
    env.Append(LIBS = ["GL", "GLU", "SDL_image", "SDL_ttf", "boost_regex-mt"])
    
    if editor:
        env.ParseConfig("pkg-config --cflags --libs QtCore QtGui QtOpenGL")
    
    env.BuildDir("../build/st-posix", ".")
    env.SConsignFile("sconsign")
   
    env.Program(name, ["../build/st-posix/" + x for x in sources])

