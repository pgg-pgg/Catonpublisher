
cmake_minimum_required(VERSION 3.4.1)

add_library( # Sets the name of the library.
             push

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/push.c
             src/main/cpp/pushstream.c
             src/main/cpp/sps_process.c
             src/main/cpp/hevc_sps_process.c
             src/main/cpp/mpegts/mpeg-ts-enc.c
             src/main/cpp/mpegts/mpeg-ts-h264.c
             src/main/cpp/mpegts/mpeg-ts-h265.c
             src/main/cpp/mpegts/mpeg-pat.c
             src/main/cpp/mpegts/mpeg-pmt.c
             src/main/cpp/mpegts/mpeg-util.c
             src/main/cpp/mpegts/crc32.c
             src/main/cpp/mpegts/mpeg-pes.c)


#引用的头文件
include_directories(
                src/main/cpp/r2tp/include
                src/main/cpp/ssl/include
                src/main/cpp/mpegts
)

#添加动态库为SHARED 静态库是STATIC
add_library(crypto SHARED IMPORTED)
set_target_properties(crypto
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libcrypto.so)

add_library(ssl SHARED IMPORTED)
set_target_properties(ssl
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libssl.so)

add_library(r2tp SHARED IMPORTED)
set_target_properties(r2tp
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libr2tpv3.so)


find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )



target_link_libraries(
                       push

                        crypto
                        ssl
                        r2tp

                       ${log-lib} )