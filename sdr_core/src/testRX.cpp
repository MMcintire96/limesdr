#include "../include/limeSDR.h"
#include "../include/aoPlayer.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "math.h"
#include <iomanip>
#include <complex>
#include <cmath>


float fmDemod(std::complex<float> &sample, std::complex<float> &lastSample, std::complex<float> &v, float demodOut, float filterOut) {
    // FM demodulate
    sample *= 1.0/(0.2 + std::abs(sample));    // limit amplitude to 1
    v = sample * std::conj(lastSample);        // compute phase change vector
    lastSample = sample;                       // remember the this sample
    demodOut = std::imag(v);                   // 'Q' or imaginary part will contain the audio

    // lowpass filter the demod output
    filterOut = 0.98 * filterOut + 0.02 * demodOut;
    return filterOut;
}


int running = 0;

int main(int argc, char** argv) {

  limeSDR sdr = limeSDR();
  aoPlayer player = aoPlayer();
  char *audio_buffer;
  int driver = player.initDefaultPlayer();

  sdr.setRX();
  float freq = 104.3e6;
  sdr.setFreq(freq);

  int audioGain = 7000;

  const int audioSampleRate = 44100;
  const int overSampleRate = 16;
  double sampleRate = (audioSampleRate * overSampleRate);

  sdr.setSampleRate(sampleRate);
  sdr.setGain(1.00);
  sdr.setFIRFilter(200e3); //200e3 for wfm

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
  static float demodOut;
  static float filterOut = 0;
  static float decimateCnt = 0;
  std::complex<float> sample;
  static std::complex<float> lastSample(0, 0);
  std::complex<float> v;

  int audioSampleCnt = 0;

  //int fd;
  //std::string fifoLoc = "/tmp/limesdr-fifo";
  //mkfifo(fifoLoc.c_str(), 0666);
  //fd = open(fifoLoc.c_str(), O_RDONLY);
  //if (fd == -1) return -1;

  int iqfd;
  std::string iqfifo = "/tmp/limesdr-iq-fifo";
  mkfifo(iqfifo.c_str(), 0666);
  iqfd = open(iqfifo.c_str(), O_WRONLY);

  //if (iqfd == -1) return -1;

  char freqBuffer[11];


  while (running) {
    int samplesRead = sdr.getStream(*buffer, sampleCnt);

    for (int i=0; i<samplesRead*2; i+=2) {

      const float gain = 1.0/32;
      I = gain * (float)buffer[i];
      Q = gain * (float)buffer[i+1];
      sample = std::complex<float>(I, Q);

      filterOut = fmDemod(sample, lastSample, v, demodOut, filterOut);

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
