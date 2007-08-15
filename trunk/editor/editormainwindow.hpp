#ifndef EDITORMAINWINDOW_HPP_INCLUDED
#define EDITORMAINWINDOW_HPP_INCLUDED

#include <vector>

#include "ui_mainwindow.hpp"

class EditorMainWindow : public QMainWindow
{
	Q_OBJECT

	public:
		EditorMainWindow(QWidget *parent = 0); 
		bool openMap(char *file);
	
	public slots:
		void openRequested();
		void saveRequested();
		void zoominRequested();
		void zoomoutRequested();
		void horzScroll(int value);
		void vertScroll(int value);
		void rotateLeft();
		void rotateRight();
		void undo();
		void redo();
		void setTerrain(const std::string& id, bool feature);

	private:
		Ui::MainWindow ui;
		hex::camera *camera_;
		hex::gamemap *map_;
		bool opened_;
		std::vector<class TerrainHandler*> handlers_;
};

#endif
