# vi: syntax=python

Help("""
Type "scons" to build the game. Additional options:

* crosscompile=1 Use the MinGW crosscompiler.
* editor=1       Build the editor instead of the game.
* debug=1        Use debug mode instead of release mode.
* builddir=X     Use X as build dir [build].
* includedir=X   Use X as extra path to look for includes, split multiple
                 ones with a colon.
* libdir=X       Same as includedir but for libraries.
""")

crosscompile = ARGUMENTS.get("crosscompile", "")
editor = ARGUMENTS.get("editor", "")
debug = ARGUMENTS.get("debug", "")
builddir = ARGUMENTS.get("builddir", "build")
includedir = ARGUMENTS.get("includedir", "")
libdir = ARGUMENTS.get("libdir", "")

import glob

sources = glob.glob("*.cpp")
sources += ["xml/xml.c"]

name = "game"
if editor:
    sources.remove("main.cpp")
    editor_sources = glob.glob("editor/*.cpp")
    editor_sources.remove("editor/oldmain.cpp")
    editor_uis = glob.glob("editor/*.ui")
    editor_mocables = Split("""
        editor/editorglwidget.hpp
        editor/editormainwindow.hpp
        editor/terrainhandler.hpp
        editor/editpartydialog.hpp
        editor/editwmldialog.hpp
     """)
    
    sources.extend(editor_sources)
    name = "edit"

env = Environment()

def buildpath():
    name = builddir
    if editor: name += "/st-edit"
    else: name += "/st-game"
    if crosscompile: name += "-mingw"
    else: name += "-posix"
    if debug: name += "-debug"
    else: name += "-release"
    return name + "/"

bdir = buildpath()
realsources = [bdir + x for x in sources] 

if debug:
    env.Append(CCFLAGS = ["-g3"])
else:
    env.Append(CCFLAGS = ["-O2"])

if crosscompile:
    # The cross compiler to use
    env["CC"] = "i586-mingw32msvc-gcc"
    env["CXX"] = "i586-mingw32msvc-g++"

    # Dependencies are here for me (same folder as for Wesnoth).
    env.Append(CPPPATH = includedir.split(":"))

    # This is where I put the dependency libs
    env.Append(LIBPATH = libdir.split(":"))
    
    # This is where I placed the Qt stuff for mingw
    if editor:
        env.Append(LIBS = ["QtCore4", "QtGui4", "QtOpenGL4"])

    # Compilation settings
    env.Append(CCFLAGS = ["-mthreads"])
    env.Append(LINKFLAGS = ["-s", "-mwindows", "-lmingwthrd"])
    env.Append(CPPPATH = ["src", "src/widgets"])
    env.Append(LIBS = ["mingw32", "SDLmain", "SDL", "SDL_net", "SDL_mixer", "SDL_image",
        "SDL_ttf", "opengl32", "glu32", "boost_regex"])

    env["PROGSUFFIX"] = ".exe"

else:
    # assume Linux
    env.AppendUnique(QTDIR='/usr/')
    env.Tool('qt4', toolpath=['scons'])

    env.ParseConfig("sdl-config --cflags")
    env.ParseConfig("sdl-config --libs")
    env.Append(CPPPATH = ["/usr/include/GL"])
    env.Append(LIBS = ["GL", "GLU", "SDL_image", "SDL_ttf", "boost_regex"])
    if editor:
        env.EnableQt4Modules(['QtCore', 'QtGui', 'QtOpenGL'])
        env.Replace(QT4_UICIMPLSUFFIX=".hpp", QT4_UICDECLSUFFIX=".hpp",
            QT4_UICIMPLPREFIX="ui_", QT4_MOCHPREFIX="", QT4_MOCHSUFFIX=".moc",
            QT4_AUTOSCAN=0)
        env.Uic4([bdir + x for x in editor_uis])
        for mocable in editor_mocables: # Moc4 doesn't handle lists properly
            env.Moc4(bdir + mocable)
            
env.BuildDir(bdir, ".")
env.SConsignFile("sconsign")
env.Program(name, realsources)

