cmake_minimum_required(VERSION 3.4.1)

#变量
set(distribution_DIR ../../../../libs)

find_library( # Sets the name of the path variable.
              log-lib
              log )
find_library( android-lib
               android)
add_library( native-lib
             SHARED
             src/main/cpp/player.cpp
             src/main/cpp/queue.h
             src/main/cpp/native-lib.cpp )
add_library( avutil-54
             SHARED
             IMPORTED)
set_target_properties( avutil-54
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libavutil-54.so)
add_library( swresample-1
             SHARED
             IMPORTED)
set_target_properties( swresample-1
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libswresample-1.so)
add_library( avcodec-56
             SHARED
             IMPORTED)
set_target_properties( avcodec-56
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libavcodec-56.so)
add_library( avfilter-5
             SHARED
             IMPORTED)
set_target_properties( avfilter-5
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libavfilter-5.so)
add_library( swscale-3
               SHARED
               IMPORTED)
set_target_properties( swscale-3
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libswscale-3.so)
add_library( avformat-56
             SHARED
             IMPORTED)
set_target_properties( avformat-56
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libavformat-56.so)
add_library( avdevice-56
             SHARED
             IMPORTED)
set_target_properties( avdevice-56
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libavdevice-56.so)
add_library( postproc-53
             SHARED
             IMPORTED)
set_target_properties( postproc-53
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libpostproc-53.so)
add_library( libyuv
             SHARED
             IMPORTED)
set_target_properties( libyuv
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/${ANDROID_ABI}/libyuv.so)

#设置编译C的标准
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")


include_directories(src/main/cpp)
include_directories(libs/include/ffmpeg)
include_directories(libs/include/libyuv)

target_link_libraries( # Specifies the target library.
                       native-lib
                       avutil-54
                       swresample-1
                       avcodec-56
                       avfilter-5
                       swscale-3
                       avformat-56
                       avdevice-56
                       postproc-53
                       libyuv
                       ${android-lib}
                       ${log-lib} )
