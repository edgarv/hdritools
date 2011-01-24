HDR Image Tools – 0.1
April 17 2009
(c) Program of Computer Graphics, Cornell University 2008-2009
Edgar Velázquez-Armendariz  -  eva5 _at_ cs.cornell.edu

This module provides a set of simple tools to manipulate HDR images in 
the Radiance (.rgbe;.hdr) and OpenEXR (.exr) formats. They include a 
viewer, a batch tonemapper, the OpenEXR JNI file for RTGI and Matlab 
extensions to read and save OpenEXR files. 

The projects use the meta-build system CMake to create the actual build 
files. To generate the build files using cmake one can just execute, 
from the directory where the build files will be created: 

$ cmake –G “Generator Name” PATH_TO_MODULE

Where “Generator Name” is the name of the desired generator as shown by 
executing cmake without arguments. PATH_TO_MODULE is the path (relative 
or absolute) to the directory where the CMakeLists.txt file at the top 
of the module is located. More details about how to invoke cmake may be 
consulted online. 

The module provides the following:
- OpenEXR_JNI – The JNI implementation for RTGI. 

- OpenEXR_Matlab – Matlab extensions to read and save OpenEXR files. 

- ImageIO – Parallel C++ library to manipulate HDR files in the Radiance 
and OpenEXR files. It is tuned for speed, using templates, SSE/SSE2 
intrinsics. It can also write PNGs of integral type pixels at 8 and 16
bits per pixel.

- batchToneMapper – a parallel command line utility to tone map HDR 
files using exposure and gamma correction. It can also read the HDR 
files contained in a zip file directly. It is based on ImageIO. 

- qt4Image – HDR file viewer to tone map, zoom and compare files, using 
amenities such as drag-and-drop. This is meant to be a modern 
replacement of the venerable glimage. 


Required extra stuff:
* CMake >=2.6 (www.cmake.org) – to create the build files. 
* Thread Building Blocks >=2.0 (www.threadingbuildingblocks.org) – used 
  for the parallelization. 
* Qt >=4.4 (trolltech.com/products/qt/) – for batchToneMapper and 
  qt4Image. 

Build options:
The options are documented in cmake, use a GUI or ncurses version so see 
their description. However the defaults should be decent enough. 

Issues:
When compiling against a static Qt4 in windows, you might need to link 
manually batchToneMapper and qt4Image to a DirectX library contained in 
the SKD if you receive the error “error LNK2001: unresolved external 
symbol IID_ID3DXEffectStateManager”: 
 
“$(DXSDK_DIR)\Lib\x64\dxguid.lib” (for Win64 or)
“$(DXSDK_DIR)\Lib\x86\dxguid.lib” (for Win32)
Where DXSDK_DIR is the environment variable with the path to the DirectX SDK.


Acknowledgements

Fabio Pellacini did the original version of ImageIO/Image.h. The zip file reading
is based on code by Gilles Vollant. The OpenEXR_Matlab module is based on
work by Jinwei Gu.
