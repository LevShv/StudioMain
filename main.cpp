#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <ViewModel.h>
#include "filebrowser.h"
#include "MidiMessageModel.h"

#include <iostream>
#include <windows.h>


int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "Ru");
#if defined(Q_OS_WIN) && QT_VERSION_CHECK(5, 6, 0) <= QT_VERSION && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

	AllocConsole();

	FILE* fDummy;

	if (freopen_s(&fDummy, "CONIN$", "r", stdin) != 0) {
		std::cerr << "Ошибка перенаправления stdin!" << std::endl;
	}
	if (freopen_s(&fDummy, "CONOUT$", "w", stdout) != 0) {
		std::cerr << "Ошибка перенаправления stdout!" << std::endl;
	}
	if (freopen_s(&fDummy, "CONOUT$", "w", stderr) != 0) {
		std::cerr << "Ошибка перенаправления stderr!" << std::endl;
	}
	qDebug() << "Compile-time version:" << QT_VERSION_STR;
    QGuiApplication app(argc, argv);

	qmlRegisterType<FileBrowser>("FileBrowser", 1, 0, "FileBrowser");

    QQmlApplicationEngine engine;

   // ViewModel viewModel;
	auto viewModel = new ViewModel(&app);
    engine.rootContext()->setContextProperty("viewModel", viewModel);

    engine.load(QUrl(QStringLiteral("qrc:/View/MainWindowView/MainWindow.qml")));

	qDebug() << "Root objects:" << engine.rootObjects();

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
