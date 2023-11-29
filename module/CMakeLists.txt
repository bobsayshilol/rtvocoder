# The LADSPA plugin that provides IPC communication and loads the internal lib.
add_library(ladspa_vocoder_module SHARED
    IPC.cc
    ladspa_lib.cc
    #lv2_lib.cc
    ModuleWrapper.cc
)
target_link_libraries(ladspa_vocoder_module
    PUBLIC
        ladspa
        lv2
)

add_subdirectory(internal)
