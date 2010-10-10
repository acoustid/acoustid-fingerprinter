#include <QApplication>
#include "decoder.h"
#include "mainwindow.h"

int main(int argc, char **argv)
{
	Decoder::initialize();
	QApplication app(argc, argv);
	MainWindow window;
	window.show();
	return app.exec();
}
