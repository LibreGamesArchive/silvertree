# vi: syntax=python:et:ts=4

def CheckBoostLib(context, boost_lib, require_version = None):
    env = context.env
    backup = env.Clone().Dictionary()
    env.AppendUnique(CPPPATH = [env["BOOSTDIR"]], LIBPATH = [env["BOOSTLIBS"]])

    boost_headers = {
        "regex" : "regex/config.hpp",
        "program_options" : "program_options/config.hpp"
        }
    header_name = boost_headers.get(boost_lib, boost_lib + ".hpp")
    libname = "boost_" + boost_lib + env.get("BOOST_SUFFIX", "")

    env.AppendUnique(CPPPATH = [env["BOOSTDIR"]], LIBPATH = [env["BOOSTLIBS"]])
    env.AppendUnique(LIBS = [libname])

    test_program = """
        #include <boost/%s>
        \n""" % header_name
    if require_version:
        version = require_version.split(".", 2)
        major = int(version[0])
        minor = int(version[1])
        try:
            sub_minor = int(version[2])
        except (ValueError, IndexError):
            sub_minor = 0
        test_program += "#include <boost/version.hpp>\n"
        test_program += \
            "#if BOOST_VERSION < %d\n#error Boost version is too old!\n#endif\n" \
            % (major * 100000 + minor * 100 + sub_minor)
    test_program += """
        int main()
        {
        }
        \n"""
    if context.TryLink(test_program, ".cpp"):
        return True
    else:
        env.Replace(**backup)
        return False

def CheckBoost(context, boost_lib, require_version = None):
    if require_version:
        context.Message("Checking for Boost %s library version >= %s... " % (boost_lib, require_version))
    else:
        context.Message("Checking for Boost %s library... " % boost_lib)
    check_result = CheckBoostLib(context, boost_lib, require_version)
    if not check_result and not context.env["BOOST_SUFFIX"]:
        context.env["BOOST_SUFFIX"] = "-mt"
        check_result = CheckBoostLib(context, boost_lib, require_version)
    if check_result:
        context.Result("yes")
    else:
        context.Result("no")
    return check_result

def get_checks():
    return { "CheckBoost" : CheckBoost }
