#include <QDebug>
#include <QDir>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QtConcurrentRun>
#include <QDesktopServices>
#include <QMutexLocker>
#include "loadfilelisttask.h"
#include "analyzefiletask.h"
#include "updatelogfiletask.h"
#include "fingerprinter.h"
#include "constants.h"
#include "utils.h"
#include "gzip.h"

Fingerprinter::Fingerprinter(const QString &apiKey, const QStringList &directories)
    : m_apiKey(apiKey), m_directories(directories), m_paused(false), m_cancelled(false),
	  m_finished(false), m_reply(0), m_activeFiles(0), m_fingerprintedFiles(0), m_submittedFiles(0)
{
	m_networkAccessManager = new QNetworkAccessManager(this);
	connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply *)), SLOT(onRequestFinished(QNetworkReply*)));
}

Fingerprinter::~Fingerprinter()
{
}

void Fingerprinter::start()
{
	m_time.start();
	LoadFileListTask *task = new LoadFileListTask(m_directories);
	connect(task, SIGNAL(finished(const QStringList &)), SLOT(onFileListLoaded(const QStringList &)));
	connect(task, SIGNAL(currentPathChanged(const QString &)), SIGNAL(currentPathChanged(const QString &)));
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
	connect(task, SIGNAL(finished(AnalyzeResult *)), SLOT(onFileAnalyzed(AnalyzeResult *)));
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
		QUrl url;
		url.addQueryItem("user", m_apiKey);
		url.addQueryItem("client", CLIENT_API_KEY);
		for (int i = 0; i < size; i++) {
			AnalyzeResult *result = m_submitQueue.takeFirst();
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
			m_submitting.append(result->fileName);
			delete result;
		}
		QNetworkRequest request = QNetworkRequest(QUrl::fromEncoded(SUBMIT_URL));
		request.setRawHeader("Content-Encoding", "gzip");
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
