#!/usr/bin/env python

import os, sys, glob, pysvn, shutil, optparse, StringIO

def get_files():
    """Find all SVN files to include in the release."""
    client = pysvn.Client()
    files = client.status(".", recurse = True, get_all = True)

    return [f.path for f in files if f.is_versioned and f.entry and
        f.entry.kind == pysvn.node_kind.file]

def main():
    global revision, releasename, distpath, options
    p = optparse.OptionParser()
    p.add_option("-u", "--upload", action = "store_true", help = "upload to the website (requires ftp account)")
    p.add_option("-n", "--nothing", action = "store_true", help = "do nothing (useful for testing)")
    options, args = p.parse_args()

    revision = os.popen("svnversion -n").read().strip("M")
    releasename = "silvertree-windows-" + revision
    distpath = os.path.join("dist", releasename)

    if not options.nothing: create_release()

    if options.upload:
        upload()

def create_release():
    shutil.rmtree(distpath, True)

    shutil.rmtree("dist", True)
    os.mkdir("dist")

    files = get_files()

    files += glob.glob("*.exe")
    #files += glob.glob("*.dll")
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
    os.chdir("..")
    
    print "The .zip has been created (%s.zip)." % releasename

def create_index(files):
    x = """
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><head>
<meta http-equiv="content-type" content="text/html; charset=utf-8"><title>Silver Tree Project</title>
<link rel="stylesheet" href="/jwb.css"></head><body>
<div align="center">
<h1>Silver Tree Project</h1>
<h2>Daily Windows SVN binaries</h2>
You also need to place the DLLs from silvertree-dlls.zip into the
unpacked folder of the snapshot.
<ul>
"""
    sorted = []
    for f in files:
        permissions, links, user, group, bytes, remainder = f.split(None, 5)
        filename = remainder.split()[-1]
        if filename == "index.html": continue
        date = " ".join(remainder.split()[:-1])
        sorted.append((filename,
            """<li>%s <a href="%s/%s">%s</a> (%.2f MB)</li>""" % (
            date, "/windows_daily", filename, filename,
            float(bytes) / 1024.0 / 1024.0)))
    sorted.sort()
    blah = sorted[0]
    sorted = sorted[1:]
    sorted.reverse()
    sorted = [blah] + sorted
    for s in sorted:
        x += s[1]
    x += """
</ul>
<a href="/">Back</a>
</div></body></html>"""
    return x, sorted[2:]

def upload():
    import ftplib
    user, passwd = [x.strip() for x in open("passwd").readlines()]
    print "Connecting.."
    ftp = ftplib.FTP("silvertreerpg.org", user, passwd)
    print "Uploading.."

    f = open("dist/%s.zip" % releasename, "rb")
    try: ftp.mkd("mainwebsite_html/windows_daily")
    except ftplib.error_perm: pass
    ftp.storbinary("STOR mainwebsite_html/windows_daily/%s.zip" % releasename, f)
    f.close()

    files = []
    def cb(x):
        files.append(x)
    ftp.dir("mainwebsite_html/windows_daily", cb)
    index, delete = create_index(files)
    for d in delete:
        ftp.delete("mainwebsite_html/windows_daily/%s" % d[0])
        print d[0], "has been removed."
    f = StringIO.StringIO(index)
    ftp.storbinary("STOR mainwebsite_html/windows_daily/index.html", f)
    f.close()

    ftp.close()
    
    print "The file has been uploaded."

if __name__ == "__main__": main()
