cmake_minimum_required (VERSION 2.6) 
project (CoreLib) 

add_library(CoreLib_Basic STATIC
 Basic.h
 Common.h
 Dictionary.h
 Exception.h
 IntSet.h
 LibIO.cpp
 LibIO.h
 LibMath.cpp
 LibMath.h
 LibString.cpp
 LibString.h
 Link.h
 List.h
 Parser.cpp
 Parser.h
 PerformanceCounter.cpp
 PerformanceCounter.h
 SmartPointer.h
 Stream.cpp
 Stream.h
 TextIO.cpp
 TextIO.h
 Threading.h
 VectorMath.cpp
 VectorMath.h
 WideChar.cpp
 WideChar.h
 SecureCRT.h
)
add_subdirectory (Graphics) 
add_subdirectory (Imaging)
add_subdirectory (Regex)
