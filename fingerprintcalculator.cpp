#include <stdlib.h>
#include "fingerprintcalculator.h"

QMutex FingerprintCalculator::m_mutex;

FingerprintCalculator::FingerprintCalculator()
{
    QMutexLocker locker(&m_mutex);
    m_context = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);
}

FingerprintCalculator::~FingerprintCalculator()
{
    QMutexLocker locker(&m_mutex);
    chromaprint_free(m_context);
}

bool FingerprintCalculator::start(int sampleRate, int numChannels)
{
    return chromaprint_start(m_context, sampleRate, numChannels);
}

void FingerprintCalculator::feed(qint16 *data, int size)
{
    chromaprint_feed(m_context, data, size);
}

QString FingerprintCalculator::finish()
{
    char *fingerprint;

    chromaprint_finish(m_context);
    chromaprint_get_fingerprint(m_context, &fingerprint);

    QString result(fingerprint);
    free(fingerprint);

    return result;
}

