#include <QDir>
#include <QFile>
#include <QDesktopServices>
#include <QSet>
#include <QDebug>
#include "utils.h"
#include "loadfilelisttask.h"

LoadFileListTask::LoadFileListTask(const QStringList &directories)
	: m_directories(removeDuplicateDirectories(directories))
{
}

QStringList LoadFileListTask::removeDuplicateDirectories(const QStringList &directories)
{
    int directoryCount = directories.size();
    QList<QString> sortedDirectories;
    for (int i = 0; i < directoryCount; i++) {
        sortedDirectories.append(QDir(directories.at(i)).canonicalPath() + QDir::separator());
    }
    qSort(sortedDirectories);
    QStringList result;
    result.append(sortedDirectories.first());
    for (int j = 0, i = 1; i < directoryCount; i++) {
        QString path = sortedDirectories.at(i);
        if (!path.startsWith(result.at(j))) {
            result.append(path);
            j++;
        }
    }
	return result;
}

void LoadFileListTask::processFile(const QString &path)
{
    static QSet<QString> allowedExtensions = QSet<QString>()
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

	if (allowedExtensions.contains(extractExtension(path))) {
		if (!m_cache.contains(path)) {
			m_files.append(path);
		}
	}
}

void LoadFileListTask::processDirectory(const QString &path)
{
	emit currentPathChanged(path);
    QFileInfoList fileInfoList = QDir(path).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
	for (int j = 0; j < fileInfoList.size(); j++) {
		QFileInfo fileInfo = fileInfoList.at(j);
		if (fileInfo.isDir()) {
			processDirectory(fileInfo.filePath());
		}
		else {
			processFile(fileInfo.filePath());
		}
    }
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

void LoadFileListTask::run()
{
	m_cache = readCacheFile();
	foreach (QString path, m_directories) {
		processDirectory(path);
	}
	emit finished(m_files);
}

