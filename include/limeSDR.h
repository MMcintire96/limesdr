#include "lime/LimeSuite.h"

class limeSDR {
  public:
    limeSDR();
    int error();
    void setRX();
    void setFreq(float freq);
    void setGain(float gain);
    void listAntenna();
    void setAntenna(int antenna);
    void setSampleRate(double sampleRate);
    void setFIRFilter();
    void initStream();
    void startStream();
    int getStream(int16_t &buffer, int sampleCnt);
    void stopStream();
    void close();
};
