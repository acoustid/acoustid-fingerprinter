#ifndef FPSUBMIT_FINGERPRINTER_H_
#define FPSUBMIT_FINGERPRINTER_H_

#include <QObject>
#include <QDir>
#include <QFutureWatcher>

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

signals:
    void mainStatusChanged(const QString &message);
    void fileListLoaded(int fileCount);
    void fileProcessed(int processedFileCount);

private slots:
    void startReadingFileList(const QStringList &directories);
    void startFingerprintingFiles();
    void processFileList();
    void processAnalyzedFile(int index);
    void finish();

private:
    void removeDuplicateDirectories();

    QString m_apiKey;
    QStringList m_files;
    QStringList m_directories;
    QFutureWatcher<QFileInfoList> *m_fileListWatcher;
    QFutureWatcher<AnalyzeResult *> *m_analyzeWatcher;
    int m_counter;
};

#endif
