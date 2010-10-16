#include <QDebug>
#include <QDir>
#include <QUrl>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QDesktopServices>
#include <QMutexLocker>
#include "fingerprinter.h"
#include "tagreader.h"

Fingerprinter::Fingerprinter(const QString &apiKey, const QStringList &directories)
    : m_apiKey(apiKey), m_directories(directories), m_counter(0), m_finished(false)
{
    removeDuplicateDirectories();
    m_fileListWatcher = new QFutureWatcher<QFileInfoList>(this);
    connect(m_fileListWatcher, SIGNAL(finished()), SLOT(processFileList()));
    m_analyzeWatcher = new QFutureWatcher<AnalyzeResult *>(this);
    connect(m_analyzeWatcher, SIGNAL(resultReadyAt(int)), SLOT(processAnalyzedFile(int)));
    connect(m_analyzeWatcher, SIGNAL(finished()), SLOT(finish()));
	m_networkAccessManager = new QNetworkAccessManager();
	qDebug() << QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
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

static QString extractExtension(const QString &fileName)
{
	int pos = fileName.lastIndexOf('.');
	if (pos == -1) {
		return "";
	}
    return fileName.mid(pos + 1).toUpper();
}

static QString cacheFileName()
{
	return QDesktopServices::storageLocation(QDesktopServices::CacheLocation) + "/acoustid-fingerprinter.log";
}

static QSet<QString> readCacheFile()
{
	QString fileName = cacheFileName();
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Couldn't open cache file" << fileName << "for reading";
		return QSet<QString>();
	}
	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	QSet<QString> result;
	while (true) {
		QString fileName = stream.readLine();
		if (fileName.isEmpty()) {
			break;
		}
		result.insert(fileName);
	}
	return result;
}

static QStringList filterFileList(const QStringList &fileNames)
{
    QSet<QString> allowedExtensions = QSet<QString>()
        << "MP3"
        << "MP4"
        << "M4A"
        << "FLAC"
        << "OGG"
        << "OGA"
        << "APE"
        << "OGGFLAC"
        << "TTA"
        << "WV"
        << "MPC"
        << "WMA";

	QStringList result;
	QSet<QString> cache = readCacheFile();
	foreach (QString fileName, fileNames) {
		if (!allowedExtensions.contains(extractExtension(fileName))) {
			continue;
		}
		if (cache.contains(fileName)) {
			qDebug() << "Already processed" << fileName;
			continue;
		}
		result.append(fileName);
	}
	return result;
}

void Fingerprinter::processFileList()
{
    if (m_fileListWatcher->isCanceled()) {
        emit mainStatusChanged(tr("Canceled"));
		m_finished = true;
		emit finished();
        return;
    }

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
                m_files.append(fileInfo.filePath());
            }
        }
    }
    if (!directories.isEmpty()) {
        startReadingFileList(directories);
    }
    else {
		QFuture<QStringList> future = QtConcurrent::run(&filterFileList, m_files);
		m_filterFileListWatcher = new QFutureWatcher<QStringList>();
		m_filterFileListWatcher->setFuture(future);
		connect(m_filterFileListWatcher, SIGNAL(finished()), SLOT(processFilteredFileList()));
    }
}

void Fingerprinter::processFilteredFileList()
{
    QFuture<QStringList> future = m_filterFileListWatcher->future();

	m_files = future.result();
	startFingerprintingFiles();

	emit mainStatusChanged(tr("Fingerprinting..."));
	emit fileListLoaded(m_files.size());

	//delete m_filterFileListWatcher;
//	m_filterFileListWatcher = 0;
}

QByteArray Fingerprinter::prepareSubmitData()
{
	QUrl url;
	//url.addQueryItem("user", apiKey);
	url.addQueryItem("user", "aaa");
	url.addQueryItem("client", "aaa");
	for (int i = 0; i < m_submitQueue.size(); i++) {
		AnalyzeResult *result = m_submitQueue.at(i);
		url.addQueryItem(QString("length.%1").arg(i), QString::number(result->length));
		url.addQueryItem(QString("mbid.%1").arg(i), result->mbid);
		url.addQueryItem(QString("fingerprint.%1").arg(i), result->fingerprint);
		QString format = extractExtension(result->fileName);
		if (!format.isEmpty()) {
			url.addQueryItem(QString("format.%1").arg(i), format);
		}
		if (result->bitrate) {
			url.addQueryItem(QString("bitrate.%1").arg(i), QString::number(result->bitrate));
		}
	}
	return url.encodedQuery();
}

QNetworkRequest Fingerprinter::prepareSubmitRequest()
{
	QNetworkRequest request(QUrl::fromEncoded("http://127.0.0.1:8080/submit"));
	//QNetworkRequest request(QUrl::fromEncoded("http://api.acoustid.org/submit"));
	return request;
}

struct UpdateCacheTask: public QRunnable
{

	UpdateCacheTask(const QStringList &fileNames)
		: m_fileNames(fileNames)
	{
	}

	void run()
	{
		qDebug() << "updating cache";
		QMutexLocker locker(&m_mutex);
		QString fileName = cacheFileName();
		QFile file(fileName);
		if (!file.open(QIODevice::Append)) {
			qCritical() << "Couldn't open cache file" << fileName << "for writing";
			return;
		}
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		foreach (QString fileName, m_fileNames) {
			stream << fileName << "\n";
		}
	}

	QStringList m_fileNames;
	static QMutex m_mutex;
};

QMutex UpdateCacheTask::m_mutex;

bool Fingerprinter::maybeSubmitQueue(bool force)
{
	int queueSize = m_submitQueue.size();
	if (queueSize >= 10 || (force && queueSize > 0)) {
		qDebug() << "Submitting" << m_submitQueue.size() << "items";
		m_networkAccessManager->post(prepareSubmitRequest(), prepareSubmitData());
		qDebug() << "Data:" << prepareSubmitData();
        /*qDebug() << "\n" << result->fileName;
        qDebug() << " > MBID " << result->mbid;
        qDebug() << " > Length " << result->length;
        qDebug() << " > Bitrate " << result->bitrate;
        qDebug() << " > Fingerprint " << result->fingerprint;*/
		QStringList fileNames;
		foreach (AnalyzeResult *result, m_submitQueue) {
			fileNames.append(result->fileName);
		}
		qDeleteAll(m_submitQueue);
		m_submitQueue.clear();
		UpdateCacheTask *task = new UpdateCacheTask(fileNames);
		task->setAutoDelete(true);
		QThreadPool::globalInstance()->start(task, 100);
		return true;
	}
	return false;
}

void Fingerprinter::processAnalyzedFile(int index)
{
    AnalyzeResult *result = m_analyzeWatcher->future().resultAt(index);
    if (!result->error) {
        qDebug() << "Finished " << result->fileName;
		m_submitQueue.append(result);
		maybeSubmitQueue();
    }
	else {
        qDebug() << "Error " << result->fileName << "(" << result->errorMessage << ")";
		delete result;
	}
    emit fileProcessed(++m_counter);
}

void Fingerprinter::finish()
{
    if (m_analyzeWatcher->isCanceled()) {
        emit mainStatusChanged(tr("Canceled"));
		m_finished = true;
		emit finished();
    }
    else {
		if (maybeSubmitQueue(true)) {
			emit mainStatusChanged(tr("Submitting"));
		}
		else {
			emit mainStatusChanged(tr("Finished"));
			m_finished = true;
			emit finished();
		}
    }
}

