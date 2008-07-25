#ifndef DIALOGEDITORDIALOG_HPP_INCLUDED
#define DIALOGEDITORDIALOG_HPP_INCLUDED

#include <vector>

#include <QtGui/QDialog>
#include <QtGui/QGraphicsRectItem>

#include "ui_dialogeditordialog.hpp"
#include "../wml_node_fwd.hpp"

class DialogEditorDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DialogEditorDialog(wml::node_ptr node, QWidget* parent = 0);

	void selectItem(int x, int y);

protected:
	void mouseReleaseEvent(QMouseEvent* event);

private:
	Ui::DialogEditorDialog ui_;
	wml::node_ptr node_;
	wml::node_ptr cmds_;

	class SelectableArea : public QGraphicsRectItem {
	public:
		SelectableArea(wml::node_ptr node, qreal x, qreal y, qreal w, qreal h, QGraphicsItem* parent)
		  : QGraphicsRectItem(x, y, w, h, parent), node_(node)
		{
		}

		wml::node_ptr node() const { return node_; }

		void select(bool selected);
	
	protected:
		void mousePressEvent(QGraphicsSceneMouseEvent* e);
	
	private:
		wml::node_ptr node_;
	};

	struct Command {
		wml::node_ptr node;
		std::vector<QGraphicsItem*> items;
		QGraphicsItemGroup* item_group;
		std::vector<SelectableArea*> areas;
	};

	static int build_graphics_items(double x, double y, Command& cmd);

	bool getAreaAtPos(int x, int y, int* cmd, int* area) const;

	std::vector<Command> commands_;

	int selected_command_;
	int selected_area_;
};

#endif
