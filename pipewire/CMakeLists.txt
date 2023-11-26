# Executable to load and configure the plugin.
add_executable(pipewire_vocoder_loader pipewire.cc)
target_link_libraries(pipewire_vocoder_loader PUBLIC PkgConfig::PIPEWIRE)
