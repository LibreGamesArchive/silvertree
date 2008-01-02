# vi: syntax=python:et:ts=4

from os.path import join
from glob import glob
import sys
sys.path.append("./scons")
from build_output import setup_build_output

opts = Options("options.cache")
opts.AddOptions(
    ("PREFIX", "Install prefix.", "/usr/local"),
    ("STAGE_PREFIX", "Install stage prefix.", "$PREFIX"),
    ("QT4DIR", "Root directory of Qt's installation.", "/usr/"),
    EnumOption("GL_IMPL_MAC", "Mac-specific: OpenGL implementation to be used: Mesa 3D or Apple.", "mesa", ["mesa", "apple"], ignorecase=1),
    ("SDLDIR", "Root directory of SDL's installation.", "/usr/"),
    ("BOOSTDIR", "Root directory of boost installation.", "/usr/include"),
    ("BOOSTLIBS", "Directory where boost libs are located.", "/usr/lib"),
    ("BOOST_SUFFIX", "Suffix of the boost regex library.", ""),
    EnumOption("Build", "Build variant: debug or release", "release", ["debug", "release"], ignorecase=1),
    ("EXTRA_FLAGS_RELEASE", "Extra compiler/linker flags to use in release build variant.(e.g. \"-O3 -march=prescott\")", ""),
    ("EXTRA_FLAGS_DEBUG", "Extra compiler/linker flags to use in debug build variant.", ""),
    BoolOption("VERBOSE_BUILD_OUTPUT", "If true, SCons will display full command lines of commands it's running.", False),
)

env = Environment(tools = ["zip", "config_checks"], toolpath = ["scons"], options = opts)
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
if env["PLATFORM"] == "darwin":
    env.AppendUnique(LIBPATH = ["/sw/lib"])
    if env["GL_IMPL_MAC"] == "mesa":
        env.AppendUnique(LIBPATH = ["/sw/lib/mesa"], CPPPATH = ["/sw/include/mesa"])
    if env["GL_IMPL_MAC"] == "apple":
        env.AppendUnique(FRAMEWORKS = ["OpenGL"], CPPPATH = ["/System/Library/Frameworks/OpenGL.framework/Headers"])

conf = env.Configure(custom_tests = env["config_checks"])
conf.CheckBoost("regex", "1.20") or Exit(1)
conf.Finish()
namegen_env = env.Clone()
conf = env.Configure(custom_tests = env["config_checks"])
conf.CheckOpenGL(["gl", "glu"]) and \
conf.CheckSDL(require_version = "1.2.10") and \
conf.CheckSDL("SDL_image") and \
conf.CheckSDL("SDL_ttf") or Exit(1)
conf.Finish()

editor_env = env.Clone()
editor_env.Replace(QT4_UICDECLSUFFIX=".hpp", QT4_UICDECLPREFIX="", QT4_MOCIMPLPREFIX="", QT4_MOCIMPLSUFFIX=".moc")
editor_conf = editor_env.Configure(custom_tests = editor_env["config_checks"])
editor_env["HaveQt"] = editor_conf.CheckQt4Tools() and \
                       editor_conf.CheckQt4Libs(["QtCore", "QtGui", "QtOpenGL"])
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

namegen = namegen_env.Program("utilities/names/namegen", "utilities/names/namegen.cpp")
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

executables = [silvertree]
if editor_env["HaveQt"]:
    executables.append(editor)
datafiles = Split("data images model-sources FreeSans.ttf") + map(glob, Split("*.dae *.3ds *.3d"))

data_install_dir = join(env["PREFIX"], "share/silvertree-rpg")
data_stage_dir = join(env["STAGE_PREFIX"], "share/silvertree-rpg")
install_executables = Install(join(env["STAGE_PREFIX"], "bin"), executables)
install_data = Install(data_stage_dir, datafiles)
if not env["PLATFORM"] == "win32":
    Alias("install", [install_executables, install_data])
    env.Replace(CPPDEFINES = { "HAVE_CONFIG_H" : None, "DATADIR" : '\\"' + data_install_dir + '\\"' })
    editor_env.Replace(CPPDEFINES = { "HAVE_CONFIG_H" : None, "DATADIR" : '\\"' + data_install_dir + '\\"' })

bindistdir = env.Install("silvertree", Split("README LICENSE") + executables + datafiles)
bindistzip = env.Zip("silvertree.zip", bindistdir)
env.Alias("bindistzip", bindistzip)

setup_build_output([env, editor_env, namegen_env])

SConsignFile("sconsign")
