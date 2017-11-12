//
// Created by wangxi on 2017/3/17.
//

#ifndef FFMPEGDEMO_CUSTOM_LOG_H
#define FFMPEGDEMO_CUSTOM_LOG_H
#include <android/log.h>
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"jason",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);

#endif //FFMPEGDEMO_CUSTOM_LOG_H
