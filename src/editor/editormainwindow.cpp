#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

#include <QtGui/QFileDialog>
#include <QtGui/QScrollBar>
#include <QtGui/QShortcut>
#include <QtGui/QToolButton>

#include "../base_terrain.hpp"
#include "../terrain_feature.hpp"
#include "../filesystem.hpp"
#include "../wml_parser.hpp"
#include "../wml_utils.hpp"
#include "../wml_writer.hpp"
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

	QApplication::connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S),this), SIGNAL(activated()), this, SLOT(saveRequested()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_O),this), SIGNAL(activated()), this, SLOT(openRequested()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::Key_Z),this), SIGNAL(activated()), this, SLOT(zoominRequested()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::Key_X),this), SIGNAL(activated()), this, SLOT(zoomoutRequested()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left),this), SIGNAL(activated()), this, SLOT(rotateLeft()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right),this), SIGNAL(activated()), this, SLOT(rotateRight()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up),this), SIGNAL(activated()), this, SLOT(tiltUp()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down),this), SIGNAL(activated()), this, SLOT(tiltDown()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::Key_Left),this), SIGNAL(activated()), this, SLOT(panLeft()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::Key_Right),this), SIGNAL(activated()), this, SLOT(panRight()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::Key_Up),this), SIGNAL(activated()), this, SLOT(panUp()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::Key_Down),this), SIGNAL(activated()), this, SLOT(panDown()));

	ui.action_Save->setEnabled(false);
	ui.editorGLWidget->setEnabled(false);

	QToolButton* party_button = new QToolButton();
	party_button->setText("Parties");
	party_button->setCheckable(true);
	party_button->setChecked(false);
	handlers_.push_back(new TerrainHandler(*this, "party", false, tool_buttons_.size()));
	QApplication::connect(party_button, SIGNAL(pressed()), handlers_.back(), SLOT(terrainSelected()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::Key_P),this), SIGNAL(activated()), handlers_.back(), SLOT(terrainSelected()));
	ui.tilesToolBar->addWidget(party_button);
	tool_buttons_.push_back(party_button);

	QToolButton* height_button = new QToolButton();
	height_button->setText("Height");
	height_button->setCheckable(true);
	height_button->setChecked(true);
	handlers_.push_back(new TerrainHandler(*this, "", false, tool_buttons_.size()));
	QApplication::connect(height_button, SIGNAL(pressed()), handlers_.back(), SLOT(terrainSelected()));
	QApplication::connect(new QShortcut(QKeySequence(Qt::Key_H),this), SIGNAL(activated()), handlers_.back(), SLOT(terrainSelected()));
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
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Scenario"));
	if(fileName.length()) {
		openScenario(fileName.toAscii().data());
	}
}

void EditorMainWindow::saveRequested() {
	if(fname_.empty()) {
		QString fileName = QFileDialog::getSaveFileName(this, tr("Save Scenario"));
		if(!fileName.isNull()) {
			fname_ = fileName.toAscii().data();
		}
	}

	if(fname_.empty()) {
		return;
	}

	scenario_->clear_children("party");
	for(std::map<hex::location,wml::node_ptr>::const_iterator i = parties_.begin(); i != parties_.end(); ++i) {
		scenario_->add_child(i->second);
	}

	if(scenario_->has_attr("map")) {
		sys::write_file((*scenario_)["map"], map_->write());
	} else {
		scenario_->set_attr("map_data", map_->write());
	}

	std::string scenarioData;
	wml::write(scenario_, scenarioData);
	sys::write_file(fname_, scenarioData);
}

void EditorMainWindow::zoominRequested() {
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

void EditorMainWindow::panUp() {
	ui.verticalScrollBar->triggerAction(QScrollBar::SliderSingleStepSub);
}

void EditorMainWindow::panDown() {
	ui.verticalScrollBar->triggerAction(QScrollBar::SliderSingleStepAdd);
}

void EditorMainWindow::panLeft() {
	ui.horizontalScrollBar->triggerAction(QScrollBar::SliderSingleStepSub);
}

void EditorMainWindow::panRight() {
	ui.horizontalScrollBar->triggerAction(QScrollBar::SliderSingleStepAdd);
}

void EditorMainWindow::undo() {
	ui.editorGLWidget->undo();
}

void EditorMainWindow::redo() {
	ui.editorGLWidget->redo();
}

bool EditorMainWindow::openScenario(const char *file) {
	wml::node_ptr old_scenario = scenario_;

	if(file) {
		fname_ = file;
		scenario_ = wml::parse_wml(sys::read_file(file));
	} else {
		fname_ = "";
		scenario_ = wml::parse_wml(
"[scenario]"
"map_data=\""
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\n"
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\n"
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\n"
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\n"
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\n"
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\n"
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\n"
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\n"
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\n"
"0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h, 0 h\"\n"
"[/scenario]");
	}

	if(!scenario_) {
		return false;
	}

	if(!openMap(scenario_)) {
		scenario_ = old_scenario;
		return false;
	}

	parties_.clear();
	std::vector<wml::node_ptr> parties = wml::child_nodes(scenario_, "party");
	foreach(wml::node_ptr party, parties) {
		hex::location loc(wml::get_int(party,"x"),wml::get_int(party,"y"));
		parties_[loc] = party;
	}

	ui.editorGLWidget->setParties(&parties_);
	return true;
}

bool EditorMainWindow::openMap(wml::const_node_ptr node) {
	std::string mapdata;
	if(node->has_attr("map_data")) {
		mapdata = (*node)["map_data"];
	} else {
		const int fd = open((*node)["map"].c_str(),O_RDONLY);
		if(fd < 0) {
			return false;
		}
		struct stat fileinfo;
		fstat(fd,&fileinfo);

		std::vector<char> filebuf(fileinfo.st_size);
		read(fd,&filebuf[0],fileinfo.st_size);
		mapdata.assign(filebuf.begin(),filebuf.end());
		::close(fd);
	}

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
	} else if(id == "party") {
		ui.editorGLWidget->setEditParties();
	} else if(feature) {
		ui.editorGLWidget->setCurrentFeature(id);
	} else {
		ui.editorGLWidget->setCurrentTerrain(id);
	}
}

#include "editormainwindow.moc"
