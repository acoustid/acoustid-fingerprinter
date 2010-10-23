#ifndef FPSUBMIT_PROGRESSDIALOG_H_
#define FPSUBMIT_PROGRESSDIALOG_H_

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include "fingerprinter.h"

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    ProgressDialog(QWidget *parent, Fingerprinter *fingerprinter);
    ~ProgressDialog();

public slots:
    void setProgress(int value);
    void togglePause(bool);
    void stop();
	void onFileListLoadingStarted();
	void onFingerprintingStarted(int count);
	void onCurrentPathChanged(const QString &path);
	void onFinished();

protected:
    void closeEvent(QCloseEvent *event);

private:
    void setupUi();

	Fingerprinter *m_fingerprinter;
    QPushButton *m_closeButton;
    QPushButton *m_pauseButton;
    QPushButton *m_stopButton;
    QProgressBar *m_progressBar;
    QLabel *m_mainStatusLabel;
    QLabel *m_currentPathLabel;
};

#endif
