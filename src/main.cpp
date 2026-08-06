#include <QApplication>
#include <QResource>

#include "mainwindow.h"
#include "powersystem.h"

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(katex);
	QApplication app(argc, argv);

	PowerSystem system(100e6, 110e3);  // S_base=100 МВА, V_base=110 кВ

	MainWindow w(system);
	w.show();

	return app.exec();
}