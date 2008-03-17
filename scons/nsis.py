# vi: syntax=python:et:ts=4 

from SCons.Script  import *
from SCons.Builder import Builder
from SCons.Action  import Action

from os.path import basename, isdir

def generate_nsis_script(target, source, env):
    env["INSTALLER_FILES"] = Flatten(env["INSTALLER_FILES"])
    files = map(lambda src : "File /r /x .* " + str(src), env["INSTALLER_FILES"])
    uninstalls = map(lambda src : "Delete $INSTDIR\\" + basename(str(src)), env["INSTALLER_FILES"])
    uninstalls += map(lambda src : "RMDir /r $INSTDIR\\" + basename(str(src)), env["INSTALLER_FILES"])
    template = source[0].get_contents()
    script = template % {
                    "name" : env["INSTALLER_NAME"],
                    "version" : env["INSTALLER_VERSION"],
                    "executable" : basename(str(env["INSTALLER_SHORTCUTS"][0])),
                    "files" : "\n".join(files),
                    "uninst_files" : "\n".join(uninstalls)
                    }
    output = open(target[0].name, "wb")
    output.write(script)
    output.close()

def nsis_script_emitter(target, source, env):
    env.Depends(target, Flatten(env["INSTALLER_FILES"]))
    return (target, source)

nsis_script_generator = Builder(
                src_suffix="nsi.template",
                suffix=".nsi",
                emitter=nsis_script_emitter,
                action = Action(generate_nsis_script, "$NSISSCRIPTCOMSTR"),
                single_source = True
                )

installer_builder = Builder(
                src_suffix=".nsi",
                suffix=".exe",
                action = Action("$MAKENSIS $SOURCE", "$MAKENSISCOMSTR"),
                single_source = True,
                src_builder = nsis_script_generator
                )

def CheckMakeNSIS(context):
    env = context.env
    context.Message("Checking for makensis... ") 
    env["MAKENSIS"] = WhereIs("makensis")
    if env["MAKENSIS"]:
        have_makensis = context.TryAction("$MAKENSIS")
    else:
        have_makensis = False
    if have_makensis:
        env.Append(BUILDERS = { "Installer" : installer_builder } )
        env.Append(BUILDERS = { "NSISScript" : nsis_script_generator } )
        context.Result("yes")
    else:
        context.Result("no")
    return have_makensis

def get_checks():
    return { "CheckMakeNSIS" : CheckMakeNSIS }
