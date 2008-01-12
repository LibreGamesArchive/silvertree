# vi: syntax=python:et:ts=4

from config_check_utils import backup_env, restore_env

def CheckOpenGL(context, libs = ["gl"]):
    context.Message("Checking for OpenGL... ")
    env = context.env
    backup = backup_env(env, ["LIBS"])
    if env["PLATFORM"] == "win32":
        libnames = { "gl" : "opengl32", "glu" : "glu32" }
    else:
        libnames = { "gl" : "GL", "glu" : "GLU" }
    env.AppendUnique(LIBS = map(libnames.get, libs))
    test_program = ""
    for lib in libs:
        test_program += "#include <GL/%s.h>\n" % lib
    test_program += "int main()\n{}\n"
    if context.TryLink(test_program, ".c"):
        context.Result("yes")
        return True
    else:
        restore_env(env, backup)
        context.Result("no")
        return False

def CheckGLEW(context):
    context.Message("Checking for OpenGL Extension Wrangler... ")
    env = context.env
    backup = backup_env(env, ["LIBS"])
    if env["PLATFORM"] == "win32":
        env.AppendUnique(LIBS = ["glew32", "glu32", "opengl32"])
    else:
        env.AppendUnique(LIBS = ["GLEW", "GLU", "GL"])
    test_program = """
        #include <GL/glew.h>
        int main()
        {
            glewInit();
        }
"""
    if context.TryLink(test_program, ".c"):
        context.Result("yes")
        return True
    else:
        restore_env(env, backup)
        context.Result("no")
        return False

def get_checks():
    return { "CheckOpenGL" : CheckOpenGL, "CheckGLEW" : CheckGLEW }
