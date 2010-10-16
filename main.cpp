#include <QApplication>
#include "decoder.h"
#include "mainwindow.h"

int main(int argc, char **argv)
{
	Decoder::initialize();
	QApplication app(argc, argv);
	app.setOrganizationName("Acoustid");
	app.setOrganizationDomain("acoustid.org");
	app.setApplicationName("Fingerprinter");
	MainWindow window;
	window.show();
	return app.exec();
}
