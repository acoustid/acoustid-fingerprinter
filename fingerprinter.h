#ifndef FPSUBMIT_FINGERPRINTER_H_
#define FPSUBMIT_FINGERPRINTER_H_

#include <QObject>
#include <QDir>
#include <QFutureWatcher>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

struct AnalyzeResult
{
    AnalyzeResult() : error(false)
    {
    }

    QString fileName;
    QString mbid;
    QString fingerprint;
    int length;
    int bitrate;
    bool error;
    QString errorMessage;
};

class Fingerprinter : public QObject 
{
    Q_OBJECT

public:
    Fingerprinter(const QString &apiKey, const QStringList &directories);
    ~Fingerprinter();

    void start();
	bool isFinished() { return m_finished; }

signals:
    void mainStatusChanged(const QString &message);
    void fileListLoaded(int fileCount);
    void fileProcessed(int processedFileCount);
	void finished();

public slots:
    void pause();
    void resume();
    void stop();

private slots:
    void startReadingFileList(const QStringList &directories);
    void startFingerprintingFiles();
    void processFileList();
    void processFilteredFileList();
    void processAnalyzedFile(int index);
    void finish();

private:
    void removeDuplicateDirectories();
	bool maybeSubmitQueue(bool force = false);
	QByteArray prepareSubmitData();
	QNetworkRequest prepareSubmitRequest();

    QString m_apiKey;
    QStringList m_files;
    QStringList m_directories;
    QFutureWatcher<QFileInfoList> *m_fileListWatcher;
    QFutureWatcher<AnalyzeResult *> *m_analyzeWatcher;
	QFutureWatcher<QStringList> *m_filterFileListWatcher;
	QList<AnalyzeResult *> m_submitQueue;
    int m_counter;
	bool m_finished;
	QMutex m_cacheMutex;
	QNetworkAccessManager *m_networkAccessManager;
};

#endif
