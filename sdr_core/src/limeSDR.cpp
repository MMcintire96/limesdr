#include "limeSDR.h"
#include <iostream>
#include <chrono>

lms_device_t* device = NULL;
lms_stream_t streamId;

limeSDR::limeSDR() {
  int n;
  lms_info_str_t list[8];
  if ((n = LMS_GetDeviceList(list)) < 0) error();
  std::cout << "Devices found: " << n << std::endl;
  if (n < 1) return;
  if (LMS_Open(&device, list[0], NULL)) error();
  if (LMS_Init(device) != 0) error();
}


int limeSDR::error() {
  if (device != NULL)
    LMS_Close(device);
  exit(-1);
}

void limeSDR::setRX() {
  // Enable RX channel
  // Channels are numbered starting at 0
  if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
    error();
}

void limeSDR::setFreq(float freq) {
  // Set center frequency to FM station desired (in Hz)
  if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, freq) != 0)
    error();
}

void limeSDR::setSampleRate(double sampleRate, int overSampleRate) {
  if (LMS_SetSampleRate(device, sampleRate, overSampleRate) != 0)
    error();
}

void limeSDR::setGain(float gain) {
  if (LMS_SetNormalizedGain(device, false, LMS_CH_RX, gain) != 0)
    error();
}

void limeSDR::listAntenna() {
  lms_name_t list[5];
  if (LMS_GetAntennaList(device, false, LMS_CH_RX, list) == -1)
    error();
  LMS_GetAntennaList(device, false, LMS_CH_RX, list);
  for (int i = 0; i<5; i++) {
    std::cout << list[i] << std::endl;
  }
}

void limeSDR::setAntenna(int antenna) {
  if (LMS_SetAntenna(device, false, LMS_CH_RX, antenna) == 01)
    error();
}

void limeSDR::setFIRFilter(float bandwidth) {
  // Set the FIR filter bandwidth to 200 kHz
  if (LMS_SetGFIRLPF(device, false, LMS_CH_RX, true, bandwidth) != 0)
    error();
}

void limeSDR::initStream() {
  // Initialize the SDR stream
  streamId.channel = 0; //channel number
  streamId.fifoSize = 1024 * 1024; //fifo size in samples
  streamId.throughputVsLatency = 1.0; //optimize for max throughput
  streamId.isTx = false; //RX channel
  streamId.dataFmt = lms_stream_t::LMS_FMT_I12; //12-bit integers
  if (LMS_SetupStream(device, &streamId) != 0)
    error();
}

void limeSDR::startStream() {
  LMS_StartStream(&streamId);
}

int limeSDR::getStream(int16_t &buffer, int sampleCnt) {
  int samplesRead = LMS_RecvStream(&streamId, &buffer, sampleCnt, NULL, 1000);
  return samplesRead;
}

void limeSDR::stopStream() {
  LMS_StopStream(&streamId); //stream is stopped but can be started again with LMS_StartStream()
  LMS_DestroyStream(device, &streamId); //stream is deallocated and can no longer be used
}

void limeSDR::close() {
  LMS_Close(device);
}
