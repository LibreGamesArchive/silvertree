if WANT_EDITOR
bin_PROGRAMS = silvertree-editor
silvertree_editor_LDADD = ../libsilvertree.a
silvertree_editor_SOURCES = \
editorglwidget.cpp \
editormainwindow.cpp \
editpartydialog.cpp \
editwmldialog.cpp \
main.cpp \
terrainhandler.cpp \
ui_mainwindow.ui \
ui_editpartydialog.ui \
ui_editwml.ui \
editorglwidget.hpp \
editormainwindow.hpp \
terrainhandler.hpp \
editpartydialog.hpp \
editwmldialog.hpp

BUILT_SOURCES=\
ui_mainwindow.hpp \
ui_editpartydialog.hpp \
ui_editwml.hpp \
editorglwidget.moc \
editormainwindow.moc \
terrainhandler.moc \
editpartydialog.moc \
editwmldialog.moc

AM_CPPFLAGS = -DDATADIR=\"$(pkgdatadir)\" -DVERSION=\"$(VERSION)\"
AM_CXXFLAGS = @QT_CXXFLAGS@ -Wall -Werror -Wno-sign-compare -Wno-switch -Wno-switch-enum
LIBS += @QT_LIBS@

.ui.hpp:
	@QT_UIC@ $< > $@
.hpp.moc:
	@QT_MOC@ $< > $@

endif
