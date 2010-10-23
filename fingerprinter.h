#ifndef FPSUBMIT_FINGERPRINTER_H_
#define FPSUBMIT_FINGERPRINTER_H_

#include <QObject>
#include <QDir>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QTime>

class AnalyzeResult;
class QNetworkReply;

class Fingerprinter : public QObject 
{
    Q_OBJECT

public:
    Fingerprinter(const QString &apiKey, const QStringList &directories);
    ~Fingerprinter();

	bool isPaused();
	bool isCancelled();
	bool isRunning();
	bool isFinished();

	int submitttedFingerprints() const { return m_submittedFiles; }

signals:
    void statusChanged(const QString &message);
    void currentPathChanged(const QString &path);
	void fileListLoadingStarted();
	void fingerprintingStarted(int fileCount);
	void progress(int i);
	void finished();
	void networkError(const QString &message);
	void authenticationError();
	void noFilesError();

public slots:
    void start();
    void pause();
    void resume();
    void cancel();

private slots:
	void onFileListLoaded(const QStringList &files);
	void onFileAnalyzed(AnalyzeResult *);
	void onRequestFinished(QNetworkReply *reply);

private:
	void fingerprintNextFile();
	bool maybeSubmit(bool force=false);

    QString m_apiKey;
    QStringList m_files;
    QStringList m_directories;
	QNetworkAccessManager *m_networkAccessManager;
	QList<AnalyzeResult *> m_submitQueue;
	QStringList m_submitting;
	QStringList m_submitted;
	QNetworkReply *m_reply;

	QTime m_time;
	int m_fingerprintedFiles;
	int m_submittedFiles;
	int m_activeFiles;
	bool m_cancelled;
	bool m_paused;
	bool m_finished;
};

#endif
