#ifndef DERIVEMAP_DIALOG_HPP_INCLUDED
#define DERIVEMAP_DIALOG_HPP_INCLUDED

#include "boost/shared_ptr.hpp"
#include <QtGui/QDialog>

#include "ui_derivemapdialog.hpp"
#include "../gamemap.hpp"

class DeriveMapDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DeriveMapDialog(const hex::gamemap& m);

	boost::shared_ptr<hex::gamemap> result() const;

public slots:
	void okPressed();

private:
	const hex::gamemap& map_;
	Ui::DeriveMapDialog ui_;
	boost::shared_ptr<hex::gamemap> result_;
};


#endif
