AC_PREREQ([2.61])
AC_INIT([silvertree-rpg], [0.2], [dave@whitevine.net])
AC_LANG([C++])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

min_sdl_version=1.2.10
AM_PATH_SDL($min_sdl_version, 
	    [LIBS="$SDL_LIBS $LIBS" CXXFLAGS="$SDL_CFLAGS $CXXFLAGS"],
	    AC_MSG_ERROR([**** SDL version $SDL_VERSION not found !]))
AC_CHECK_LIB([SDL_ttf], [TTF_Init], [LIBS="-lSDL_ttf $LIBS"],
             AC_MSG_ERROR([**** SDL_ttf not found !]))
AC_CHECK_LIB([SDL_image], [IMG_Load], [LIBS="-lSDL_image $LIBS"],
             AC_MSG_ERROR([**** SDL_image not found !]))

AX_BOOST_BASE
AX_BOOST_REGEX

CPPFLAGS="$BOOST_CPPFLAGS $CPPFLAGS"
LDFLAGS="$BOOST_LDFLAGS $LDFLAGS"
LIBS="$BOOST_REGEX_LIB $LIBS"

AX_CHECK_GL
AS_IF([test x$no_gl = xyes],
      AC_MSG_ERROR([**** OpenGL libraries not found!]))
AX_CHECK_GLU
AS_IF([test x$no_glu = xyes],
      AC_MSG_ERROR([**** GLU libraries not found!]))

AC_CHECK_LIB([GLEW], [glewInit], [LIBS="-lGLEW $LIBS"],
             AC_MSG_ERROR([**** GLEW not found !]))

CXXFLAGS="$GL_CFLAGS $GLU_CFLAGS $CXXFLAGS"
LIBS="$GL_LIBS $GLU_LIBS $LIBS"

BNV_HAVE_QT
AS_IF([test x$have_qt = xno], 
      [
        AC_MSG_WARN([**** Qt library not found.])
      ],
      [
        AC_CHECK_LIB([QtGui], [main], [QT_LIBS="$QT_LIBS -lQtGui"],
	             [AC_MSG_WARN([**** QtGui not found !])
		      have_qt=no])
        AC_CHECK_LIB([QtOpenGL], [main], [QT_LIBS="$QT_LIBS -lQtOpenGL"],
	             [AC_MSG_WARN([**** QtOpenGL not found !])
                      have_qt=no])
      ]
)

AS_IF([test x$have_qt = xno],
     AC_MSG_WARN([**** Editor will not be built.])
)

AM_CONDITIONAL([WANT_EDITOR], [test x$have_qt = xyes])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
Makefile
src/Makefile
src/editor/Makefile 
utilities/Makefile 
utilities/names/Makefile 
utilities/versions/Makefile
])


AC_OUTPUT
