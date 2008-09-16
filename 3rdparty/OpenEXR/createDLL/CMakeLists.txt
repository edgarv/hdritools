# Cmake file for create DLL

set(SRCS
  createDLL.cpp 
  OptionParser.h OptionParser.cpp
  )
  
add_executable(createDLL ${SRCS})

# Disable the secure crt warnings in MSVC
MSVC_COMPILE_FLAGS(createDLL "/wd4996")
