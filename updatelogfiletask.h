#ifndef FPSUBMIT_UPDATELOGFILETASK_H_
#define FPSUBMIT_UPDATELOGFILETASK_H_

#include <QRunnable>
#include <QStringList>

class UpdateLogFileTask : public QRunnable
{
public:
	UpdateLogFileTask(const QStringList &files);
	void run();

private:
	QStringList m_files;
};

#endif
