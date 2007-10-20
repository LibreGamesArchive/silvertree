#ifndef EDITPARTYDIALOG_HPP_INCLUDED
#define EDITPARTYDIALOG_HPP_INCLUDED

#include <QtGui/QDialog>

#include "ui_editpartydialog.hpp"
#include "../wml_node.hpp"

class EditPartyDialog : public QDialog
{
	Q_OBJECT

public:
	EditPartyDialog(wml::node_ptr party, QWidget *parent = 0);

public slots:
	void writeParty();
	void revertParty();
	void addMember();
	void removeMember();
	void memberChanged(int row);
	void changeGenerator(int index);
	void changeStats(const QString& text);
	void editWml();

private:
	void loadCharacterStats();
	void writeCharacterStats();
	Ui::EditPartyDialog ui_;
	wml::node_ptr party_, backup_;
};


#endif
