#include <jack/jack.h>

#include <cstring>
#include <iostream>

int process(jack_nframes_t nframes, void *arg);
void shutdown(void *arg);

jack_client_t *client = nullptr;
jack_port_t *input = nullptr;
jack_port_t *output = nullptr;
uint32_t sampleRate = 0;

int main(int argc, char **argv) {
  jack_status_t status;
  client = jack_client_open("jack-passthru", JackNoStartServer, &status);

  if (client == nullptr) {
    std::cout << "jack_client_open failed";
    return -1;
  }

  sampleRate = jack_get_sample_rate(client);
  std::cout << "Sample Rate: " << sampleRate << std::endl;

  jack_set_process_callback(client, process, 0);
  jack_on_shutdown(client, shutdown, 0);

  input = jack_port_register(client, "input", JACK_DEFAULT_AUDIO_TYPE,
                             JackPortIsInput, 0);
  output = jack_port_register(client, "output", JACK_DEFAULT_AUDIO_TYPE,
                              JackPortIsOutput, 0);

  // probably do some error handling here

  if (jack_activate(client)) {
    std::cout << "Couldn't activate client." << std::endl;
    return -1;
  }

  std::cout << "Press any key to continue..." << std::endl;
  std::cin.ignore();
  return 0;
}

int process(jack_nframes_t nframes, void *arg) {
  jack_default_audio_sample_t *in, *out;

  in = (jack_default_audio_sample_t *)jack_port_get_buffer(input, nframes);
  out = (jack_default_audio_sample_t *)jack_port_get_buffer(output, nframes);

  memcpy(out, in, sizeof(jack_default_audio_sample_t) * nframes);
  return 0;
}

void shutdown(void *arg) {
  std::cout << "Killed by JACK server." << std::endl;
  exit(0);
}
