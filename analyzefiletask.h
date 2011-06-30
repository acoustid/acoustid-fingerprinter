#ifndef FPSUBMIT_ANALYZEFILETASK_H_
#define FPSUBMIT_ANALYZEFILETASK_H_

#include <QRunnable>
#include <QObject>
#include <QStringList>

struct AnalyzeResult
{
    AnalyzeResult() : error(false)
    {
    }

    QString fileName;
    QString mbid;
    QString fingerprint;
	QString track;
	QString artist;
	QString album;
	QString albumArtist;
	QString puid;
	int trackNo;
	int discNo;
	int year;
    int length;
    int bitrate;
    bool error;
    QString errorMessage;
};

class AnalyzeFileTask : public QObject, public QRunnable
{
	Q_OBJECT

public:
	AnalyzeFileTask(const QString &path);
	void run();

signals:
	void finished(AnalyzeResult *result);

private:
	QString m_path;
};

#endif
