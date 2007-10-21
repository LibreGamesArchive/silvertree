# vi: syntax=python

opts = Options("options.cache")
opts.AddOptions(
    ("QTDIR", "Root directory of Qt's installation.", "/usr/"),
    ("SDLDIR", "Root directory of SDL's installation.", "/usr/"),
    ("BOOSTDIR", "Root directory of boost installation.", "/usr/"),
    ("BOOSTLIBS", "Directory where boost libs are located.", "/usr/lib"),
    ("BOOST_SUFFIX", "Suffix of the boost regex library.", "")
)

env = Environment(tools = [], toolpath = ["scons"], options = opts)
if env["PLATFORM"] == "win32":
    env.Tool("mingw")
else:
    env.Tool("default")
opts.Save("options.cache", env)

Help("""
Type "scons" to build the game.
"scons editor"         - builds the editor.
"scons namegen"        - namegen.
"scons version_finder" - version finder.

Additional options:
""")
Help(opts.GenerateHelpText(env))

if env["PLATFORM"] == "posix":
    env.ParseConfig("sdl-config --cflags --libs")
if env["PLATFORM"] == "win32":
    env.AppendUnique(CCFLAGS = ["-D_GNU_SOURCE"])
    env.AppendUnique(LIBS = Split("mingw32 SDLmain SDL SDL_image SDL_ttf"))
    env.AppendUnique(LINKFLAGS = ["-mwindows", "-s"])
from os.path import join
env.AppendUnique(CPPPATH = [join(env["SDLDIR"], "include/SDL")], LIBPATH = [join(env["SDLDIR"], "lib")])
env.AppendUnique(CPPPATH = [env["BOOSTDIR"]], LIBPATH = [env["BOOSTLIBS"]])
conf = env.Configure()
if env["PLATFORM"] == "win32":
    conf.CheckLibWithHeader("opengl32", "GL/gl.h", "C") or Exit()
    conf.CheckLibWithHeader("glu32", "GL/glu.h", "C") or Exit()
else:
    conf.CheckLibWithHeader("GL", "GL/gl.h", "C") or Exit()
    conf.CheckLibWithHeader("GLU", "GL/glu.h", "C") or Exit()
if env["PLATFORM"] != "win32":
    # SDL checks on windows don't work because SDL declares main() in SDL.h and assumes
    # int main(int,char**) while checks use int main(void)
    conf.CheckLibWithHeader("SDL", "SDL_config.h", "C", autoadd = False) or Exit()
    conf.CheckLibWithHeader("SDL_image", "SDL_image.h", "C") or Exit()
    conf.CheckLibWithHeader("SDL_ttf", "SDL_ttf.h", "C") or Exit()
if env["BOOST_SUFFIX"]:
    conf.CheckLibWithHeader("boost_regex" + env["BOOST_SUFFIX"], "boost/regex.hpp", "C++") or Exit()
else:
    conf.CheckLibWithHeader("boost_regex", "boost/regex.hpp", "C++") or \
    conf.CheckLibWithHeader("boost_regex-mt", "boost/regex.hpp", "C++") or Exit()
conf.Finish()

if "gcc" in env["TOOLS"]:
    env.Append(CCFLAGS = Split("-ggdb -Wall -Wno-sign-compare -Wno-switch -Wno-switch-enum"))

editor_env = env.Clone()
try:
    editor_env.Tool("qt4")
    editor_env.EnableQt4Modules(["QtCore", "QtGui", "QtOpenGL"])
    editor_env.Replace(QT4_UICIMPLSUFFIX=".hpp", QT4_UICDECLSUFFIX=".hpp",
        QT4_UICIMPLPREFIX="", QT4_UICDECLPREFIX="", QT4_MOCHPREFIX="", QT4_MOCHSUFFIX=".moc",
        QT4_AUTOSCAN=0)
except:
    pass
editor_conf = editor_env.Configure()
if env["PLATFORM"] == "win32":
    QtLibSuffix = "4"
else:
    QtLibSuffix = ""
HaveQt = editor_conf.CheckLibWithHeader("QtCore" + QtLibSuffix, "QtGlobal", "C++", autoadd = False) and \
         editor_conf.CheckLibWithHeader("QtGui" + QtLibSuffix, "QApplication", "C++", autoadd = False) and \
         editor_conf.CheckLibWithHeader("QtOpenGL" + QtLibSuffix, "QGLWidget", "C++", autoadd = False)
editor_conf.Finish()

import glob
sources = glob.glob("src/*.cpp")
sources += ["src/xml/xml.c"]
sources.remove(join("src", "main.cpp"))
lib_silvertree = env.StaticLibrary("src/libsilvertree", sources)
silvertree = env.Program("silvertreerpg", ["src/main.cpp", lib_silvertree])
env.Default(silvertree)

if HaveQt:
    editor_sources = glob.glob("src/editor/*.cpp")
    editor_sources.remove(join("src", join("editor", "oldmain.cpp")))
    editor_uis = glob.glob("src/editor/*.ui")
    editor_mocables = Split("""
        src/editor/editorglwidget.hpp
        src/editor/editormainwindow.hpp
        src/editor/terrainhandler.hpp
        src/editor/editpartydialog.hpp
        src/editor/editwmldialog.hpp
     """)
    editor_env.Uic4(editor_uis)
    for mocable in editor_mocables: # Moc4 doesn't handle lists properly
        editor_env.Moc4(mocable)
    editor = editor_env.Program("silvertreeedit", editor_sources + [lib_silvertree])
    editor_env.Alias("editor", editor)
else:
    print "Couldn't find Qt. Editor cannot be built."

namegen = env.Program("utilities/names/namegen", "utilities/names/namegen.cpp")
Alias("namegen", namegen)

env.Program("version_finder", "utilities/versions/version_finder.cpp")

SConsignFile("sconsign")
