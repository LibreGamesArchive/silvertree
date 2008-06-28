# vi: syntax=python:et:ts=4
import sys

colors = {
    "blue"    : "\033[34m",
    "yellow"  : "\033[33m",
    "magenta" : "\033[35m",
    "green"   : "\033[32m",
    "bold"    : "\033[1m",
    "nocolor" : "\033[0m"
    }

def setup_build_output(envs):
    for env in envs:
        if not env["VERBOSE_BUILD_OUTPUT"]:
            if sys.platform != "win32":
                env.Replace(**colors)
            env["CCCOMSTR"]     = "${blue}Compiling C source $bold${SOURCE.srcpath}$nocolor ..."
            env["CXXCOMSTR"]    = "${blue}Compiling C++ source $bold${SOURCE.srcpath}$nocolor ..."
            env["ARCOMSTR"]     = "${yellow}Creating static library $bold${TARGET.srcpath}$nocolor ..."
            env["RANLIBCOMSTR"] = "${yellow}Indexing $bold${TARGET.srcpath}$nocolor ..."
            env["LINKCOMSTR"]   = "${magenta}Linking binary $bold${TARGET.srcpath}$nocolor ..."
            env["INSTALLSTR"]   = "Installing $bold$SOURCE$nocolor as $bold$TARGET$nocolor ..."

            env["QT4_MOCCOMSTR"] = "${green}Moccing $bold${SOURCE.srcpath}$nocolor ..."
            env["QT4_UICCOMSTR"] = "${green}Compiling user interface $bold${SOURCE.srcpath}$nocolor ..."

            env["NSISSCRIPTCOMSTR"] = "Generating NSIS script ..."
            env["MAKENSISCOMSTR"] = "Generating installer ..."
