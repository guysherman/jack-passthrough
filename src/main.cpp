#include <fmt/format.h>
#include <jack/jack.h>

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <utility>
#include <vector>

#include "cxxopts.hpp"

#define NUM_PORTS_DEFAULT_VALUE "16"
#define CLIENT_NAME_DEFAULT_VALUE "jack-passthru"

typedef std::vector<std::pair<jack_port_t *, jack_port_t *>> PortMap;

int process(jack_nframes_t nframes, void *arg);
void shutdown(void *arg);
jack_client_t *initJack(std::string clientName, PortMap &ports);
void portSetup(jack_client_t *client, uint32_t numPorts, PortMap &ports);
cxxopts::ParseResult parseArgs(int argc, char **argv);

int main(int argc, char **argv) {
  PortMap ports;

  auto options = parseArgs(argc, argv);

  std::string clientName = options["n"].as<std::string>();
  uint32_t numPorts = options["p"].as<uint32_t>();

  jack_client_t *client = initJack(clientName, ports);

  portSetup(client, numPorts, ports);

  if (jack_activate(client)) {
    std::cout << "Couldn't activate client." << std::endl;
    return -1;
  }

  std::cout << "Press ctrl+c to terminate." << std::endl;
  sleep(-1);
  return 0;
}

cxxopts::ParseResult parseArgs(int argc, char **argv) {
  cxxopts::Options options(argv[0], "jack-passthru");
  options.positional_help("[optional args]").show_positional_help();

  // clang-format off
  options.set_width(70).add_options()
    ("h,help", "Print the help message")
    ("n,name", "Client name in JACK", cxxopts::value<std::string>()->default_value(CLIENT_NAME_DEFAULT_VALUE))
    ("p,ports", "Number of in/out pairs to open", cxxopts::value<uint32_t>()->default_value(NUM_PORTS_DEFAULT_VALUE))
  ;
  // clang-format on
  auto parsedOptions = options.parse(argc, argv);

  if (parsedOptions.count("help")) {
    std::cout << options.help({""}) << std::endl;
    exit(0);
  }
  return parsedOptions;
}

jack_client_t *initJack(std::string clientName, PortMap &ports) {
  jack_status_t status;
  std::string shortenedClientName = clientName.substr(0, jack_client_name_size());
  jack_client_t *client = jack_client_open(shortenedClientName.c_str(), JackNoStartServer, &status);

  if (client == nullptr) {
    std::cout << fmt::format("jack_client_open failed: status {0:#x}", status) << std::endl;
    if (status & JackServerFailed) {
      std::cout << fmt::format("Unable to connect to JACK server") << std::endl;
    }
    exit(-1);
  }

  if (status & JackServerStarted) {
    std::cout << "Jack Server Started";
  }

  if (status & JackNameNotUnique) {
    shortenedClientName = std::string(jack_get_client_name(client));
    std::cout << fmt::format("Unique name: {} assigned.", shortenedClientName);
  }

  uint32_t sampleRate = 0;
  sampleRate = jack_get_sample_rate(client);
  std::cout << "Sample Rate: " << sampleRate << std::endl;

  jack_set_process_callback(client, process, std::addressof(ports));
  jack_on_shutdown(client, shutdown, client);

  return client;
}

void portSetup(jack_client_t *client, uint32_t numPorts, PortMap &ports) {
  for (uint32_t i = 0; i < numPorts; ++i) {
    std::string inputPortname = fmt::format("input_{}", i);
    jack_port_t *input = jack_port_register(client, inputPortname.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

    std::string outputPortname = fmt::format("output_{}", i);
    jack_port_t *output = jack_port_register(client, outputPortname.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (input == nullptr || output == nullptr) {
      std::cout << "No more JACK ports available";
      exit(-1);
    }

    ports.push_back(std::make_pair(input, output));
  }
}

int process(jack_nframes_t nframes, void *arg) {
  PortMap &ports = *((PortMap *)arg);
  jack_default_audio_sample_t *in, *out;

  PortMap::iterator it;
  for (it = ports.begin(); it != ports.end(); it++) {
    in = (jack_default_audio_sample_t *)jack_port_get_buffer(it->first, nframes);
    out = (jack_default_audio_sample_t *)jack_port_get_buffer(it->second, nframes);
    memcpy(out, in, sizeof(jack_default_audio_sample_t) * nframes);
  }

  return 0;
}

void shutdown(void *arg) {
  jack_client_t *client = (jack_client_t *)arg;
  jack_client_close(client);
  std::cout << "Killed by JACK server." << std::endl;
  exit(0);
}
