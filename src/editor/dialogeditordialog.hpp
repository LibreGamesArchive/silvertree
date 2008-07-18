#ifndef DIALOGEDITORDIALOG_HPP_INCLUDED
#define DIALOGEDITORDIALOG_HPP_INCLUDED

#include <QtGui/QDialog>

#include "ui_dialogeditordialog.hpp"
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
