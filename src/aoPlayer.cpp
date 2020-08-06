#include "../include/aoPlayer.h"
#include <cstring>


aoPlayer::aoPlayer() {
  ao_initialize();
}

ao_device *audio_device;
ao_sample_format format;


int aoPlayer::initDefaultPlayer() {
  int default_driver = ao_default_driver_id();
  return default_driver;
}

ao_device* aoPlayer::openPlayer(int driver) {
  audio_device = ao_open_live(driver, &format, NULL);
  return audio_device;
}

int aoPlayer::setFormat() {
  memset(&format, 0, sizeof(format));
  format.bits = 16;
  format.channels = 2;
  format.rate = 44100;
  format.byte_format = AO_FMT_LITTLE;
  int buffer_size = format.bits/8 * format.channels * format.rate;
  return buffer_size;
}

void aoPlayer::buildBuffer(char* audio_buffer, int pos, int audio) {
  audio_buffer[4*pos] = audio_buffer[4*pos+2] = audio & 0xff;
  audio_buffer[4*pos+1] = audio_buffer[4*pos+3] = ((audio) >> 8) & 0xff;
}

void aoPlayer::play(ao_device *device, char *audio_buffer, int buffer_size) {
  ao_play(device, audio_buffer, buffer_size);
}

void aoPlayer::close(ao_device *device) {
  ao_close(device);
}

void aoPlayer::shutDown() {
  ao_shutdown();
}
