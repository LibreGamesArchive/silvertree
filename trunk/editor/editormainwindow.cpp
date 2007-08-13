#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

#include <QtGui/QFileDialog>

#include "editormainwindow.hpp"

EditorMainWindow::EditorMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	QApplication::connect(ui.action_Open, SIGNAL(triggered()), this, SLOT(openRequested()));
	QApplication::connect(ui.action_Quit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));

	opened_ = false;
	ui.editorGLWidget->hide();
}

void EditorMainWindow::openRequested() {
	//TODO: check if we have something else open and ask if we should save it, free everything and all
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Map"));
	openMap(fileName.toAscii().data());
}

bool EditorMainWindow::openMap(char *file) {
	const int fd = open(file,O_RDONLY);
	if(fd < 0) {
		std::cerr << "could not open map\n";
		opened_ = false;
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
	ui.editorGLWidget->show();
	opened_ = true;
	return true;
}

#include "editormainwindow.moc"
