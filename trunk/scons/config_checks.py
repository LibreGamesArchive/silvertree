# vi: syntax=python:et:ts=4
from SCons.Script import *

checks = Split("""
    gl
    sdl
    boost
    qt4ng
""")

def exists(env):
    return True

def generate(env):
    modules = map(__import__, checks)
    config_checks = {}
    for module in modules:
        config_checks.update(module.get_checks())
    env["config_checks"] = config_checks
