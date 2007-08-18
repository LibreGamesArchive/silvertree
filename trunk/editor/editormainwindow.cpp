#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

#include <QtGui/QFileDialog>
#include <QtGui/QScrollBar>
#include <QtGui/QToolButton>

#include "../base_terrain.hpp"
#include "../terrain_feature.hpp"
#include "../filesystem.hpp"
#include "../wml_parser.hpp"
#include "../wml_utils.hpp"
#include "editormainwindow.hpp"
#include "terrainhandler.hpp"

EditorMainWindow::EditorMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	map_ = 0;
	camera_ = 0;

	ui.setupUi(this);
	QApplication::connect(ui.action_Open, SIGNAL(triggered()), this, SLOT(openRequested()));
	QApplication::connect(ui.action_Save, SIGNAL(triggered()), this, SLOT(saveRequested()));
	QApplication::connect(ui.action_Quit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
	QApplication::connect(ui.actionZoom_In, SIGNAL(triggered()), this, SLOT(zoominRequested()));
	QApplication::connect(ui.actionZoom_Out, SIGNAL(triggered()), this, SLOT(zoomoutRequested()));
	QApplication::connect(ui.horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScroll(int)));
	QApplication::connect(ui.verticalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(vertScroll(int)));
	QApplication::connect(ui.actionRotate_Left, SIGNAL(triggered()), this, SLOT(rotateLeft()));
	QApplication::connect(ui.actionRotate_Right, SIGNAL(triggered()), this, SLOT(rotateRight()));
	QApplication::connect(ui.actionTilt_Up, SIGNAL(triggered()), this, SLOT(tiltUp()));
	QApplication::connect(ui.actionTilt_Down, SIGNAL(triggered()), this, SLOT(tiltDown()));
	QApplication::connect(ui.actionUndo, SIGNAL(triggered()), this, SLOT(undo()));
	QApplication::connect(ui.actionRedo, SIGNAL(triggered()), this, SLOT(redo()));

	ui.action_Save->setEnabled(false);
	ui.editorGLWidget->setEnabled(false);

	QToolButton* height_button = new QToolButton();
	height_button->setText("Height");
	height_button->setCheckable(true);
	height_button->setChecked(true);
	handlers_.push_back(new TerrainHandler(*this, "", false, tool_buttons_.size()));
	QApplication::connect(height_button, SIGNAL(pressed()), handlers_.back(), SLOT(terrainSelected()));
	ui.tilesToolBar->addWidget(height_button);
	tool_buttons_.push_back(height_button);

	QToolButton* picker_button = new QToolButton();
	picker_button->setText("Picker");
	picker_button->setCheckable(true);
	handlers_.push_back(new TerrainHandler(*this, "", true, tool_buttons_.size()));
	QApplication::connect(picker_button, SIGNAL(pressed()), handlers_.back(), SLOT(terrainSelected()));
	ui.tilesToolBar->addWidget(picker_button);
	tool_buttons_.push_back(picker_button);

	std::vector<std::string> terrain_ids;
	hex::base_terrain::get_terrain_ids(terrain_ids);
	for(int n = 0; n != terrain_ids.size(); ++n) {
		QToolButton* b = new QToolButton();
		b->setText(hex::base_terrain::get(terrain_ids[n])->name().c_str());
		b->setCheckable(true);
		handlers_.push_back(new TerrainHandler(*this, terrain_ids[n], false, tool_buttons_.size()));
		QApplication::connect(b, SIGNAL(pressed()), handlers_.back(), SLOT(terrainSelected()));
		ui.tilesToolBar->addWidget(b);
		tool_buttons_.push_back(b);
	}

	terrain_ids.clear();
	hex::terrain_feature::get_feature_ids(terrain_ids);
	for(int n = 0; n != terrain_ids.size(); ++n) {
		QToolButton* b = new QToolButton();
		b->setText(hex::terrain_feature::get(terrain_ids[n])->name().c_str());
		b->setCheckable(true);
		handlers_.push_back(new TerrainHandler(*this, terrain_ids[n], true, tool_buttons_.size()));
		QApplication::connect(b, SIGNAL(pressed()), handlers_.back(), SLOT(terrainSelected()));
		ui.tilesToolBar->addWidget(b);
		tool_buttons_.push_back(b);
	}
}

void EditorMainWindow::openRequested() {
	//TODO: check if we have something else open and ask if we should save it, free everything and all
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Map"));
	openMap(fileName.toAscii().data());
}

void EditorMainWindow::saveRequested() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Map"));
	if(!fileName.isNull()) {
		sys::write_file(fileName.toAscii().data(), map_->write());
	}
}

void EditorMainWindow::zoominRequested() {
	std::cerr << "zoom\n";
	for(int n = 0; n != 5; ++n) {
		camera_->zoom_in();
	}
	ui.editorGLWidget->updateGL();
}

void EditorMainWindow::zoomoutRequested() {
	for(int n = 0; n != 5; ++n) {
		camera_->zoom_out();
	}
	ui.editorGLWidget->updateGL();
}

void EditorMainWindow::horzScroll(int value) {
	camera_->set_pan_x(-value);
	ui.editorGLWidget->updateGL();
}

void EditorMainWindow::vertScroll(int value) {
	camera_->set_pan_y(-(ui.verticalScrollBar->maximum() - value));
	ui.editorGLWidget->updateGL();
}

void EditorMainWindow::rotateLeft() {
	camera_->rotate_left();
	ui.editorGLWidget->updateGL();
}

void EditorMainWindow::rotateRight() {
	camera_->rotate_right();
	ui.editorGLWidget->updateGL();
}

void EditorMainWindow::tiltUp() {
	for(int n = 0; n != 5; ++n) {
		camera_->tilt_up();
	}
	ui.editorGLWidget->updateGL();
}

void EditorMainWindow::tiltDown() {
	for(int n = 0; n != 5; ++n) {
		camera_->tilt_down();
	}
	ui.editorGLWidget->updateGL();
}

void EditorMainWindow::undo() {
	ui.editorGLWidget->undo();
}

void EditorMainWindow::redo() {
	ui.editorGLWidget->redo();
}

bool EditorMainWindow::openScenario(const char *file) {
	wml::node_ptr old_scenario = scenario_;
	scenario_ = wml::parse_wml(sys::read_file(file));
	if(!scenario_) {
		return false;
	}

	if(!openMap((*scenario_)["map"].c_str())) {
		scenario_ = old_scenario;
		return false;
	}

	parties_.clear();
	WML_MUTABLE_FOREACH(party, scenario_, "party") {
		hex::location loc(wml::get_int(party,"x"),wml::get_int(party,"y"));
		parties_[loc] = party;
	}

	ui.editorGLWidget->setParties(&parties_);
	return true;
}

bool EditorMainWindow::openMap(const char *file) {
	const int fd = open(file,O_RDONLY);
	if(fd < 0) {
		std::cerr << "could not open map\n";
		return false;
	}
	struct stat fileinfo;
	fstat(fd,&fileinfo);
	
	std::string mapdata;

	std::vector<char> filebuf(fileinfo.st_size);
	read(fd,&filebuf[0],fileinfo.st_size);
	mapdata.assign(filebuf.begin(),filebuf.end());
	::close(fd);

	map_ = new hex::gamemap(mapdata);
	camera_ = new hex::camera(*map_);
	camera_->allow_keyboard_panning();

	camera_->set_pan_y(-(map_->size().y()-1));
	ui.horizontalScrollBar->setMaximum(map_->size().x());
	ui.verticalScrollBar->setMaximum(map_->size().y());

	ui.editorGLWidget->setMap(map_);
	ui.editorGLWidget->setCamera(camera_);
	ui.editorGLWidget->setEnabled(true);

	ui.action_Save->setEnabled(true);
	return true;
}

void EditorMainWindow::setTerrain(const std::string& id, bool feature, int button)
{
	for(int n = 0; n != tool_buttons_.size(); ++n) {
		tool_buttons_[n]->setChecked(false);
	}

	if(id == "") {
		if(feature) {
			ui.editorGLWidget->setPicker();
		} else {
			ui.editorGLWidget->setHeightEdit();
		}
		return;
	}

	if(feature) {
		ui.editorGLWidget->setCurrentFeature(id);
	} else {
		ui.editorGLWidget->setCurrentTerrain(id);
	}
}

#include "editormainwindow.moc"
