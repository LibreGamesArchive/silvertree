#ifndef DIALOGEDITORDIALOG_HPP_INCLUDED
#define DIALOGEDITORDIALOG_HPP_INCLUDED

#include <QtGui/QDialog>

#include "ui_dialogeditor.hpp"
#include "../wml_node_fwd.hpp"

class DialogEditorDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DialogEditorDialog(wml::node_ptr node);

private:
	Ui::DialogEditorDialog ui_;
	wml::node_ptr node_;
};

#endif

/********************************************************************************
** Form generated from reading ui file 'ui_dialogeditordialog.ui'
**
** Created: Wed Jul 16 21:05:43 2008
**      by: Qt User Interface Compiler version 4.4.0
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_DIALOGEDITORDIALOG_H
#define UI_DIALOGEDITORDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGraphicsView>
#include <QtGui/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_DialogEditorDialog
{
public:
    QDialogButtonBox *buttonBox;
    QGraphicsView *dialogGraphicsView;
    QPushButton *addButton;

    void setupUi(QDialog *DialogEditorDialog)
    {
    if (DialogEditorDialog->objectName().isEmpty())
        DialogEditorDialog->setObjectName(QString::fromUtf8("DialogEditorDialog"));
    DialogEditorDialog->resize(752, 591);
    buttonBox = new QDialogButtonBox(DialogEditorDialog);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(400, 550, 341, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);
    dialogGraphicsView = new QGraphicsView(DialogEditorDialog);
    dialogGraphicsView->setObjectName(QString::fromUtf8("dialogGraphicsView"));
    dialogGraphicsView->setGeometry(QRect(10, 10, 521, 571));
    addButton = new QPushButton(DialogEditorDialog);
    addButton->setObjectName(QString::fromUtf8("addButton"));
    addButton->setGeometry(QRect(550, 10, 75, 24));

    retranslateUi(DialogEditorDialog);
    QObject::connect(buttonBox, SIGNAL(accepted()), DialogEditorDialog, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), DialogEditorDialog, SLOT(reject()));

    QMetaObject::connectSlotsByName(DialogEditorDialog);
    } // setupUi

    void retranslateUi(QDialog *DialogEditorDialog)
    {
    DialogEditorDialog->setWindowTitle(QApplication::translate("DialogEditorDialog", "Dialog", 0, QApplication::UnicodeUTF8));
    addButton->setText(QApplication::translate("DialogEditorDialog", "Add", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(DialogEditorDialog);
    } // retranslateUi

};

namespace Ui {
    class DialogEditorDialog: public Ui_DialogEditorDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOGEDITORDIALOG_H
