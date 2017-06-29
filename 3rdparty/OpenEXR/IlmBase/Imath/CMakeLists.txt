# Description of the Imath library

set(IMATH_HEADERS
  ImathBoxAlgo.h
  ImathBox.h
  ImathColorAlgo.h
  ImathColor.h
  ImathEuler.h
  ImathExc.h
  ImathExport.h
  ImathForward.h
  ImathFrame.h
  ImathFrustum.h
  ImathFrustumTest.h
  ImathFun.h
  ImathGL.h
  ImathGLU.h
  ImathHalfLimits.h
  ImathInt64.h
  ImathInterval.h
  ImathLimits.h
  ImathLineAlgo.h
  ImathLine.h
  ImathMath.h
  ImathMatrixAlgo.h
  ImathMatrix.h
  ImathNamespace.h
  ImathPlane.h
  ImathPlatform.h
  ImathQuat.h
  ImathRandom.h
  ImathRoots.h
  ImathShear.h
  ImathSphere.h
  ImathVecAlgo.h
  ImathVec.h
)
  
set(IMATH_SOURCES
  ImathShear.cpp
  ImathMatrixAlgo.cpp
  ImathVec.cpp
  ImathColorAlgo.cpp
  ImathFun.cpp
  ImathBox.cpp
  ImathRandom.cpp
)
 

# Creates the special target  
ADD_OPENEXR_LIBRARY(Imath IMATH_HEADERS IMATH_SOURCES
  "Imath implements 2D and 3D vectors, 3x3 and 4x4 matrices, quaternions and other 2D and 3D math functions."
  ${ILMBASE_VERSION} ${ILMBASE_SOVERSION} "2004")

target_link_libraries(Imath Iex)

# Disable recurrent warnings in MSVC
MSVC_COMPILE_FLAGS(Imath "/wd4244 /wd4267")
