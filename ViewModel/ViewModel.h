#pragma once

#include <QObject>
#include <QStringList>
#include <MainWindowView.h>


class ViewModel : public QObject {

private:

	Q_OBJECT
	//Q_PROPERTY(QStringList tracks READ tracks NOTIFY tracksChanged)

public:

	explicit ViewModel(QObject* parent = nullptr);

};