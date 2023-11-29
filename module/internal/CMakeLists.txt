# The internal lib that actually provides the vocoder functionality.
add_library(internal_vocoder_module SHARED
    Module.cc
)
target_link_libraries(internal_vocoder_module
    PRIVATE
        vocoder
)
