#include "Pipewire.h"
#include "Processor.h"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <pipewire/impl.h>
#include <readline/readline.h>
#include <string>
#include <string_view>
#include <utility>

namespace {

class Loader {
    pw_impl_module *m_module = nullptr;

    pw_main_loop *m_loop = nullptr;
    pw_context *m_context = nullptr;

    std::vector<pipewire::Device> m_devices;

  private:
    void shutdown() {
        pw_context_destroy(std::exchange(m_context, nullptr));
        pw_main_loop_destroy(std::exchange(m_loop, nullptr));
    }

  public:
    ~Loader() {
        unload();
        shutdown();
    }

    bool init(int argc, char **argv) {
        pw_log_set_level(spa_log_level::SPA_LOG_LEVEL_DEBUG);
        pw_init(&argc, &argv);
        m_loop = pw_main_loop_new(nullptr);
        m_context = pw_context_new(pw_main_loop_get_loop(m_loop), nullptr, 0);
        return true;
    }

    void load(std::size_t dev_id) {
        unload();

        if (dev_id >= m_devices.size()) {
            printf("Invalid device ID\n");
            return;
        }

#if 0  // TODO
        auto const &in_dev = m_devices[dev_id];
        char const source_name[] = "libpipewire-module-filter-chain";
        std::string source_args = R"delim(
filter.graph = {

    nodes = [
        {
            type = ladspa
            name = vocoder
            plugin = /home/bob/Documents/gits/pipewire-vocoder/build/module/libladspa_vocoder_module.so
            label = vocoder_test_filter
            control = {
                Port 25560
            }
        }
    ]

    inputs  = [ "vocoder:Input audio buffer" ]
    outputs = [ "vocoder:Output audio buffer" ]

    capture.props = {
        node.name    = "capture.vocoder_test"
        #node.passive = true
        #audio.rate   = 48000
        audio.channels = 1
        #media.class = Audio/Source
        node.target  = ")delim" + in_dev.name +
                                  R"delim("
    }
    playback.props = {
        media.class = Audio/Source
    }
}


node.description =  "Test vocoder"
media.name =  "Test vocoder"


)delim";
        m_module = pw_context_load_module(m_context, source_name,
                                          source_args.c_str(), nullptr);
        printf("%p: %i\n", (void *)m_module, errno);

        if (!m_module) {
            unload();
        }
#endif
    }

    void unload() {
        if (m_module) {
            pw_impl_module_destroy(m_module);
            m_module = nullptr;
        }
    }

    // TODO: separate thread or something so we can remain interactive
    void start() { pipewire::run_processor(m_loop); }
    void stop() {}

    void fetch() {
        auto display_devices = [](std::vector<pipewire::Device> const &devices,
                                  char const *title) {
            printf("%s:\n", title);
            for (std::size_t i = 0; i < devices.size(); i++) {
                auto const &device = devices[i];
                printf("  %zu: (%i) %s - %s\n", i, device.id,
                       device.description.c_str(), device.name.c_str());
            }
        };
        m_devices =
            pipewire::fetch_devices(m_context, m_loop, pipewire::Direction::In);
        display_devices(m_devices, "Input devices");
        // m_out_devices = pipewire::fetch_devices(m_context, m_loop,
        //                                         pipewire::Direction::Out);
        // display_devices(m_out_devices, "Output devices");
    }
} g_loader;

const struct {
    char const *prefix;
    char const *help;
    bool (*run)(std::string_view arg);
} s_commands[]{
    {"load", "Load the plugin",
     [](std::string_view args) {
         // Should be in the form "load X".
         args.remove_prefix(4);
         auto len = args.find_first_not_of(' ');
         if (len != args.npos) {
             args.remove_prefix(len);
             // Parse X.
             int idx = 0;
             if (std::from_chars(args.begin(), args.end(), idx).ec ==
                 std::errc{}) {
                 g_loader.load(idx);
             }
         }
         return true;
     }},
    {"unload", "Unload the plugin",
     [](std::string_view) {
         g_loader.unload();
         return true;
     }},
    {"start", "Start vocoding",
     [](std::string_view) {
         g_loader.start();
         return true;
     }},
    {"stop", "Stop vocoding",
     [](std::string_view) {
         g_loader.stop();
         return true;
     }},
    {"quit", "Quit the REPL", [](std::string_view) { return false; }},
    {"fetch", "Fetch devices",
     [](std::string_view) {
         g_loader.fetch();
         return true;
     }},
};

void print_help() {
    for (auto const &command : s_commands) {
        printf("%s: %s\n", command.prefix, command.help);
    }
}

}  // namespace

int main(int argc, char **argv) {
    // Init our connection.
    if (!g_loader.init(argc, argv)) {
        return EXIT_FAILURE;
    }

    // Fetch the devices so we don't have to type it once.
    g_loader.fetch();
    g_loader.start();
    // return 0;

    // Don't know why we have to call it again here, but if we don't then
    // there's no logs.
    pw_log_set_level(spa_log_level::SPA_LOG_LEVEL_DEBUG);

    // Enter the loop.
    bool quit = false;
    while (!quit) {
        // Read the next command.
        char *line = readline(">: ");
        if (line != nullptr) {
            // See if we have a matching command.
            std::string_view const line_view{line};
            auto command_it =
                std::find_if(std::begin(s_commands), std::end(s_commands),
                             [=](auto const &command) {
                                 return line_view.starts_with(command.prefix);
                             });
            if (command_it != std::end(s_commands)) {
                quit = !command_it->run(line);
            } else {
                print_help();
            }
        } else {
            quit = true;
        }
        free(line);
    }

    // Cleanup.
    g_loader.unload();
    return EXIT_SUCCESS;
}
