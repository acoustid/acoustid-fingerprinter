#ifndef FPSUBMIT_FINGERPRINTCALCULATOR_H_
#define FPSUBMIT_FINGERPRINTCALCULATOR_H_

#include <QString>
#include <QMutex>
#include <chromaprint.h>

class FingerprintCalculator {

public:
    FingerprintCalculator();
    ~FingerprintCalculator();

    bool start(int sampleRate, int numChannels);
    void feed(qint16 *data, int size);
    QString finish();

private:
    ChromaprintContext *m_context;    
    static QMutex m_mutex;
};

#endif
