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

