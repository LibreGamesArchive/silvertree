#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

#include <QtGui/QFileDialog>

#include "../filesystem.hpp"
#include "editormainwindow.hpp"

EditorMainWindow::EditorMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	map_ = 0;
	camera_ = 0;

	ui.setupUi(this);
	QApplication::connect(ui.action_Open, SIGNAL(triggered()), this, SLOT(openRequested()));
	QApplication::connect(ui.action_Save, SIGNAL(triggered()), this, SLOT(saveRequested()));
	QApplication::connect(ui.action_Quit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));

	ui.action_Save->setEnabled(false);
	ui.editorGLWidget->setEnabled(false);
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

bool EditorMainWindow::openMap(char *file) {
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

	ui.editorGLWidget->setMap(map_);
	ui.editorGLWidget->setCamera(camera_);
	ui.editorGLWidget->setEnabled(true);

	ui.action_Save->setEnabled(true);
	return true;
}

#include "editormainwindow.moc"
