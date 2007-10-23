# vi: syntax=python
from os.path import join

opts = Options("options.cache")
opts.AddOptions(
    ("QTDIR", "Root directory of Qt's installation.", "/usr/"),
    ("SDLDIR", "Root directory of SDL's installation.", "/usr/"),
    ("BOOSTDIR", "Root directory of boost installation.", "/usr/include"),
    ("BOOSTLIBS", "Directory where boost libs are located.", "/usr/lib"),
    ("BOOST_SUFFIX", "Suffix of the boost regex library.", ""),
    EnumOption("Build", "Build variant: debug or release", "release", ["debug", "release"], ignorecase=1),
    ("EXTRA_FLAGS_RELEASE", "Extra compiler/linker flags to use in release build variant.(e.g. \"-O3 -march=prescott\")", ""),
    ("EXTRA_FLAGS_DEBUG", "Extra compiler/linker flags to use in debug build variant.", "")
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

print "Configuring for " + env["PLATFORM"] + " platform..."
if env["PLATFORM"] == "posix" or env["PLATFORM"] == "darwin":
    env.ParseConfig("sdl-config --cflags --libs")
if env["PLATFORM"] == "win32":
    env.AppendUnique(CCFLAGS = ["-D_GNU_SOURCE"])
    env.AppendUnique(LIBS = Split("mingw32 SDLmain SDL SDL_image SDL_ttf"))
    env.AppendUnique(LINKFLAGS = ["-mwindows", "-s"])
env.AppendUnique(CPPPATH = [join(env["SDLDIR"], "include/SDL")], LIBPATH = [join(env["SDLDIR"], "lib")])
env.AppendUnique(CPPPATH = [env["BOOSTDIR"]], LIBPATH = [env["BOOSTLIBS"]])
conf = env.Configure()
if env["PLATFORM"] == "win32":
    conf.CheckLibWithHeader("opengl32", "GL/gl.h", "C") or Exit()
    conf.CheckLibWithHeader("glu32", "GL/glu.h", "C") or Exit()
else:
    conf.CheckLibWithHeader("GL", "GL/gl.h", "C") or Exit()
    conf.CheckLibWithHeader("GLU", "GL/glu.h", "C") or Exit()
if env["PLATFORM"] != "win32" and env["PLATFORM"] != "darwin":
    # SDL checks on windows and Mac don't work because SDL declares main() in SDL.h and assumes
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
editor_env["HaveQt"] = editor_conf.CheckLibWithHeader("QtCore" + QtLibSuffix, "QtGlobal", "C++", autoadd = False) and \
         editor_conf.CheckLibWithHeader("QtGui" + QtLibSuffix, "QApplication", "C++", autoadd = False) and \
         editor_conf.CheckLibWithHeader("QtOpenGL" + QtLibSuffix, "QGLWidget", "C++", autoadd = False)
editor_conf.Finish()

if "gcc" in env["TOOLS"]:
    ccflags = Split("-Wall -Wno-sign-compare -Wno-switch -Wno-switch-enum")
    linkflags = []
    if env["Build"] == "release":
        ccflags.append("-O2")
        linkflags.append("-s")
    if env["Build"] == "debug":
        ccflags.append("-ggdb")
    env.AppendUnique(CCFLAGS = ccflags, LINKFLAGS = linkflags)
    editor_env.AppendUnique(CCFLAGS = ccflags, LINKFLAGS = linkflags)

env.MergeFlags(env["EXTRA_FLAGS_" + env["Build"].upper()], unique=0)
editor_env.MergeFlags(env["EXTRA_FLAGS_" + env["Build"].upper()], unique=0)

namegen = env.Program("utilities/names/namegen", "utilities/names/namegen.cpp")
Alias("namegen", namegen)

env.Program("version_finder", "utilities/versions/version_finder.cpp")

Export("env")
Export("editor_env")
silvertree, editor = SConscript("src/SConscript", build_dir = join("build", env["Build"]))

if env["Build"] == "debug":
    ExecutableSuffix = "-debug" + env["PROGSUFFIX"]
else:
    ExecutableSuffix = env["PROGSUFFIX"]
env.Default(env.Alias("silvertreerpg", env.InstallAs("./silvertreerpg" + ExecutableSuffix, silvertree)))
if editor:
    editor_env.Alias("editor", editor_env.InstallAs("./silvertreeedit" + ExecutableSuffix, editor))

SConsignFile("sconsign")
