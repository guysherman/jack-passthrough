#include <fmt/format.h>
#include <jack/jack.h>

#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

#include "cxxopts.hpp"

int process(jack_nframes_t nframes, void *arg);
void shutdown(void *arg);

jack_client_t *client = nullptr;
std::vector<std::pair<jack_port_t *, jack_port_t *>> ports;
uint32_t sampleRate = 0;

#define NUM_PORTS_DEFAULT_VALUE "16"
#define CLIENT_NAME_DEFAULT_VALUE "jack-passthru"

int main(int argc, char **argv) {
  cxxopts::Options options(argv[0], " - example command line options");
  options.positional_help("[optional args]").show_positional_help();

  options.set_width(70).add_options()(
      "n,name", "Client name in JACK",
      cxxopts::value<std::string>()->default_value(CLIENT_NAME_DEFAULT_VALUE))(
      "p,ports", "Number of in/out pairs to open",
      cxxopts::value<uint32_t>()->default_value(NUM_PORTS_DEFAULT_VALUE));

  auto result = options.parse(argc, argv);

  std::string clientName = result["n"].as<std::string>();
  uint32_t numPorts = result["p"].as<uint32_t>();

  jack_status_t status;
  client = jack_client_open(clientName.c_str(), JackNoStartServer, &status);

  if (client == nullptr) {
    std::cout << "jack_client_open failed";
    return -1;
  }

  sampleRate = jack_get_sample_rate(client);
  std::cout << "Sample Rate: " << sampleRate << std::endl;

  jack_set_process_callback(client, process, 0);
  jack_on_shutdown(client, shutdown, 0);

  for (uint32_t i = 0; i < numPorts; ++i) {
    std::string inputPortname = fmt::format("input_{}", i);
    jack_port_t *input =
        jack_port_register(client, inputPortname.c_str(),
                           JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

    std::string outputPortname = fmt::format("output_{}", i);
    jack_port_t *output =
        jack_port_register(client, outputPortname.c_str(),
                           JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    ports.push_back(std::make_pair(input, output));
  }

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

  std::vector<std::pair<jack_port_t *, jack_port_t *>>::iterator it;
  for (it = ports.begin(); it != ports.end(); it++) {
    in =
        (jack_default_audio_sample_t *)jack_port_get_buffer(it->first, nframes);
    out = (jack_default_audio_sample_t *)jack_port_get_buffer(it->second,
                                                              nframes);
    memcpy(out, in, sizeof(jack_default_audio_sample_t) * nframes);
  }

  return 0;
}

void shutdown(void *arg) {
  std::cout << "Killed by JACK server." << std::endl;
  exit(0);
}
