# Dependencies #

  * [Minimalist GNU for Windows](http://downloads.sourceforge.net/mingw/MinGW-5.1.3.exe).
  * [Python](http://www.python.org/ftp/python/2.5.1/python-2.5.1.msi)(needed by SCons).
  * [SCons](http://downloads.sourceforge.net/scons/scons-0.97.win32.exe?modtime=1179416442&big_mirror=0).
  * [Qt4](http://trolltech.com/developer/downloads/qt/windows)(needed by SilverTree editor).

These are provided by [SilverTree SDK](http://files.openomy.com/public/loonycyborg/silvertree_SDK-r2.zip):

  * SDL
  * SDL\_image
  * SDL\_ttf
  * boost regex
  * boost program\_options

# Compiling SilverTree #

  * Install MinGW; add C:\path\to\mingw\bin to _PATH_ environment variable
  * Install Python; add C:\path\to\python to _PATH_ environment variable
  * Install SCons
  * Check out a working copy as detailed [elsewhere](http://code.google.com/p/silvertree/source).
  * Unpack SilverTree SDK into working copy root.
  * Run command prompt, cd to working copy root and run _scons_.

## Compiling SilverTree editor ##
  * Install Qt.
  * run _scons QT4DIR=C:\path\to\qt editor_.