#include <QDebug>
#include <QDir>
#include <QtConcurrentMap>
#include "fingerprinter.h"
#include "tagreader.h"

Fingerprinter::Fingerprinter(const QString &apiKey, const QStringList &directories)
    : m_apiKey(apiKey), m_directories(directories), m_counter(0)
{
    removeDuplicateDirectories();
    m_fileListWatcher = new QFutureWatcher<QFileInfoList>(this);
    connect(m_fileListWatcher, SIGNAL(finished()), SLOT(processFileList()));
    m_analyzeWatcher = new QFutureWatcher<AnalyzeResult *>(this);
    connect(m_analyzeWatcher, SIGNAL(resultReadyAt(int)), SLOT(processAnalyzedFile(int)));
    connect(m_analyzeWatcher, SIGNAL(finished()), SLOT(finish()));
}

void Fingerprinter::pause()
{
    if (!m_fileListWatcher->isPaused()) {
        m_fileListWatcher->pause();
        m_analyzeWatcher->pause();
    }
}

void Fingerprinter::resume()
{
    if (m_fileListWatcher->isPaused()) {
        m_fileListWatcher->resume();
        m_analyzeWatcher->resume();
    }
}

void Fingerprinter::stop()
{
    if (!m_fileListWatcher->isCanceled()) {
        emit mainStatusChanged(tr("Stopping..."));
        m_fileListWatcher->cancel();
        m_analyzeWatcher->cancel();
    }
}

void Fingerprinter::removeDuplicateDirectories()
{
    int directoryCount = m_directories.size();
    QList<QString> sortedDirectories;
    for (int i = 0; i < directoryCount; i++) {
        sortedDirectories.append(QDir(m_directories.at(i)).canonicalPath() + QDir::separator());
    }
    qSort(sortedDirectories);
    m_directories.clear();
    m_directories.append(sortedDirectories.first());
    for (int j = 0, i = 1; i < directoryCount; i++) {
        QString path = sortedDirectories.at(i);
        if (!path.startsWith(m_directories.at(j))) {
            m_directories.append(path);
            j++;
        }
    }
}

Fingerprinter::~Fingerprinter()
{
}

void Fingerprinter::start()
{
    emit mainStatusChanged(tr("Reading file list..."));
    startReadingFileList(m_directories);
}

static QFileInfoList getEntryInfoList(const QString &path)
{
    return QDir(path).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
}

#include "decoder.h"

static AnalyzeResult *analyzeFile(const QString &path)
{
    AnalyzeResult *result = new AnalyzeResult();
    result->fileName = path;

    TagReader tags(path);
    if (!tags.read()) {
        result->error = true;
        result->errorMessage = "Couldn't read metadata";
        return result;
    }

    result->mbid = tags.mbid();
    result->length = tags.length();
    result->bitrate = tags.bitrate();

    Decoder decoder(qPrintable(path));
    if (!decoder.Open()) {
        result->error = true;
        result->errorMessage = "Couldn't open the file";
        return result;
    }

    FingerprintCalculator fpcalculator;
    fpcalculator.start(decoder.SampleRate(), decoder.Channels());
    decoder.Decode(&fpcalculator, 120);
    result->fingerprint = fpcalculator.finish();

    return result;
}

void Fingerprinter::startReadingFileList(const QStringList &directories)
{
    QFuture<QFileInfoList> results = QtConcurrent::mapped(directories, &getEntryInfoList);
    m_fileListWatcher->setFuture(results);
}

void Fingerprinter::startFingerprintingFiles()
{
    QFuture<AnalyzeResult *> results = QtConcurrent::mapped(m_files, &analyzeFile);
    m_analyzeWatcher->setFuture(results);
}

void Fingerprinter::processFileList()
{
    QSet<QString> extensions = QSet<QString>()
        << ".MP3"
        << ".MP4"
        << ".M4A"
        << ".FLAC"
        << ".OGG"
        << ".OGA"
        << ".APE"
        << ".OGGFLAC"
        << ".TTA"
        << ".WV"
        << ".MPC"
        << ".WMA";

    QStringList directories;
    QFuture<QFileInfoList> results = m_fileListWatcher->future();
    for (int i = 0; i < results.resultCount(); i++) {
        QFileInfoList fileInfoList = results.resultAt(i); 
        for (int j = 0; j < fileInfoList.size(); j++) {
            QFileInfo fileInfo = fileInfoList.at(j);
            if (fileInfo.isDir()) {
                directories.append(fileInfo.filePath());
            }
            else {
                QString path = fileInfo.filePath();
                QString extension = path.mid(path.lastIndexOf('.')).toUpper();
                if (extensions.contains(extension)) {
                    m_files.append(path);
                }
            }
        }
    }
    if (!directories.isEmpty()) {
        startReadingFileList(directories);
    }
    else {
        emit mainStatusChanged(tr("Fingerprinting..."));
        emit fileListLoaded(m_files.size());
        startFingerprintingFiles();
    }
}

void Fingerprinter::processAnalyzedFile(int index)
{
    AnalyzeResult *result = m_analyzeWatcher->future().resultAt(index);
    if (!result->error) {
        qDebug() << "\n" << result->fileName;
        qDebug() << " > MBID " << result->mbid;
        qDebug() << " > Length " << result->length;
        qDebug() << " > Bitrate " << result->bitrate;
        qDebug() << " > Fingerprint " << result->fingerprint;
    }
    emit fileProcessed(++m_counter);
    delete result;
}

void Fingerprinter::finish()
{
    emit mainStatusChanged(tr("Finished"));
}

