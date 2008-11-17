# Cmake file for running the ImageIO tests. It requires

set(SRCS
  main.cpp
  Timer.h Timer.cpp)

# Easily find the ImageIO headers at the parent directory
include_directories(..)

# We link againt ImageIO, so we need to remove that definition
remove_definitions(-DIMAGEIO_EXPORTS)

add_executable(ImageIO_Test ${SRCS})
target_link_libraries(ImageIO_Test ImageIO)

# Enable explicitly SSE2 in linux
if(NOT MSVC)
set_target_properties(ImageIO_Test PROPERTIES 
  COMPILE_FLAGS "-msse -msse2 -ftree-vectorizer-verbose=5"
  )
endif(NOT MSVC)
