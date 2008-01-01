# vi: syntax=python:et:ts=4

def setup_build_output(envs):
    for env in envs:
        if not env["VERBOSE_BUILD_OUTPUT"]:
            env["CCCOMSTR"]     = "Compiling C source ${SOURCE.srcpath} ..."
            env["CXXCOMSTR"]    = "Compiling C++ source ${SOURCE.srcpath} ..."
            env["ARCOMSTR"]     = "Creating static library ${TARGET.srcpath} ..."
            env["RANLIBCOMSTR"] = "Indexing ${TARGET.srcpath} ..."
            env["LINKCOMSTR"]   = "Linking binary ${TARGET.srcpath} ..."
            env["INSTALLSTR"]   = "Installing $SOURCE as $TARGET ..."

            env["QT4_MOCCOMSTR"] = "Moccing ${SOURCE.srcpath} ..."
            env["QT4_UICCOMSTR"]      = "Compiling user interface: ${SOURCE.srcpath} ..."
