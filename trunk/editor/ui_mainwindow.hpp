#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QScrollBar>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>
#include "editorglwidget.hpp"

class Ui_MainWindow
{
public:
    QAction *action_Quit;
    QAction *action_Open;
    QAction *action_Save;
    QAction *actionZoom_In;
    QAction *actionZoom_Out;
    QAction *actionRotate_Left;
    QAction *actionRotate_Right;
    QAction *actionUndo;
    QAction *actionRedo;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QGridLayout *gridLayout1;
    EditorGLWidget *editorGLWidget;
    QScrollBar *verticalScrollBar;
    QScrollBar *horizontalScrollBar;
    QMenuBar *menubar;
    QMenu *menuView;
    QMenu *menu_File;
    QMenu *menuEdit;
    QStatusBar *statusbar;
    QToolBar *toolBar;
    QToolBar *tilesToolBar;

    void setupUi(QMainWindow *MainWindow)
    {
    MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
    MainWindow->resize(QSize(977, 801).expandedTo(MainWindow->minimumSizeHint()));
    action_Quit = new QAction(MainWindow);
    action_Quit->setObjectName(QString::fromUtf8("action_Quit"));
    action_Open = new QAction(MainWindow);
    action_Open->setObjectName(QString::fromUtf8("action_Open"));
    action_Save = new QAction(MainWindow);
    action_Save->setObjectName(QString::fromUtf8("action_Save"));
    actionZoom_In = new QAction(MainWindow);
    actionZoom_In->setObjectName(QString::fromUtf8("actionZoom_In"));
    actionZoom_Out = new QAction(MainWindow);
    actionZoom_Out->setObjectName(QString::fromUtf8("actionZoom_Out"));
    actionRotate_Left = new QAction(MainWindow);
    actionRotate_Left->setObjectName(QString::fromUtf8("actionRotate_Left"));
    actionRotate_Right = new QAction(MainWindow);
    actionRotate_Right->setObjectName(QString::fromUtf8("actionRotate_Right"));
    actionUndo = new QAction(MainWindow);
    actionUndo->setObjectName(QString::fromUtf8("actionUndo"));
    actionRedo = new QAction(MainWindow);
    actionRedo->setObjectName(QString::fromUtf8("actionRedo"));
    centralwidget = new QWidget(MainWindow);
    centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
    gridLayout = new QGridLayout(centralwidget);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(9);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout1 = new QGridLayout();
    gridLayout1->setSpacing(6);
    gridLayout1->setMargin(0);
    gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
    editorGLWidget = new EditorGLWidget(centralwidget);
    editorGLWidget->setObjectName(QString::fromUtf8("editorGLWidget"));

    gridLayout1->addWidget(editorGLWidget, 0, 0, 1, 1);

    verticalScrollBar = new QScrollBar(centralwidget);
    verticalScrollBar->setObjectName(QString::fromUtf8("verticalScrollBar"));
    verticalScrollBar->setOrientation(Qt::Vertical);

    gridLayout1->addWidget(verticalScrollBar, 0, 1, 1, 1);

    horizontalScrollBar = new QScrollBar(centralwidget);
    horizontalScrollBar->setObjectName(QString::fromUtf8("horizontalScrollBar"));
    horizontalScrollBar->setOrientation(Qt::Horizontal);

    gridLayout1->addWidget(horizontalScrollBar, 1, 0, 1, 1);


    gridLayout->addLayout(gridLayout1, 0, 0, 1, 1);

    MainWindow->setCentralWidget(centralwidget);
    menubar = new QMenuBar(MainWindow);
    menubar->setObjectName(QString::fromUtf8("menubar"));
    menubar->setGeometry(QRect(0, 0, 977, 27));
    menuView = new QMenu(menubar);
    menuView->setObjectName(QString::fromUtf8("menuView"));
    menu_File = new QMenu(menubar);
    menu_File->setObjectName(QString::fromUtf8("menu_File"));
    menuEdit = new QMenu(menubar);
    menuEdit->setObjectName(QString::fromUtf8("menuEdit"));
    MainWindow->setMenuBar(menubar);
    statusbar = new QStatusBar(MainWindow);
    statusbar->setObjectName(QString::fromUtf8("statusbar"));
    statusbar->setGeometry(QRect(0, 779, 977, 22));
    MainWindow->setStatusBar(statusbar);
    toolBar = new QToolBar(MainWindow);
    toolBar->setObjectName(QString::fromUtf8("toolBar"));
    toolBar->setOrientation(Qt::Horizontal);
    MainWindow->addToolBar(static_cast<Qt::ToolBarArea>(4), toolBar);
    tilesToolBar = new QToolBar(MainWindow);
    tilesToolBar->setObjectName(QString::fromUtf8("tilesToolBar"));
    tilesToolBar->setOrientation(Qt::Vertical);
    MainWindow->addToolBar(static_cast<Qt::ToolBarArea>(2), tilesToolBar);

    menubar->addAction(menu_File->menuAction());
    menubar->addAction(menuEdit->menuAction());
    menubar->addAction(menuView->menuAction());
    menuView->addAction(actionZoom_In);
    menuView->addAction(actionZoom_Out);
    menu_File->addAction(action_Open);
    menu_File->addAction(action_Save);
    menu_File->addAction(action_Quit);
    menuEdit->addAction(actionUndo);
    menuEdit->addAction(actionRedo);
    toolBar->addAction(action_Open);
    toolBar->addAction(action_Save);
    toolBar->addSeparator();
    toolBar->addAction(actionZoom_In);
    toolBar->addAction(actionZoom_Out);
    toolBar->addSeparator();
    toolBar->addAction(actionRotate_Left);
    toolBar->addAction(actionRotate_Right);
    retranslateUi(MainWindow);

    QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
    MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
    action_Quit->setText(QApplication::translate("MainWindow", "&Quit", 0, QApplication::UnicodeUTF8));
    action_Open->setText(QApplication::translate("MainWindow", "&Open", 0, QApplication::UnicodeUTF8));
    action_Save->setText(QApplication::translate("MainWindow", "&Save", 0, QApplication::UnicodeUTF8));
    actionZoom_In->setText(QApplication::translate("MainWindow", "Zoom In", 0, QApplication::UnicodeUTF8));
    actionZoom_Out->setText(QApplication::translate("MainWindow", "Zoom Out", 0, QApplication::UnicodeUTF8));
    actionRotate_Left->setText(QApplication::translate("MainWindow", "Rotate Left", 0, QApplication::UnicodeUTF8));
    actionRotate_Right->setText(QApplication::translate("MainWindow", "Rotate Right", 0, QApplication::UnicodeUTF8));
    actionUndo->setText(QApplication::translate("MainWindow", "Undo", 0, QApplication::UnicodeUTF8));
    actionRedo->setText(QApplication::translate("MainWindow", "Redo", 0, QApplication::UnicodeUTF8));
    menuView->setTitle(QApplication::translate("MainWindow", "View", 0, QApplication::UnicodeUTF8));
    menu_File->setTitle(QApplication::translate("MainWindow", "&File", 0, QApplication::UnicodeUTF8));
    menuEdit->setTitle(QApplication::translate("MainWindow", "Edit", 0, QApplication::UnicodeUTF8));
    toolBar->setWindowTitle(QApplication::translate("MainWindow", "toolBar", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

#endif // MAINWINDOW_H
