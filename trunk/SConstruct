# vi: syntax=python

# To use, type:
# scons crosscompile=1
# scons crosscompile=1 editor=1
#
# Also, for now, it has some hardcoded pathes of where I put my mingw stuff,
# need to find a better way to sort this out if anyone else is going to
# use it for regular cross compilation.

# Note: You need to first build with the makefile, to do all the Qt4 magic.

# Pathes to use:
BUILD_PATH = "../build"
EXTRA_INCLUDE_PATHS = ["../win-deps/include",
    "/usr/i586-mingw32msvc/include/GL",
    "/usr/i586-mingw32msvc/include/SDL"] 
EXTRA_EDIT_INCLUDE_PATHS = ["/home/elias/.wine/drive_c/Qt/4.3.1/include"]
EXTRA_LIB_PATHS = ["../win-deps/lib"]
EXTRA_EDIT_LIB_PATHS = ["/home/elias/.wine/drive_c/Qt/4.3.1/lib"]

import glob

sources = glob.glob("*.cpp")
sources += ["xml/xml.c"]

crosscompile = ARGUMENTS.get("crosscompile", "")
editor = ARGUMENTS.get("editor", "")

name = "game"
if editor:
    sources.remove("main.cpp")
    editor_sources = glob.glob("editor/*.cpp")
    editor_sources.remove("editor/oldmain.cpp")
    
    sources.extend(editor_sources)
    name = "edit"

env = Environment()

def buildpath():
    name = BUILD_PATH
    if editor: name += "/st-edit"
    else: name += "st-game"
    if crosscompile: name += "-mingw"
    else: name += "-posix"
    return name + "/"

bdir = buildpath()
realsources = [bdir + x for x in sources] 

if crosscompile:
    # The cross compiler to use
    env["CC"] = "i586-mingw32msvc-gcc"
    env["CXX"] = "i586-mingw32msvc-g++"

    # Dependencies are here for me (same folder as for Wesnoth).
    env.Append(CPPPATH = EXTRA_INCLUDE_PATHS)

    # This is where I put the dependency libs
    env.Append(LIBPATH = EXTRA_LIB_PATHS)
    
    # This is where I placed the Qt stuff for mingw
    if editor:
        env.Append(CPPPATH = EXTRA_EDIT_INCLUDE_PATHS)
        env.Append(LIBPATH = EXTRA_EDIT_LIB_PATHS)
        env.Append(LIBS = ["QtCore4", "QtGui4", "QtOpenGL4"])

    # Compilation settings
    env.Append(CCFLAGS = ["-O2", "-mthreads"])
    env.Append(LINKFLAGS = ["-s", "-mwindows", "-lmingwthrd"])
    env.Append(CPPPATH = ["src", "src/widgets"])
    env.Append(LIBS = ["mingw32", "SDLmain", "SDL", "SDL_net", "SDL_mixer", "SDL_image",
        "SDL_ttf", "opengl32", "glu32", "boost_regex"])

    env["PROGSUFFIX"] = ".exe"

else:
    # assume Linux
    env.ParseConfig("sdl-config --cflags")
    env.ParseConfig("sdl-config --libs")
    env.Append(CPPPATH = ["/usr/include/GL"])
    env.Append(LIBS = ["GL", "GLU", "SDL_image", "SDL_ttf", "boost_regex-mt"])
    env.ParseConfig("pkg-config --cflags --libs QtCore QtGui QtOpenGL")
            
env.BuildDir(bdir, ".")
env.SConsignFile("sconsign")
env.Program(name, realsources)

