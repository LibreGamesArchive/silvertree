# vi: syntax=python:et:ts=4
import os
from os.path import join
from os import environ
from SCons.Util import AppendPath

def CheckPango(context, backend, require_version = None):
    if require_version:
        version_string = " >= " + require_version
    else:
        version_string = ""
    context.Message("Checking for Pango" + version_string + " with " + backend + " backend... ")
    version_string = version_string.replace(">=", "'>='")
    env = context.env
    gtkdir = env.get("GTKDIR", os.environ.get("GTK_BASEPATH"))
    if gtkdir:
        environ["PATH"] = AppendPath(environ["PATH"], join(gtkdir, "bin"))
        environ["PKG_CONFIG_PATH"] = AppendPath(environ.get("PKG_CONFIG_PATH", ""), join(gtkdir, "lib/pkgconfig"))

    try:
        env.ParseConfig("pkg-config --libs --cflags pango" + backend + version_string)
        context.Result("yes")
        return True
    except OSError:
        context.Result("no")
        return False

def get_checks():
    return { "CheckPango" : CheckPango }
