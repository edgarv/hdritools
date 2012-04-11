# Description of the Imath library

set(IMATH_HEADERS
  ImathColorAlgo.h
  ImathMatrixAlgo.h
  ImathVec.h
  ImathShear.h ImathFun.h ImathBox.h ImathBoxAlgo.h
  ImathEuler.h ImathExc.h ImathLimits.h ImathLine.h
  ImathLineAlgo.h ImathMatrix.h ImathPlane.h
  ImathSphere.h ImathVecAlgo.h ImathQuat.h
  ImathFrustum.h ImathMath.h ImathGL.h
  ImathColor.h ImathRandom.h ImathRoots.h
  ImathHalfLimits.h ImathInterval.h ImathGLU.h
  ImathFrame.h ImathPlatform.h ImathInt64.h
  )
  
set(IMATH_SOURCES
  ImathShear.cpp ImathMatrixAlgo.cpp ImathVec.cpp
  ImathColorAlgo.cpp ImathFun.cpp
  ImathBox.cpp ImathRandom.cpp
  )
 

# Creates the special target  
ADD_OPENEXR_LIBRARY(Imath IMATH_HEADERS IMATH_SOURCES
  "Imath implements 2D and 3D vectors, 3x3 and 4x4 matrices, quaternions and other 2D and 3D math functions."
  ${ILMBASE_VERSION} ${ILMBASE_SOVERSION} "2004")

target_link_libraries(Imath Iex)

# Disable recurrent warnings in MSVC
MSVC_COMPILE_FLAGS(Imath "/wd4244 /wd4267")
