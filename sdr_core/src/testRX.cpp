#include "limeSDR.h"
#include "aoPlayer.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "math.h"
#include <iomanip>
#include <complex>
#include <cmath>

class FM_Demod
{
public:
    float process(std::complex<float> &sample) {
        // FM demodulate
        std::complex<float> v;
        sample *= 1.0/(0.2 + std::abs(sample));    // limit amplitude to 1
        v = sample * std::conj(lastSample);        // compute phase change vector
        lastSample = sample;                       // remember the this sample
        demodOut = std::imag(v);                   // 'Q' or imaginary part will contain the audio

        // lowpass filter the demod output
        filterOut = 0.98 * filterOut + 0.02 * demodOut;
        return filterOut;
    }
    
    float demodOut = 0;
    float filterOut = 0;
    std::complex<float> lastSample;
};

//TODO fifo block (running) untill a byte is read -- fix

int running = 0;

int run(int argc, char** argv) {
  limeSDR sdr = limeSDR();

  float sdr_freq = 104.3e6;
  float sdr_gain = 0.5;
  float sdr_FIRFilter = 200e3; // bandwidth

  aoPlayer player = aoPlayer();
  char *audio_buffer;
  int driver = player.initDefaultPlayer();

  sdr.setRX();
  sdr.setFreq(sdr_freq);

  int audioGain = 7000;

//  const int audioSampleRate = 44100;
  const int overSampleRate = 16;
//  double sampleRate = (audioSampleRate * overSampleRate);

  //sdr.setSampleRate(sampleRate);
  sdr.setGain(sdr_gain);
  sdr.setFIRFilter(sdr_FIRFilter); //200e3 for wfm

  sdr.initStream();
  sdr.startStream();

  // Initialize data buffers
  const int sampleCnt = 5000; //complex samples per buffer
  int16_t buffer[sampleCnt * 2]; //buffer to hold complex values (2*samples))

  int buf_size = player.setFormat();
  ao_device *device = player.openPlayer(driver);
  audio_buffer = (char*)calloc(buf_size, sizeof(char));

  running = 1;
  float I, Q;
  FM_Demod fm_Demod;
  float filterOut;
  
  static float decimateCnt = 0;
  std::complex<float> sample;

  int audioSampleCnt = 0;

  std::string iqfifo = "/tmp/limesdr-iq-fifo";
  mkfifo(iqfifo.c_str(), 0666);
  FILE *iqfd = fopen(iqfifo.c_str(), "wb");

//  char freqBuffer[11];

  while (running) {
    int samplesRead = sdr.getStream(*buffer, sampleCnt);

    //TODO not every sample -> send 1k/sec -> oversampled to 705600
    fwrite(buffer, sizeof(int16_t), sampleCnt * 2, iqfd);

    for (int i=0; i<samplesRead*2; i+=2) {

      const float gain = 1.0/32;
      I = gain * (float)buffer[i];
      Q = gain * (float)buffer[i+1];
      sample = std::complex<float>(I, Q);

      filterOut = fm_Demod.process(sample);

      if (++decimateCnt >= overSampleRate) {
        decimateCnt = 0;

        int j = audioSampleCnt;
        int16_t filterOutInt = audioGain * filterOut;
        player.buildBuffer(audio_buffer, j, filterOutInt);

        if (++audioSampleCnt == buf_size/4) {
          player.play(device, audio_buffer, buf_size);
          audioSampleCnt = 0;
        }
      }
    }


    //debounc this for sure, sdr setFreq should return success, then change freq
    //if (read(fd, freqBuffer, sizeof(freqBuffer)) > 0) {
    //  float nFreq = std::stof(freqBuffer) * 1000000;
    //  if (nFreq != freq) {
    //    sdr.setFreq(nFreq);
    //    freq = nFreq;
    //  }
    //}

  }

  sdr.stopStream();

  player.close(device);
  player.shutDown();

  sdr.close();
  return 0;
}
