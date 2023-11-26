# The LADSPA plugin that provides the vocoder functionality.
add_library(ladspa_vocoder_module SHARED
    IPC.cc
    ladspa_lib.cc
    #lv2_lib.cc
    Module.cc
)
target_link_libraries(ladspa_vocoder_module
    PUBLIC
        ladspa
        lv2
        vocoder
)
