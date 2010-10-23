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

    Decoder decoder(qPrintable(m_path));
    if (!decoder.Open()) {
        result->error = true;
        result->errorMessage = "Couldn't open the file";
		emit finished(result);
        return;
    }

    FingerprintCalculator fpcalculator;
    fpcalculator.start(decoder.SampleRate(), decoder.Channels());
    decoder.Decode(&fpcalculator, AUDIO_LENGTH);
    result->fingerprint = fpcalculator.finish();

	emit finished(result);
}

