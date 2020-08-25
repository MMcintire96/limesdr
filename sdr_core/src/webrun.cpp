#include "../include/limeSDR.h"
#include "../include/aoPlayer.h"
#include "../include/Controller.h"
#include "../include/utils.h"

#include <string.h>
#include <fstream>
#include <iostream>

#include <map>
#include <vector>

#include "math.h"
#include <iomanip>
#include <complex>

#include <sys/stat.h>


std::string configFile = "/tmp/sdr-app-info";
std::string updateFile = "/tmp/sdr-app-update";
struct stat result;
long lastReadTime = 0;

std::string parseCommand(limeSDR sdr, std::string key, std::string value) {
  std::string rValue = "";
  if (key == "frequency") {
    sdr.setFreq(std::stof(value));
    return rValue;
  }
  if (key == "gain") {
    sdr.setGain(std::stof(value));
    return rValue;
  }
  if (key == "sampleRate") {
    sdr.setSampleRate(std::stoi(value) * 16, 0);
    return rValue;
  }
  if (key == "bandwidth") {
    sdr.setFIRFilter(std::stof(value));
    return rValue;
  }
  return rValue;
}

void readController(limeSDR sdr, Controller &c) {
  std::string pipeData = ""; //becomes value
  c.readPipe(&pipeData);
  if (pipeData.length() == 0) {
    return;
  }
  std::string key = getKey(pipeData, ":");
  std::string rValue = parseCommand(sdr, key, pipeData);
}

void updateStats(std::map<std::string, std::string> stats) {
  std::ofstream outFile(configFile);
  stats["mode"] = "rx";
  stats["frequency"] = "104.3e6";
  stats["gain"] = "1.0";
  stats["bandwidth"] = "200e3";
  stats["sampleRate"] = "44100";
  stats["overSampleRate"] = "16";
  for (auto const& x : stats) {
    outFile << x.first << " : " << x.second << std::endl;
  }
  outFile.close();
}

// read from a file with key : value seperated information
std::map<std::string, std::string> getStats() {
  std::map<std::string, std::string> stats;

  std::fstream file;
  file.open(configFile);

  std::string val;
  std::string delimiter = " : ";
  std::vector<std::string> str_copy;
  while (getline(file, val)) {
    std::string key;
    std::size_t pos;
    while ((pos = val.find(delimiter)) != std::string::npos) {
        key = val.substr(0, pos);
        str_copy.push_back(key);
        val.erase(0, pos + delimiter.length());
    }
    stats[key] = val;
  }
  if (stats.size() < 1) {
    printf("Error, map size too small");
  }
  return stats;
}


void configureSDR(limeSDR sdr, std::map<std::string, std::string> stats) {
  sdr.setRX();
  sdr.setFreq(std::stof(stats["frequency"]));
  sdr.setSampleRate((std::stoi(stats["sampleRate"]) * std::stoi(stats["overSampleRate"])), 0);
  sdr.setGain(std::stof(stats["gain"]));
  sdr.setFIRFilter(std::stof(stats["bandwidth"]));
  stat(updateFile.c_str(), &result);
  lastReadTime = (long)result.st_mtime;
}

void listenOnUpdate(limeSDR sdr) {
  stat(updateFile.c_str(), &result);
  if ((long)result.st_mtime != lastReadTime) {
    std::fstream inFile(updateFile);
    std::string action;
    std::getline(inFile, action);

    inFile.close();

    if (action == "update") {
      std::map<std::string, std::string> stats = getStats();
      configureSDR(sdr, stats);
    }

    std::ofstream outFile(updateFile);
    outFile << "done" << std::endl;
    outFile.close();
    stat(updateFile.c_str(), &result);
    lastReadTime = result.st_mtime;
  }

}

float fmDemod_new(std::complex<float> &sample, std::complex<float> &lastSample, std::complex<float> &v, float demodOut, float filterOut) {
    // FM demodulate
    sample *= 1.0/(0.2 + std::abs(sample));    // limit amplitude to 1
    v = sample * std::conj(lastSample);        // compute phase change vector
    lastSample = sample;                       // remember the this sample
    demodOut = std::imag(v);                   // 'Q' or imaginary part will contain the audio

    // lowpass filter the demod output
    filterOut = 0.98 * filterOut + 0.02 * demodOut;
    return filterOut;
}

void play(limeSDR sdr, Controller &controller) {
  // Initialize data buffers
  aoPlayer player = aoPlayer();
  char *audio_buffer;
  int driver = player.initDefaultPlayer();

  const int sampleCnt = 5000; //complex samples per buffer
  int16_t buffer[sampleCnt * 2]; //buffer to hold complex values (2*samples))

  int buf_size = player.setFormat();
  ao_device *device = player.openPlayer(driver);
  audio_buffer = (char*)calloc(buf_size, sizeof(char));

  int running = 1;
  float I, Q;
  static float demodOut;
  static float filterOut = 0;
  static float decimateCnt = 0;
  std::complex<float> sample;
  static std::complex<float> lastSample(0, 0);
  std::complex<float> v;

  const int overSampleRate = 16;

  int audioSampleCnt = 0;
  int audioGain = 7000;

  while (running) {
    int samplesRead = sdr.getStream(*buffer, sampleCnt);

    //TODO not every sample -> send 1k/sec -> oversampled to 705600
    //fwrite(buffer, sizeof(int16_t), sampleCnt * 2, iqfd);

    for (int i=0; i<samplesRead*2; i+=2) {

      const float gain = 1.0/32;
      I = gain * (float)buffer[i];
      Q = gain * (float)buffer[i+1];
      sample = std::complex<float>(I, Q);

      filterOut = fmDemod_new(sample, lastSample, v, demodOut, filterOut);

      if (++decimateCnt >= overSampleRate) {
        decimateCnt = 0;

        int j = audioSampleCnt;
        int16_t filterOutInt = audioGain * filterOut;
        player.buildBuffer(audio_buffer, j, filterOutInt);

        if (++audioSampleCnt == buf_size/4) {
          readController(sdr, controller);

          //listenOnUpdate(sdr);
          player.play(device, audio_buffer, buf_size);
          audioSampleCnt = 0;
        }
      }
    }
  }
  player.close(device);
  player.shutDown();
}


int main(int argc, char** argv) {
  // if file not written
  //std::map<std::string, std::string> stats;
  //updateStats(stats);

  std::map<std::string, std::string> stats = getStats();
  limeSDR sdr = limeSDR();
  configureSDR(sdr, stats);
  listenOnUpdate(sdr);
  Controller controller;
  controller.start();

  sdr.initStream();
  sdr.startStream();

  play(sdr, controller);

  controller.stop();
  sdr.stopStream();
  sdr.close();
}
