#include "limeSDR.h"
#include "aoPlayer.h"
#include "Controller.h"
#include "utils.h"
#include "signals.h"

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
  stats["gain"] = "0.5";
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
    
    // here we create two types of demods and assign one to be the "Demod In Use"
    FM_Demod fm_Demod;
    OOK_Demod ook_Demod;
    BaseClass_Demod* demodInUse;
    demodInUse = &fm_Demod;
    
  float filterOut;

  static float decimateCnt = 0;
  std::complex<float> sample;

  const int overSampleRate = 16;

  int audioSampleCnt = 0;
  int audioGain = 7000;

  while (running) {
    int samplesRead = sdr.getStream(*buffer, sampleCnt);

    for (int i=0; i<samplesRead*2; i+=2) {

      const float gain = 1.0/32;
      I = gain * (float)buffer[i];
      Q = gain * (float)buffer[i+1];
      sample = std::complex<float>(I, Q);

      filterOut = demodInUse->process(sample);

      if (++decimateCnt >= overSampleRate) {
        decimateCnt = 0;

        int j = audioSampleCnt;
        int16_t filterOutInt = audioGain * filterOut;
        player.buildBuffer(audio_buffer, j, filterOutInt);

        if (++audioSampleCnt == buf_size/4) {
          readController(sdr, controller);

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
  std::map<std::string, std::string> stats;
  updateStats(stats);

  stats = getStats();

  limeSDR sdr = limeSDR();
  configureSDR(sdr, stats);

  Controller controller;
  controller.start();

  sdr.initStream();
  sdr.startStream();

  play(sdr, controller);

  controller.stop();
  sdr.stopStream();
  sdr.close();
}
