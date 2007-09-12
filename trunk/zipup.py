#!/usr/bin/env python

import os, glob, pysvn, shutil

def get_files():
    """Find all SVN files to include in the release."""
    client = pysvn.Client()
    files = client.status(".", recurse = True, get_all = True)

    return [f.path for f in files if f.is_versioned and f.entry and
        f.entry.kind == pysvn.node_kind.file]

revision = os.popen("svnversion -n").read().strip("M")
releasename = "silvertree" + revision
distpath = os.path.join("dist", releasename)

shutil.rmtree(distpath, True)

shutil.rmtree("dist", True)
os.mkdir("dist")

files = get_files()

files += glob.glob("*.exe")
files += glob.glob("*.dll")
#files.remove("QtCore4.dll")
#files.remove("QtGui4.dll")
#files.remove("QtOpenGL4.dll")

for f in files:
    target = os.path.join(distpath, f)
    targetdir = os.path.dirname(target)
    try: os.makedirs(targetdir)
    except OSError: pass

    shutil.copy2(f, targetdir)

os.chdir("dist")
os.system("zip -r '%s.zip' %s > /dev/null" % (releasename, releasename))

print """
All SVN files (revision %(revision)s) and all .exe and .dll files in the
current directory have been packed into dist/%(releasename)s.zip.
""".strip() % locals()
