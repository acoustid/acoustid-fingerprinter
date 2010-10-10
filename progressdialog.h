#ifndef FPSUBMIT_PROGRESSDIALOG_H_
#define FPSUBMIT_PROGRESSDIALOG_H_

#include <QDialog>
#include <QProgressBar>
#include <QLabel>

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    ProgressDialog(QWidget *parent = 0);
    ~ProgressDialog();

signals:
    void stop();

public slots:
    void setMainStatus(const QString &message);
    void configureProgressBar(int maximum);
    void setProgress(int value);

private:
    void setupUi();

    QProgressBar *m_progressBar;
    QLabel *m_mainStatusLabel;
};

#endif
