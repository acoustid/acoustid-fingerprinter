#include <QDebug>
#include "decoder.h"
#include "tagreader.h"
#include "utils.h"
#include "analyzefiletask.h"
#include "constants.h"

AnalyzeFileTask::AnalyzeFileTask(const QString &path)
	: m_path(path)
{
}

void AnalyzeFileTask::run()
{
    qDebug() << "Analyzing file" << m_path;

    AnalyzeResult *result = new AnalyzeResult();
    result->fileName = m_path;

    TagReader tags(m_path);
    if (!tags.read()) {
        result->error = true;
        result->errorMessage = "Couldn't read metadata";
		emit finished(result);
        return;
    }

    result->mbid = tags.mbid();
    result->length = tags.length();
    result->bitrate = tags.bitrate();
    if (result->length < 10) {
        result->error = true;
        result->errorMessage = "Too short audio stream, should be at least 10 seconds";
		emit finished(result);
        return;
    }

    Decoder decoder(qPrintable(m_path));
    if (!decoder.Open()) {
        result->error = true;
        result->errorMessage = QString("Couldn't open the file: ") + QString::fromStdString(decoder.LastError());
		emit finished(result);
        return;
    }

    FingerprintCalculator fpcalculator;
    if (!fpcalculator.start(decoder.SampleRate(), decoder.Channels())) {
        result->error = true;
        result->errorMessage = "Error while fingerpriting the file";
		emit finished(result);
        return;
	}
    decoder.Decode(&fpcalculator, AUDIO_LENGTH);
    result->fingerprint = fpcalculator.finish();

	emit finished(result);
}

