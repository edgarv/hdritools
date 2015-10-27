# Description of the Iex library

set(IEX_HEADERS
  IexBaseExc.h
  IexMathExc.h
  IexThrowErrnoExc.h
  IexErrnoExc.h
  IexMacros.h
  Iex.h
  IexNamespace.h
  IexExport.h
  IexForward.h
)
  
set(IEX_SOURCES
  IexThrowErrnoExc.cpp 
  IexBaseExc.cpp
  )
  

# Creates the special target  
ADD_OPENEXR_LIBRARY(Iex IEX_HEADERS IEX_SOURCES
  "Iex is an exception-handling library."
  ${ILMBASE_VERSION} ${ILMBASE_SOVERSION} "2002")

# Disable recurrent warnings in MSVC
MSVC_COMPILE_FLAGS(Iex "/wd4996 /wd4267")
