#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

#include "editormainwindow.hpp"

EditorMainWindow::EditorMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	QApplication::connect(ui.action_Quit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
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
	return true;
}

#include "editormainwindow.moc"
