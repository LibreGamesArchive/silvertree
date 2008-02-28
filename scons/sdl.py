# vi: syntax=python:et:ts=4
from os.path import join
from SCons.Script import *

def CheckSDL(context, sdl_lib = "SDL", require_version = None):
    if require_version:
        version = require_version.split(".", 2)
        major_version = int(version[0])
        minor_version = int(version[1])
        try:
            patchlevel    = int(version[2])
        except (ValueError, IndexError):
            patch_level = 0

    if sdl_lib == "SDL": 
        if require_version:
            context.Message("Checking for Simple DirectMedia Layer library version >= %d.%d.%d... " % (major_version, minor_version, patchlevel))
        else:
            context.Message("Checking for Simple DirectMedia Layer library... ")
        env = context.env
        env.AppendUnique(CPPPATH = [join(env["SDLDIR"], "include/SDL")], LIBPATH = [join(env["SDLDIR"], "lib")])
        if env["PLATFORM"] == "posix" or env["PLATFORM"] == "darwin":
            env.ParseConfig("sdl-config --cflags --libs")
        if env["PLATFORM"] == "win32":
            env.AppendUnique(CCFLAGS = ["-D_GNU_SOURCE"])
            env.AppendUnique(LIBS = Split("mingw32 SDLmain SDL SDL_image SDL_ttf"))
            env.AppendUnique(LINKFLAGS = ["-mwindows"])
    else:
        if require_version:
            context.Message("Checking for %s library version >= %d.%d.%d... " % (sdl_lib, major_version, minor_version, patchlevel))
        else:
            context.Message("Checking for %s library... " % sdl_lib)
        context.env.AppendUnique(LIBS = [sdl_lib])
    test_program = """
        #include <%s.h> 
        \n""" % sdl_lib
    if require_version:
        test_program += "#if SDL_VERSIONNUM(%s, %s, %s) < SDL_VERSIONNUM(%d, %d, %d)\n#error Library is too old!\n#endif\n" % \
            (sdl_lib.upper() + "_MAJOR_VERSION", \
             sdl_lib.upper() + "_MINOR_VERSION", \
             sdl_lib.upper() + "_PATCHLEVEL", \
             major_version, minor_version, patchlevel)
    test_program += """
        int main(int argc, char** argv)
        {
        }
        \n"""
    if context.TryLink(test_program, ".c"):
        context.Result("yes")
        return True
    else:
        context.Result("no")
        return False

def get_checks():
    return { "CheckSDL" : CheckSDL }
