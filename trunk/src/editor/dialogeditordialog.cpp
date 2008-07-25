#include <QtGui/QMessageBox>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsRectItem>
#include <QtGui/QGraphicsItemGroup>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QMouseEvent>

#include <iostream>

#include "dialogeditordialog.moc"
#include "dialogeditordialog.hpp"

#include "../foreach.hpp"
#include "../wml_node.hpp"
#include "../wml_utils.hpp"

class DialogGraphicsScene : public QGraphicsScene
{
	DialogEditorDialog& dialog_;
public:
	explicit DialogGraphicsScene(DialogEditorDialog& d) : dialog_(d)
	{}

	void mousePressEvent(QGraphicsSceneMouseEvent* e) {
		dialog_.selectItem(e->scenePos().x(), e->scenePos().y());
	}
private:
};

int DialogEditorDialog::build_graphics_items(double x, double y, Command& cmd) {
	foreach(QGraphicsItem* i, cmd.items) {
		cmd.item_group->removeFromGroup(i);
	}

	cmd.areas.clear();

	const int RectWidth = 200;
	const int RectBorder = 5;

	int max_height = 0;

	if(cmd.node->name() == "dialog") {
		SelectableArea* rect_widget = new SelectableArea(cmd.node, x, y, RectWidth, 100, cmd.item_group);
		rect_widget->setZValue(0);
		cmd.areas.push_back(rect_widget);
		cmd.items.push_back(rect_widget);
		double xpos = x + RectBorder;
		double ypos = y + RectBorder;
		for(wml::node::const_attr_iterator i = cmd.node->begin_attr();
		    i != cmd.node->end_attr(); ++i) {
			std::string text = i->first + ": " + i->second;
			QGraphicsTextItem* text_widget = new QGraphicsTextItem(QString(text.c_str()));
			text_widget->setZValue(1);
			text_widget->setPos(xpos, ypos);
			text_widget->setTextWidth(RectWidth - RectBorder*2);
			text_widget->adjustSize();
			ypos += text_widget->boundingRect().height();
			cmd.items.push_back(text_widget);
		}

		rect_widget->setRect(x, y, RectWidth, ypos - y);

		for(wml::node::const_child_iterator i = cmd.node->begin_child("option");
		    i != cmd.node->end_child("option"); ++i) {
			SelectableArea* rect_widget = new SelectableArea(i->second, x, ypos, RectWidth, 100, cmd.item_group);
			rect_widget->setZValue(0);
			cmd.areas.push_back(rect_widget);
			cmd.items.push_back(rect_widget);
			const std::string text = i->second->attr("text");
			QGraphicsTextItem* text_widget = new QGraphicsTextItem(QString(text.c_str()));
			text_widget->setZValue(1);
			text_widget->setPos(xpos, ypos);
			text_widget->setTextWidth(RectWidth - RectBorder*2);
			text_widget->adjustSize();
			rect_widget->setRect(x, ypos, RectWidth, text_widget->boundingRect().height());
			ypos += text_widget->boundingRect().height();
			cmd.items.push_back(text_widget);
		}

		if(ypos - y > max_height) {
			max_height = ypos - y;
		}
	}
	
	foreach(QGraphicsItem* i, cmd.items) {
		cmd.item_group->addToGroup(i);
	}

	return max_height;
}

DialogEditorDialog::DialogEditorDialog(wml::node_ptr node, QWidget* parent)
  : QDialog(parent), node_(node), cmds_(node->get_child("dialog_commands")),
    selected_command_(-1), selected_area_(-1)
{
	if(!cmds_) {
		cmds_ = wml::node_ptr(new wml::node("dialog_commands"));
		node_->add_child(cmds_);
	}

	ui_.setupUi(this);
	int xpos = 10, ypos = 10;

	ui_.dialogGraphicsView->setScene(new DialogGraphicsScene(*this));

	int nitems = 0;
	for(wml::node::all_child_iterator i = cmds_->begin_children();
	    i != cmds_->end_children(); ++i) {
		++nitems;
	}

	int ncols = 1;
	while(ncols*ncols < nitems) {
		++ncols;
	}

	int row = 0, col = 0;
	int max_height = 0;

	for(wml::node::all_child_iterator i = cmds_->begin_children();
	    i != cmds_->end_children(); ++i) {
		QGraphicsScene* scene = ui_.dialogGraphicsView->scene();
		if(scene == NULL) {
			std::cerr << "NULL SCENE!\n";
		}
		Command cmd;
		cmd.node = *i;
		cmd.item_group = scene->createItemGroup(QList<QGraphicsItem*>());
		const int height = build_graphics_items(xpos + col*300, ypos, cmd);
		commands_.push_back(cmd);
		if(height > max_height) {
			max_height = height;
		}
		++col;
		if(col == ncols) {
			++row;
			col = 0;
			std::cerr << "max_height: " << max_height << "\n";
			ypos += max_height + 20;
			max_height = 0;
		}
	}
}

void DialogEditorDialog::mouseReleaseEvent(QMouseEvent* e)
{
	std::cerr << "PRESS: " << e->x() << "," << e->y() << "\n";
}

void DialogEditorDialog::SelectableArea::select(bool selected)
{
	if(selected) {
		setBrush(QBrush(QColor(255, 255, 0)));
		setPen(QPen(QColor(255, 0, 0)));
	} else {
		setBrush(QBrush());
		setPen(QPen(QColor(0, 0, 0)));
	}
	update();
}

void DialogEditorDialog::SelectableArea::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	std::cerr << "press: " << e->pos().x() << "," << e->pos().y() << "\n";
}

void DialogEditorDialog::selectItem(int x, int y)
{
	std::cerr << "select: " << x << ", " << y << "\n";
	int cmd, area;
	if(getAreaAtPos(x, y, &cmd, &area)) {
		std::cerr << "SELECT: " << cmd << ", " << area << "\n";
		if(selected_command_ >= 0 && selected_area_ >= 0) {
			commands_[selected_command_].areas[selected_area_]->select(false);
		}
		selected_command_ = cmd;
		selected_area_ = area;
		commands_[selected_command_].areas[selected_area_]->select(true);
	} else {
		std::cerr << "NO SELECT\n";
	}
}

bool DialogEditorDialog::getAreaAtPos(int x, int y, int* cmd, int* area) const
{
	std::cerr << " COMMANDS: " << commands_.size() << "\n";
	for(int i = 0; i != commands_.size(); ++i) {
		std::cerr << "CMD[" << i << "]" << commands_[i].areas.size() << "\n";
		for(int j = 0; j != commands_[i].areas.size(); ++j) {
			const QRectF& r = commands_[i].areas[j]->rect();
			std::cerr << "is " << x << "," << y << " in " << r.left() << "," << r.top() << "," << r.right() << "," << r.bottom() << " ? " << (r.contains(x,y) ? "yes" : "no") << "\n";
			if(r.contains(x, y)) {
				*cmd = i;
				*area = j;
				return true;
			}
		}
	}

	return false;
}
