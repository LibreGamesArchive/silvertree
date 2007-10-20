#include "editpartydialog.moc"
#include "editpartydialog.hpp"
#include "editwmldialog.hpp"
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
  : QDialog(parent), party_(party), backup_(wml::deep_copy(party))
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

	if(members.empty()) {
		addMember();
	}

	loadCharacterStats();

	QApplication::connect(ui_.okButton, SIGNAL(pressed()), this, SLOT(writeParty()));
	QApplication::connect(ui_.cancelButton, SIGNAL(pressed()), this, SLOT(revertparty()));
	QApplication::connect(ui_.addButton, SIGNAL(pressed()), this, SLOT(addMember()));
	QApplication::connect(ui_.removeButton, SIGNAL(pressed()), this, SLOT(removeMember()));
	QApplication::connect(ui_.editWMLButton, SIGNAL(pressed()), this, SLOT(editWml()));
	QApplication::connect(ui_.membersListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(memberChanged(int)));
	QApplication::connect(ui_.characterGeneratorComboBox, SIGNAL(activated(int)), this, SLOT(changeGenerator(int)));
	QApplication::connect(ui_.levelLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(changeStats(const QString&)));
	QApplication::connect(ui_.skillsLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(changeStats(const QString&)));
	QApplication::connect(ui_.equipmentLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(changeStats(const QString&)));
	QApplication::connect(ui_.strengthLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(changeStats(const QString&)));
	QApplication::connect(ui_.agilityLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(changeStats(const QString&)));
	QApplication::connect(ui_.enduranceLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(changeStats(const QString&)));
	QApplication::connect(ui_.intelligenceLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(changeStats(const QString&)));
	QApplication::connect(ui_.perceptionLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(changeStats(const QString&)));
	QApplication::connect(ui_.personaLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(changeStats(const QString&)));
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
	if(c && c->has_attr("image")) {
		party_->set_attr("image", (*c)["image"]);
	}

	party_->set_or_erase_attr("money", ui_.moneyLineEdit->text().toAscii().data());

	writeCharacterStats();
}

void EditPartyDialog::revertParty()
{
	wml::copy_over(backup_,party_);
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
	loadCharacterStats();
}

void EditPartyDialog::removeMember()
{
	wml::node_vector members = wml::child_nodes(party_, "character");
	const int row = ui_.membersListWidget->currentRow();
	if(row >= 0 && row < members.size()) {
		party_->erase_child(members[row]);
		delete ui_.membersListWidget->takeItem(row);
	}

	loadCharacterStats();
	if(!party_->get_child("character")) {
		done(0);
	}
}

void EditPartyDialog::memberChanged(int row)
{
	wml::node_vector members = wml::child_nodes(party_, "character");
	std::cerr << "selection: " << row << "\n";
	if(row >= 0 && row < members.size()) {

	}

	loadCharacterStats();
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
			member->set_or_erase_attr(i->first,i->second);
		}

		for(wml::node::const_all_child_iterator i = generator->begin_children();
		    i != generator->end_children(); ++i) {
			member->add_child(wml::deep_copy(*i));
		}

		assert(ui_.membersListWidget->currentItem());
		ui_.membersListWidget->currentItem()->setText((*member)["description"].c_str());
	}

	loadCharacterStats();
}

void EditPartyDialog::changeStats(const QString& text)
{
	writeCharacterStats();
}

void EditPartyDialog::editWml()
{
	writeParty();
	EditWmlDialog d(party_);
	d.exec();
	if(d.result()) {
		accept();
	}
}

void EditPartyDialog::loadCharacterStats()
{
	wml::node_vector members = wml::child_nodes(party_, "character");
	const int row = ui_.membersListWidget->currentRow();
	if(row >= 0 && row < members.size()) {
		wml::const_node_ptr c = members[row];

		const std::string gen = c->attr("id");
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

		wml::const_node_ptr attr = c->get_child("attributes");
		if(attr) {
			ui_.levelLineEdit->setText(QString(c->attr("level").c_str()));
			ui_.skillsLineEdit->setText(QString(c->attr("skills").c_str()));
			ui_.equipmentLineEdit->setText(QString(c->attr("equipment").c_str()));
			ui_.strengthLineEdit->setText(QString(attr->attr("strength").c_str()));
			ui_.agilityLineEdit->setText(QString(attr->attr("agility").c_str()));
			ui_.enduranceLineEdit->setText(QString(attr->attr("endurance").c_str()));
			ui_.intelligenceLineEdit->setText(QString(attr->attr("intelligence").c_str()));
			ui_.perceptionLineEdit->setText(QString(attr->attr("perception").c_str()));
			ui_.personaLineEdit->setText(QString(attr->attr("persona").c_str()));
		}
	}
}

void EditPartyDialog::writeCharacterStats()
{
	wml::node_vector members = wml::child_nodes(party_, "character");
	const int row = ui_.membersListWidget->currentRow();
	if(row < 0 || row >= members.size()) {
		return;
	}

	wml::node_ptr ch = members[row];

	wml::node_ptr attr = ch->get_child("attributes");
	if(!attr) {
		attr.reset(new wml::node("attributes"));
		ch->add_child(attr);
	}

	ch->set_or_erase_attr("level", ui_.levelLineEdit->text().toAscii().data());
	ch->set_or_erase_attr("skills", ui_.skillsLineEdit->text().toAscii().data());
	ch->set_or_erase_attr("equipment", ui_.equipmentLineEdit->text().toAscii().data());
	attr->set_or_erase_attr("strength", ui_.strengthLineEdit->text().toAscii().data());
	attr->set_or_erase_attr("agility", ui_.agilityLineEdit->text().toAscii().data());
	attr->set_or_erase_attr("endurance", ui_.enduranceLineEdit->text().toAscii().data());
	attr->set_or_erase_attr("intelligence", ui_.intelligenceLineEdit->text().toAscii().data());
	attr->set_or_erase_attr("perception", ui_.perceptionLineEdit->text().toAscii().data());
	attr->set_or_erase_attr("persona", ui_.personaLineEdit->text().toAscii().data());
}
