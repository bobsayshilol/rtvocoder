# Make the lib
add_library(vocoder
  BandPass.cc
  LowPass.cc
  SecondOrderFilter.cc
  Utils.cc
  Vocoder.cc
  WAVFile.cc
)
target_include_directories(vocoder
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
