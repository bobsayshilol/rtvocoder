# Make the lib
add_library(vocoder
  BandPass.cc
  LowPass.cc
  Utils.cc
  Vocoder.cc
)
target_include_directories(vocoder
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
