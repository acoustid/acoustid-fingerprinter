#ifndef FPSUBMIT_MAINWINDOW_H_
#define FPSUBMIT_MAINWINDOW_H_

#include <QMainWindow>
#include <QLineEdit>
#include "checkabledirmodel.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();

private slots:
	void openAcoustidWebsite();
	void fingerprint();

private:
	void setupUi();
	bool validateFields(QString &apiKey, QList<QString> &directories);

	QLineEdit *m_apiKeyEdit;
	CheckableDirModel *m_directoryModel;
};

#endif
