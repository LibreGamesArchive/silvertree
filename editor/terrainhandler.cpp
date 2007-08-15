#include <QtGui/QWidget>
#include "editormainwindow.hpp"
#include "terrainhandler.hpp"
#include "terrainhandler.moc"

void TerrainHandler::terrainSelected() {
	window_.setTerrain(id_, feature_, button_);
}
