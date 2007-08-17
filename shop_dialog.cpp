#include <assert.h>
#include <iostream>

#include "button.hpp"
#include "callback.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "item.hpp"
#include "item_display_dialog.hpp"
#include "label.hpp"
#include "party.hpp"
#include "shop_dialog.hpp"
#include "string_utils.hpp"
#include "translate.hpp"

namespace game_dialogs {

class purchase_callback : public functional::callback<shop_dialog>
{
	int index_;
	void execute(shop_dialog* ptr) { ptr->purchase(index_); }
public:
	purchase_callback(shop_dialog* ptr, int index) : functional::callback<shop_dialog>(ptr), index_(index)
	{}
};

shop_dialog::shop_dialog(game_logic::party& party, int cost,
                         const std::string& items_str)
   : dialog(0,0,1024,768), party_(party), cost_(cost)
{
	const std::vector<std::string> items = util::split(items_str);
	foreach(const std::string& str, items) {
		std::cerr << "adding item: '" << str << "'\n";
		game_logic::const_item_ptr i = game_logic::item::get(str);
		if(i) {
			items_.push_back(i);
		} else {
			std::cerr << "could not find item '" << str << "'\n";
		}
	}

	init();
}

void shop_dialog::init()
{
	clear();

	using namespace game_logic;
	using namespace gui;
	typedef widget_ptr ptr;
	const SDL_Color color = { 0xFF, 0xFF, 0xFF };
	set_padding(30);
	add_widget(ptr(new label("shop", color, 28)), 20, 20);

	add_widget(ptr(new label(formatter() << party_.money() << " "
	                         << i18n::translate("currency_units"), color, 22)));

	grid_ptr grid(new gui::grid(3));
	int index = 0;
	foreach(const const_item_ptr& i, items_) {
		dialog* d = new item_display_dialog(i);
		d->add_widget(ptr(new label(formatter() << price(i) << " "
		                    << i18n::translate("currency_units"), color, 20)));
		functional::callback_ptr callback(new purchase_callback(this,index));

		if(price(i) <= party_.money()) {
			button_ptr b(new button(ptr(new label("purchase", color)),callback));
			d->add_widget(b);
		}
		grid->add_col(widget_ptr(d));
		++index;
	}

	grid->finish_row();
	add_widget(grid);
}

int shop_dialog::price(const game_logic::const_item_ptr& i) const
{
	return (i->value()*cost_)/100;
}

void shop_dialog::purchase(int index)
{
	assert(index >= 0 && index < items_.size());
	party_.get_money(-price(items_[index]));
	party_.acquire_item(items_[index]->clone());
	init();
}

}
