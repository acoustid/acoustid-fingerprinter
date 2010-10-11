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
    void stopClicked();
    void pauseClicked();
    void resumeClicked();

public slots:
    void setMainStatus(const QString &message);
    void configureProgressBar(int maximum);
    void setProgress(int value);
    void togglePause(bool);
    void stop();

protected:
    void closeEvent(QCloseEvent *event);

private:
    void setupUi();

    QPushButton *m_pauseButton;
    QPushButton *m_stopButton;
    QProgressBar *m_progressBar;
    QLabel *m_mainStatusLabel;
};

#endif
