#include <QtGui/QMessageBox>

#include "editwmldialog.moc"
#include "editwmldialog.hpp"

#include "../wml_parser.hpp"
#include "../wml_utils.hpp"
#include "../wml_writer.hpp"

#include <iostream>

EditWmlDialog::EditWmlDialog(wml::node_ptr node)
  : node_(node)
{
	ui_.setupUi(this);

	std::string str;
	wml::write(node,str);
	ui_.wmlTextEdit->append(str.c_str());

	QApplication::connect(ui_.okButton, SIGNAL(pressed()), this, SLOT(okPressed()));
}

void EditWmlDialog::okPressed()
{
	wml::node_ptr node;
	try {
		node = wml::parse_wml(ui_.wmlTextEdit->document()->toPlainText().toAscii().data());
	} catch(wml::parse_error& e) {
		QMessageBox::warning(this, "Error in WML",
		                     QString("There was an error in your WML:\n") +
							 QString(e.message.c_str()));
		return;
	}

	wml::copy_over(node, node_);

	accept();
}
