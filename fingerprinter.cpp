#include <QDebug>
#include <QDir>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>
#include <QDesktopServices>
#include <QMutexLocker>
#include <QThreadPool>
#include "loadfilelisttask.h"
#include "analyzefiletask.h"
#include "updatelogfiletask.h"
#include "fingerprinter.h"
#include "constants.h"
#include "utils.h"
#include "gzip.h"

class NetworkProxyFactory : public QNetworkProxyFactory
{
public:
	NetworkProxyFactory() : m_httpProxy(QNetworkProxy::NoProxy)
	{
		char* httpProxyUrl = getenv("http_proxy");
		if (httpProxyUrl) {
			QUrl url = QUrl::fromEncoded(QByteArray(httpProxyUrl));
			if (url.isValid() && !url.host().isEmpty()) {
				m_httpProxy = QNetworkProxy(QNetworkProxy::HttpProxy, url.host(), url.port(80));
				if (!url.userName().isEmpty())	{
					m_httpProxy.setUser(url.userName());
					if (!url.password().isEmpty()) {
						m_httpProxy.setPassword(url.password());
					}
				}
			}
		}
	}

	QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery& query)
	{
		QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(query);
		QString protocol = query.protocolTag().toLower();
		if (m_httpProxy.type() != QNetworkProxy::NoProxy && protocol == QLatin1String("http")) {
			QMutableListIterator<QNetworkProxy> i(proxies);
			while (i.hasNext()) {
				QNetworkProxy& proxy = i.next();
				if (proxy.type() == QNetworkProxy::NoProxy) {
					i.remove();
				}
			}
			proxies.append(m_httpProxy);
			proxies.append(QNetworkProxy::NoProxy);
		}
		return proxies;
	}

private:
	QNetworkProxy m_httpProxy;
};

Fingerprinter::Fingerprinter(const QString &apiKey, const QStringList &directories)
    : m_apiKey(apiKey), m_directories(directories), m_paused(false), m_cancelled(false),
	  m_finished(false), m_reply(0), m_activeFiles(0), m_fingerprintedFiles(0), m_submittedFiles(0)
{
	m_networkAccessManager = new QNetworkAccessManager(this);
	m_networkAccessManager->setProxyFactory(new NetworkProxyFactory());
	connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply *)), SLOT(onRequestFinished(QNetworkReply*)));
}

Fingerprinter::~Fingerprinter()
{
}

void Fingerprinter::start()
{
	m_time.start();
	LoadFileListTask *task = new LoadFileListTask(m_directories);
	connect(task, SIGNAL(finished(const QStringList &)), SLOT(onFileListLoaded(const QStringList &)), Qt::QueuedConnection);
	connect(task, SIGNAL(currentPathChanged(const QString &)), SIGNAL(currentPathChanged(const QString &)), Qt::QueuedConnection);
	task->setAutoDelete(true);
	emit fileListLoadingStarted();
	QThreadPool::globalInstance()->start(task);
}

void Fingerprinter::pause()
{
	m_paused = true;
}

void Fingerprinter::resume()
{
	m_paused = false;
	while (!m_files.isEmpty() && m_activeFiles < MAX_ACTIVE_FILES) {
		fingerprintNextFile();
	}
	maybeSubmit();
}

void Fingerprinter::cancel()
{
	m_cancelled = true;
	m_files.clear();
	m_submitQueue.clear();
	if (m_reply) {
		m_reply->abort();
	}
}

bool Fingerprinter::isPaused()
{
	return m_paused;
}

bool Fingerprinter::isCancelled()
{
	return m_cancelled;
}

bool Fingerprinter::isFinished()
{
	return m_finished;
}

bool Fingerprinter::isRunning()
{
	return !isPaused() && !isCancelled() && !isFinished();
}

void Fingerprinter::onFileListLoaded(const QStringList &files)
{
	m_files = files;
	if (m_files.isEmpty()) {
		m_finished = true;
		emit noFilesError();
		emit finished();
		return;
	}
	emit fingerprintingStarted(files.size());
	while (!m_files.isEmpty() && m_activeFiles < MAX_ACTIVE_FILES) {
		fingerprintNextFile();
	}
}

void Fingerprinter::fingerprintNextFile()
{
	if (m_files.isEmpty()) {
		return;
	}
	m_activeFiles++;
	QString path = m_files.takeFirst();
	emit currentPathChanged(path);
	AnalyzeFileTask *task = new AnalyzeFileTask(path);
	connect(task, SIGNAL(finished(AnalyzeResult *)), SLOT(onFileAnalyzed(AnalyzeResult *)), Qt::QueuedConnection);
	task->setAutoDelete(true);
	QThreadPool::globalInstance()->start(task);
}

void Fingerprinter::onFileAnalyzed(AnalyzeResult *result)
{
	m_activeFiles--;
	emit progress(++m_fingerprintedFiles);
	if (!result->error) {
		if (!isCancelled()) {
			m_submitQueue.append(result);
		}
		if (isRunning()) {
			maybeSubmit();
		}
	}
	else {
		qDebug() << "Error" << result->errorMessage << "while processing" << result->fileName;
	}
	if (isRunning()) {
		fingerprintNextFile();
	}
	if (m_activeFiles == 0 && m_files.isEmpty()) {
		if (m_submitQueue.isEmpty()) {
			m_finished = true;
			emit finished();
			return;
		}
		maybeSubmit(true);
	}
}

bool Fingerprinter::maybeSubmit(bool force)
{
	int size = qMin(MAX_BATCH_SIZE, m_submitQueue.size());
	if (!m_reply && (size >= MIN_BATCH_SIZE || (force && size > 0))) {
		qDebug() << "Submitting" << size << "fingerprints";
		QUrl url;
		url.addQueryItem("user", m_apiKey);
		url.addQueryItem("client", CLIENT_API_KEY);
		for (int i = 0; i < size; i++) {
			AnalyzeResult *result = m_submitQueue.takeFirst();
			qDebug() << "  " << result->mbid;
			url.addQueryItem(QString("duration.%1").arg(i), QString::number(result->length));
			if (!result->puid.isEmpty()) {
				url.addQueryItem(QString("puid.%1").arg(i), result->puid);
			}
			if (!result->mbid.isEmpty()) {
				url.addQueryItem(QString("mbid.%1").arg(i), result->mbid);
			}
			if (!result->track.isEmpty()) {
				url.addQueryItem(QString("track.%1").arg(i), result->track);
			}
			if (!result->artist.isEmpty()) {
				url.addQueryItem(QString("artist.%1").arg(i), result->artist);
			}
			if (!result->album.isEmpty()) {
				url.addQueryItem(QString("album.%1").arg(i), result->album);
			}
			if (!result->albumArtist.isEmpty()) {
				url.addQueryItem(QString("albumartist.%1").arg(i), result->albumArtist);
			}
			if (result->year) {
				url.addQueryItem(QString("year.%1").arg(i), QString::number(result->year));
			}
			if (result->trackNo) {
				url.addQueryItem(QString("trackno.%1").arg(i), QString::number(result->trackNo));
			}
			if (result->discNo) {
				url.addQueryItem(QString("discno.%1").arg(i), QString::number(result->discNo));
			}
			url.addQueryItem(QString("fingerprint.%1").arg(i), result->fingerprint);
			QString format = extractExtension(result->fileName);
			if (!format.isEmpty()) {
				url.addQueryItem(QString("fileformat.%1").arg(i), format);
			}
			if (result->bitrate) {
				url.addQueryItem(QString("bitrate.%1").arg(i), QString::number(result->bitrate));
			}
			m_submitting.append(result->fileName);
			delete result;
		}
		qDebug() << url.encodedQuery();
		QNetworkRequest request = QNetworkRequest(QUrl::fromEncoded(SUBMIT_URL));
		request.setRawHeader("Content-Encoding", "gzip");
		request.setRawHeader("User-Agent", userAgentString().toAscii());
		m_reply = m_networkAccessManager->post(request, gzipCompress(url.encodedQuery()));
		return true;
	}
	return false;
}

void Fingerprinter::onRequestFinished(QNetworkReply *reply)
{
	bool stop = false;
	QNetworkReply::NetworkError error = reply->error();

	if (m_cancelled) {
		stop = true;
	}
	else if (error == QNetworkReply::UnknownContentError) {
		QString errorMessage = reply->readAll();
		if (errorMessage.contains("User with the API key")) {
			emit authenticationError();
			stop = true;
		}
		else {
			qWarning() << "Submittion failed:" << errorMessage;
		}
	}
	else if (error != QNetworkReply::NoError) {
		qWarning() << "Submission failed with network error" << error;
		emit networkError(reply->errorString());
		stop = true;
	}

	if (!stop && error == QNetworkReply::NoError) {
		m_submitted.append(m_submitting);
		m_submittedFiles += m_submitting.size();
		maybeSubmit();
		qDebug() << "Submission finished"; 
	}

	if (m_submitted.size() > 0) {
		UpdateLogFileTask *task = new UpdateLogFileTask(m_submitted);
		task->setAutoDelete(true);
		QThreadPool::globalInstance()->start(task);
		m_submitted.clear();
	}

	m_submitting.clear();
	reply->deleteLater();
	m_reply = 0;

	if (m_submitQueue.isEmpty() && m_files.isEmpty()) {
		m_finished = true;
		emit finished();
		return;
	}

	if (isRunning()) {
		maybeSubmit(m_files.isEmpty());
	}
}
