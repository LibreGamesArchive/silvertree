# vi: syntax=python:et:ts=4

from os.path import join
from glob import glob
import sys

EnsureSConsVersion(0, 98, 0)
sys.path.insert(0, "./scons")
from build_output import setup_build_output
from cross_compile import setup_cross_compile

opts = Options("options.cache")
opts.AddOptions(
    ("PREFIX", "Install prefix.", "/usr/local"),
    ("STAGE_PREFIX", "Install stage prefix.", "$PREFIX"),
    ("QT4DIR", "Root directory of Qt's installation.", ""),
    BoolOption("use_frameworked_qt", "Use Qt frameworks on Mac", False),
    ("SDLDIR", "Root directory of SDL's installation.", "/usr/"),
    ("BOOSTDIR", "Root directory of boost installation.", "/usr/include"),
    ("BOOSTLIBS", "Directory where boost libs are located.", "/usr/lib"),
    ("BOOST_SUFFIX", "Suffix of the boost regex library.", ""),
    ("GTKDIR", "Directory where GTK SDK is located.", ""),
    BoolOption("use_pango", "Use experimental pango based text renderer.", True),
    BoolOption("AUDIO", "Whether sound support is enabled", False),
    EnumOption("Build", "Build variant: debug or release", "release", ["debug", "release"], ignorecase=1),
    ("EXTRA_FLAGS_RELEASE", "Extra compiler/linker flags to use in release build variant.(e.g. \"-O3 -march=prescott\")", ""),
    ("EXTRA_FLAGS_DEBUG", "Extra compiler/linker flags to use in debug build variant.", ""),
    BoolOption("VERBOSE_BUILD_OUTPUT", "If true, SCons will display full command lines of commands it's running.", False),
    ("HOST", "Cross-compile host.", ""),
    BoolOption("STRICT", "Strict compilation (warnings are errors, etc..).", False)
)

env = Environment(tools = ["zip", "config_checks"], toolpath = ["scons"], options = opts)
env["Build"] = env["Build"].lower()
if env["PLATFORM"] == "win32":
    env.Tool("mingw")
else:
    env.Tool("default")
setup_cross_compile(env)
opts.Save("options.cache", env)

Help("""
Type "scons" to build the game.
"scons editor"         - builds the editor.
"scons namegen"        - namegen.
"scons version_finder" - version finder.

Additional options:
""")
Help(opts.GenerateHelpText(env))
if GetOption("help") : Return()

if env["PLATFORM"] == "win32": openal_lib = "openal32"
else: openal_lib = "openal"

env.Append(LIBPATH = "/usr/X11R6/lib")
conf = env.Configure(custom_tests = env["config_checks"])
conf.CheckBoost("regex", "1.20") or Exit(1)
conf.Finish()
namegen_env = env.Clone()
conf = env.Configure(custom_tests = env["config_checks"])
conf.CheckBoost("program_options") and \
conf.CheckGLEW() and \
conf.CheckSDL(require_version = "1.2.9") and \
conf.CheckSDL("SDL_image") or Return()
if env["use_pango"]:
    env["use_pango"] = conf.CheckPango("ft2")
if not env["use_pango"]:
    conf.CheckSDL("SDL_ttf") or Return()
if env["AUDIO"]:
    env["AUDIO"] = conf.CheckLibWithHeader(openal_lib, "AL/al.h", "C") and \
                   conf.CheckLibWithHeader("mpg123", "mpg123.h", "C")
HaveNSIS = conf.CheckMakeNSIS()
conf.Finish()

editor_env = env.Clone()
editor_env.Replace(QT4_UICDECLSUFFIX=".hpp", QT4_UICDECLPREFIX="", QT4_MOCIMPLPREFIX="", QT4_MOCIMPLSUFFIX=".moc")
editor_conf = editor_env.Configure(custom_tests = editor_env["config_checks"])
editor_env["HaveQt"] = editor_conf.CheckQt4Tools() and \
                       editor_conf.CheckQt4Libs(["QtCore", "QtGui", "QtOpenGL"])
editor_conf.Finish()
if "-mms-bitfields" in editor_env["CCFLAGS"]:
    editor_env["CCFLAGS"].remove("-mms-bitfields")

if env["AUDIO"]:
    env.Replace(CPPDEFINES = { "AUDIO" : None })
else:
    env.Replace(CPPDEFINES = dict())

if "gcc" in env["TOOLS"]:
    ccflags = Split("-Wall -Wno-sign-compare -Wno-switch -Wno-switch-enum")
    if env["STRICT"]:
        ccflags += Split("-Werror -ansi")
    linkflags = []
    if env["Build"] == "release":
        ccflags.append("-O2")
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
if env["HOST"]: build = env["Build"] + "-" + env["HOST"]
else: build = env["Build"]
silvertree, editor = SConscript("src/SConscript", build_dir = join("build", build))

if (not env["HOST"] or env["HOST"] == "mingw32") and env["Build"] == "release":
    ExecutableSuffix = env["PROGSUFFIX"]
else:
    ExecutableSuffix = "-" + build + env["PROGSUFFIX"]
env.Default(env.Alias("silvertreerpg", env.InstallAs("./silvertreerpg" + ExecutableSuffix, silvertree)))
if editor:
    editor_env.Alias("editor", editor_env.InstallAs("./silvertreeedit" + ExecutableSuffix, editor))

executables = [silvertree]
if editor_env["HaveQt"]:
    executables.append(editor)
executables += glob("*.dll")
datafiles = map(Dir, Split("data images model-sources music"))
datafiles += ["FreeSans.ttf"] + map(glob, Split("*.dae *.3ds *.3d"))

data_install_dir = join(env["PREFIX"], "share/silvertree-rpg")
data_stage_dir = join(env["STAGE_PREFIX"], "share/silvertree-rpg")
install_executables = Install(join(env["STAGE_PREFIX"], "bin"), executables)
install_data = Install(data_stage_dir, datafiles)
if not env["PLATFORM"] == "win32":
    Alias("install", [install_executables, install_data])
    env.AppendUnique(CPPDEFINES = { "DATADIR" : '\\"' + data_install_dir + '\\"' })
    editor_env.Replace(CPPDEFINES = { "DATADIR" : '\\"' + data_install_dir + '\\"' })

bindistdir = env.Install("silvertree", Split("README LICENSE") + executables + datafiles)
bindistzip = env.Zip("silvertree.zip", bindistdir)
env.Alias("bindistzip", bindistzip)

if HaveNSIS:
    env["INSTALLER_FILES"] = datafiles + executables + glob("*.dll")
    env["INSTALLER_NAME"] = "SilverTree"
    env["INSTALLER_VERSION"] = "0.2.1"
    env["INSTALLER_SHORTCUTS"] = silvertree
    installer = env.Installer(env["INSTALLER_NAME"] + "-" + env["INSTALLER_VERSION"], "installer.nsi.template")
    env.Alias("installer", installer)

setup_build_output([env, editor_env, namegen_env])

SConsignFile("sconsign")

if not GetOption("silent"):
    print env.subst("""
Build variant:          $Build
Text rendering backend: ${use_pango and 'pango' or 'sdl-ttf'}
Audio support: ${ AUDIO and 'enabled' or 'disabled' }
""")
