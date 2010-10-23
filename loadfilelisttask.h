#ifndef FPSUBMIT_LOADFILELISTTASK_H_
#define FPSUBMIT_LOADFILELISTTASK_H_

#include <QRunnable>
#include <QObject>
#include <QSet>
#include <QStringList>

class LoadFileListTask : public QObject, public QRunnable
{
	Q_OBJECT

public:
	LoadFileListTask(const QStringList &directories);
	void run();

	QStringList files() const { return m_files; }

signals:
	void finished(const QStringList &files);
	void currentPathChanged(const QString &path);

private:
	static QStringList removeDuplicateDirectories(const QStringList &directories);
	void processDirectory(const QString &path);
	void processFile(const QString &path);

	QSet<QString> m_cache;
	QStringList m_directories;
	QStringList m_files;
};

#endif
