#ifndef EDITWMLDIALOG_HPP_INCLUDED
#define EDITWMLDIALOG_HPP_INCLUDED

#include <QtGui/QDialog>

#include "ui_editwml.hpp"
#include "../wml_node.hpp"

class EditWmlDialog : public QDialog
{
	Q_OBJECT

public:
	EditWmlDialog(wml::node_ptr node);

public slots:
	void okPressed();

private:
	Ui::EditWmlDialog ui_;
	wml::node_ptr node_;
};

#endif
