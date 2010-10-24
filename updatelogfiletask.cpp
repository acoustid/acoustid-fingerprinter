#include <QDir>
#include <QFile>
#include <QDebug>
#include "utils.h"
#include "updatelogfiletask.h"

UpdateLogFileTask::UpdateLogFileTask(const QStringList &files)
	: m_files(files)
{
}

void UpdateLogFileTask::run()
{
	QString fileName = cacheFileName();
	qDebug() << "Updating" << fileName;
	QDir().mkpath(QDir::cleanPath(fileName + "/.."));
	QFile file(fileName);
	if (!file.open(QIODevice::Append)) {
		qCritical() << "Couldn't open cache file" << fileName << "for writing";
		return;
	}
	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	foreach (QString fileName, m_files) {
		stream << fileName << "\n";
	}
}

