#include "editpartydialog.moc"
#include "editpartydialog.hpp"
#include "../wml_utils.hpp"

EditPartyDialog::EditPartyDialog(wml::node_ptr party, QWidget* parent)
  : QDialog(parent), party_(party)
{
	ui_.setupUi(this);
	
	WML_MUTABLE_FOREACH(member, party, "character") {
		ui_.membersListWidget->addItem(QString((*member)["description"].c_str()));
	}
}
