#include "editpartydialog.moc"
#include "editpartydialog.hpp"
#include "../filesystem.hpp"
#include "../formatter.hpp"
#include "../wml_parser.hpp"
#include "../wml_utils.hpp"

#include <iostream>

namespace {
wml::const_node_ptr generators_node;
std::vector<wml::const_node_ptr> generators;
}

EditPartyDialog::EditPartyDialog(wml::node_ptr party, QWidget* parent)
  : QDialog(parent), party_(party)
{
	if(!generators_node) {
		generators_node = wml::parse_wml(sys::read_file("data/character_generators.cfg"));
		if(generators_node) {
			generators = wml::child_nodes(generators_node, "character");
		}
	}

	ui_.setupUi(this);

	foreach(const wml::const_node_ptr& gen, generators) {
		ui_.characterGeneratorComboBox->addItem(QString((*gen)["description"].c_str()));
	}
	
	wml::node_vector members = wml::child_nodes(party_, "character");
	foreach(wml::node_ptr member, members) {
		ui_.membersListWidget->addItem(QString((*member)["description"].c_str()));
	}

	ui_.membersListWidget->setCurrentRow(0);

	if((*party)["allegiance"] == "good") {
		ui_.allegianceComboBox->setCurrentIndex(1);
	} else {
		ui_.allegianceComboBox->setCurrentIndex(0);
	}

	ui_.moneyLineEdit->setText(QString((*party_)["money"].c_str()));

	QApplication::connect(ui_.addButton, SIGNAL(pressed()), this, SLOT(addMember()));
	QApplication::connect(ui_.removeButton, SIGNAL(pressed()), this, SLOT(removeMember()));
	QApplication::connect(ui_.membersListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(memberChanged(int)));
	QApplication::connect(ui_.characterGeneratorComboBox, SIGNAL(activated(int)), this, SLOT(changeGenerator(int)));

	if(members.empty()) {
		addMember();
	}
}

void EditPartyDialog::writeParty()
{
	switch(ui_.allegianceComboBox->currentIndex()) {
	case 0:
		party_->set_attr("allegiance","evil");
		party_->set_attr("aggressive","yes");
		break;
	case 1:
		party_->set_attr("allegiance","good");
		party_->set_attr("aggressive","no");
		break;
	}

	wml::const_node_ptr c = party_->get_child("character");
	if(c) {
		party_->set_attr("image", (*c)["image"]);
	}

	party_->set_attr("money", ui_.moneyLineEdit->text().toAscii().data());
}

void EditPartyDialog::addMember()
{
	if(generators.empty()) {
		std::cerr << "no generators!\n";
		return;
	}

	wml::node_ptr c = wml::deep_copy(generators.front());
	party_->add_child(c);
	ui_.membersListWidget->addItem(QString((*c)["description"].c_str()));
	ui_.membersListWidget->setCurrentRow(ui_.membersListWidget->count()-1);
}

void EditPartyDialog::removeMember()
{
	wml::node_vector members = wml::child_nodes(party_, "character");
	const int row = ui_.membersListWidget->currentRow();
	if(row >= 0 && row < members.size()) {
		party_->erase_child(members[row]);
		delete ui_.membersListWidget->takeItem(row);
	}

	if(!party_->get_child("character")) {
		done(0);
	}
}

void EditPartyDialog::memberChanged(int row)
{
	wml::node_vector members = wml::child_nodes(party_, "character");
	std::cerr << "selection: " << row << "\n";
	if(row >= 0 && row < members.size()) {
		const std::string gen = (*members[row])["generator"];
		if(!gen.empty()) {
			int index = 0;
			foreach(const wml::const_node_ptr& node, generators) {
				if((*node)["id"] == gen) {
					ui_.characterGeneratorComboBox->setCurrentIndex(index);
					break;
				}
				++index;
			}
		}
	}
}

void EditPartyDialog::changeGenerator(int index)
{
	std::cerr << "generator: " << index << "\n";
	wml::node_vector members = wml::child_nodes(party_, "character");
	const int row = ui_.membersListWidget->currentRow();
	if(row >= 0 && row < members.size()) {
		wml::const_node_ptr generator = generators[index];
		wml::node_ptr member = members[row];
		member->clear_attr();
		member->clear_children();
		for(wml::node::const_attr_iterator i = generator->begin_attr();
		    i != generator->end_attr(); ++i) {
			member->set_attr(i->first,i->second);
		}

		for(wml::node::const_all_child_iterator i = generator->begin_children();
		    i != generator->end_children(); ++i) {
			member->add_child(wml::deep_copy(*i));
		}

		assert(ui_.membersListWidget->currentItem());
		ui_.membersListWidget->currentItem()->setText((*member)["description"].c_str());
	}
}
