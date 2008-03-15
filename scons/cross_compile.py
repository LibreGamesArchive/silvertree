# vi: syntax=python:et:ts=4

def setup_cross_compile(env):
    if env["HOST"] == "mingw32":
        env["PLATFORM"] = "win32"
        env["PROGSUFFIX"] = ".exe"

    tools = [
        "CC",
        "CXX",
        "AR",
        "RANLIB"
        ]
    for tool in tools:
        if env["HOST"]:
            env[tool] = env["HOST"] + "-" + env[tool]
