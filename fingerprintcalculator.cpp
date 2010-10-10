#include <stdlib.h>
#include "fingerprintcalculator.h"

FingerprintCalculator::FingerprintCalculator()
{
    m_context = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);
}

FingerprintCalculator::~FingerprintCalculator()
{
    chromaprint_free(m_context);
}

void FingerprintCalculator::start(int sampleRate, int numChannels)
{
    chromaprint_start(m_context, sampleRate, numChannels);
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

