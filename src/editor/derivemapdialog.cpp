#include "boost/lexical_cast.hpp"
#include "derivemapdialog.moc"
#include "derivemapdialog.hpp"

#include "../formatter.hpp"
#include "../zoom_map_generator.hpp"

DeriveMapDialog::DeriveMapDialog(const hex::gamemap& map) : map_(map)
{
	ui_.setupUi(this);
	ui_.xLineEdit->setText(QString("0"));
	ui_.yLineEdit->setText(QString("0"));
	ui_.widthLineEdit->setText(QString((formatter() << map.size().x()).c_str()));
	ui_.heightLineEdit->setText(QString((formatter() << map.size().y()).c_str()));
	ui_.newWidthLineEdit->setText(QString((formatter() << map.size().x()).c_str()));
	ui_.newHeightLineEdit->setText(QString((formatter() << map.size().y()).c_str()));
	ui_.fuzzinessLineEdit->setText(QString("0.0"));
	ui_.heightScaleLineEdit->setText(QString("1.0"));

	QApplication::connect(ui_.okButton, SIGNAL(pressed()), this, SLOT(okPressed()));
}

void DeriveMapDialog::okPressed()
{
	try {
		std::cerr << "okay\n";
		int x = boost::lexical_cast<int>(ui_.xLineEdit->text().toAscii().data());
		int y = boost::lexical_cast<int>(ui_.yLineEdit->text().toAscii().data());
		int x2 = boost::lexical_cast<int>(ui_.widthLineEdit->text().toAscii().data());
		int y2 = boost::lexical_cast<int>(ui_.heightLineEdit->text().toAscii().data());

		if(x2 < x) {
			std::swap(x,x2);
		}

		if(y2 < y) {
			std::swap(y,y2);
		}

		const int w = 1+x2 - x;
		const int h = 1+x2 - x;

		const int new_w = boost::lexical_cast<int>(ui_.newWidthLineEdit->text().toAscii().data());
		const int new_h = boost::lexical_cast<int>(ui_.newHeightLineEdit->text().toAscii().data());
		const double height_scale = boost::lexical_cast<double>(ui_.heightScaleLineEdit->text().toAscii().data());
		const double fuzziness = boost::lexical_cast<double>(ui_.fuzzinessLineEdit->text().toAscii().data());
		if(!map_.is_loc_on_map(hex::location(x,y)) || !(map_.is_loc_on_map(hex::location(x+w-1,y+h-1)))) {
			std::cerr << "bad dimensions\n";
		}

		if(new_w <= 0 || new_h <= 0) {
			return;
		}

		result_ = hex::generate_zoom_map(map_, hex::location(x,y), hex::location(w,h), hex::location(new_w,new_h), height_scale, fuzziness);
		if(result_) {
			std::cerr << "generated map: " << new_w << ", " << new_h << "\n";
		}
	} catch(boost::bad_lexical_cast&) {
		std::cerr << "could not recognize number\n";
	}
}

boost::shared_ptr<hex::gamemap> DeriveMapDialog::result() const
{
	return result_;
}
