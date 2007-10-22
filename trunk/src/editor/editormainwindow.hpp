#ifndef EDITORMAINWINDOW_HPP_INCLUDED
#define EDITORMAINWINDOW_HPP_INCLUDED

#include <QtGui/QToolButton>
#include <map>
#include <vector>

#include "../wml_node_fwd.hpp"
#include "ui_mainwindow.hpp"

class EditorMainWindow : public QMainWindow
{
	Q_OBJECT

	public:
		EditorMainWindow(QWidget *parent = 0);
		bool openScenario(const char* file);
		bool openMap(wml::const_node_ptr node);

	public slots:
		void openRequested();
		void saveRequested();
		void deriveMap();
		void zoominRequested();
		void zoomoutRequested();
		void horzScroll(int value);
		void vertScroll(int value);
		void rotateLeft();
		void rotateRight();
		void tiltUp();
		void tiltDown();
		void panUp();
		void panDown();
		void panLeft();
		void panRight();
		void undo();
		void redo();
		void setTerrain(const std::string& id, bool feature, int button);

	private:
		Ui::MainWindow ui;
		hex::camera *camera_;
		boost::shared_ptr<hex::gamemap> map_;
		wml::node_ptr scenario_;
		bool opened_;
		std::map<hex::location,wml::node_ptr> parties_;
		std::vector<class TerrainHandler*> handlers_;
		std::vector<QToolButton*> tool_buttons_;
		std::string fname_;
};

#endif
