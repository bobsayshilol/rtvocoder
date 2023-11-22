# Fake mic with vocoder
add_executable(pipewire_vocoder pipewire.cc)
target_link_libraries(pipewire_vocoder PUBLIC PkgConfig::PIPEWIRE vocoder)
