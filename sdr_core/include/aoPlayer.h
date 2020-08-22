#include <ao/ao.h>

class aoPlayer {
  public:
    aoPlayer();
    int initDefaultPlayer();
    ao_device* openPlayer(int driver);
    int setFormat();
    void buildBuffer(char* audio_buffer, int pos, int audio);
    void play(ao_device *device, char *audio_buffer, int buffer_size);
    void close(ao_device *device);
    void shutDown();
};
