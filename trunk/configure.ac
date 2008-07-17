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

AC_ARG_ENABLE([audio], 
              AC_HELP_STRING([--enable-audio], [enable sound (default is yes)]), 
              [want_audio=$enableval], [want_audio=check])
AC_ARG_ENABLE([pango],
              AC_HELP_STRING([--enable-pango], [use pango backend (default is no)]), 
              [want_pango=$enableval], [want_pango=no])
  
AC_ARG_ENABLE([formula_test],
              AC_HELP_STRING([--enable-formula-test] [unit test formula code on startup (default is no)]),
              [want_formula_test=$enableval], [want_formula_test=no])
            
CXXFLAGS="-I.. $CXXFLAGS"

min_sdl_version=1.2.9
AM_PATH_SDL($min_sdl_version, 
	    [LIBS="$SDL_LIBS $LIBS" CXXFLAGS="$SDL_CFLAGS $CXXFLAGS"],
	    AC_MSG_ERROR([**** SDL version $SDL_VERSION not found !]))
AC_CHECK_LIB([SDL_ttf], [TTF_Init], [LIBS="-lSDL_ttf $LIBS"],
             AC_MSG_ERROR([**** SDL_ttf not found !]))
AC_CHECK_LIB([SDL_image], [IMG_Load], [LIBS="-lSDL_image $LIBS"],
             AC_MSG_ERROR([**** SDL_image not found !]))


AS_IF([test x$want_audio = xno], 
      [have_audio=no],
      [have_audio=yes
       AC_CHECK_LIB([mpg123], [mpg123_init], [AUDIO_LIBS="-lmpg123 $AUDIO_LIBS"],
       [AC_MSG_WARN([**** mpg123 not found !])
                    have_audio=no])
       AC_CHECK_LIB([openal], [alcGetIntegerv], [AUDIO_LIBS="-lopenal $AUDIO_LIBS"],
       [AC_MSG_WARN([**** OpenAL not found !])
                    have_audio=no])
])

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

AC_CHECK_PROG([pkgconfig_available], [pkg-config], [yes], [no])

AS_IF([test x$want_pango = xno], 
      [have_pango=no],
      [AS_IF([test "x$pkgconfig_available" = xyes],
        [PKG_CHECK_MODULES(PANGO, pangoft2 >= 1.14.8, [
        CPPFLAGS="$CPPFLAGS $PANGO_CFLAGS"
        AC_DEFINE([USE_PANGO], 1, [Pango backend enabled.])
        LIBS="$PANGO_LIBS $LIBS"
        have_pango=yes
        ], [
         AC_MSG_WARN([**** Pango with freetype2 backend not found !])
        ])
     ])])

BNV_HAVE_QT
AS_IF([test x$have_qt = xno], 
      [
              AC_MSG_WARN([**** Qt library not found.])
      ],
      [
        AC_CHECK_LIB([QtGui], [main], [QT_LIBS="$QT_LIBS -lQtGui"],
	             [AC_MSG_WARN([**** QtGui not found !])
		      have_qt=no], [$QT_LIBS])
        AC_CHECK_LIB([QtOpenGL], [main], [QT_LIBS="$QT_LIBS -lQtOpenGL"],
	             [AC_MSG_WARN([**** QtOpenGL not found !])
                      have_qt=no], [$QT_LIBS])
      ])

AS_IF([test x$want_audio = xno], [],
AS_IF([test x$have_audio = xno],
      AS_IF([test x$want_audio = xyes],
            AC_MSG_ERROR([**** Audio libraries not found.]),
            AC_MSG_WARN([**** Audio libraries not found - no sound.])),
      [AC_DEFINE([AUDIO], 1, [Audio enabled])
       LIBS="$AUDIO_LIBS $LIBS"]
))

AM_CONDITIONAL([HAVE_PANGO], [test x$have_pango = xyes])

AM_CONDITIONAL([WANT_AUDIO], [test x$have_audio = xyes])

AS_IF([test x$have_qt = xno],
     AC_MSG_WARN([**** Editor will not be built.])
)

AS_IF([test x$want_formula_test = xyes],
      [AC_DEFINE([UNIT_TEST_FORMULA], 1, [Formula unit testing enabled in formula.cpp])])

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
