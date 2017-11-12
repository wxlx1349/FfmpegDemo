//
// Created by wangxi on 2017/3/15.
//
extern "C" {
#include "com_example_wangxi_ffmpegdemo_VideoUtils.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "custom_log.h"
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <bits/c++locale.h>

#include "libyuv.h"
#include "queue.h"

//封装格式
#include "libavformat/avformat.h"
//解码
#include "libavcodec/avcodec.h"
//缩放
#include "libswscale/swscale.h"

#include "libswresample/swresample.h"

#include "libavutil/time.h"

#define MAX_AUDIO_FRME_SIZE 48000 * 4
using namespace libyuv;
using namespace std;

#define MAX_STREAM 2
#define PACKET_QUEUE_VIDEO_SIZE 140
#define PACKET_QUEUE_SIZE 100
#define PACKET_QUEUE_AUDIO_SIZE 110

#define MIN_SLEEP_TIME_US 1000ll
#define AUDIO_TIME_ADJUST_US -200000ll

struct Player {
    JavaVM *javaVM;

    AVFormatContext *input_format_ctx;
    int video_stream_index;
    int audio_stream_index;
    //流的总个数
    int captrue_streams_no;
    AVCodecContext *input_codec_ctx[MAX_STREAM];

    pthread_t decode_threads[MAX_STREAM];
    ANativeWindow *nativeWindow;

    SwrContext *swrContext;
    //输入采样率
    int in_sample_rate;
    int out_sample_rate;
    int out_channel_nb;
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt;

    //JNI
    jobject audio_track;
    jmethodID audio_track_write_mid;

    pthread_t thread_read_from_stream;
    //音频，视频队列数组
    Queue *packets[MAX_STREAM];

    //互斥锁
    pthread_mutex_t mutex;
    //条件变量
    pthread_cond_t cond;

    //视频开始播放的时间
    int64_t start_time;

    int64_t audio_clock;
};

//解码数据
struct DecoderData {
    Player *player;
    int stream_index;
};

void init_input_format_ctx(struct Player *player, const char *input_cstr) {
    //1.注册组件
    av_register_all();

    //封装格式上下文
    player->input_format_ctx = avformat_alloc_context();

    //2.打开输入视频文件
    if (avformat_open_input(&player->input_format_ctx, input_cstr, NULL, NULL) != 0) {
        LOGE("%s", "打开输入视频文件失败");
        return;
    }
    //3.获取视频信息
    if (avformat_find_stream_info(player->input_format_ctx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
        return;
    }

    int i = 0;
    player->captrue_streams_no = player->input_format_ctx->nb_streams;
    for (; i < player->input_format_ctx->nb_streams; i++) {
        //根据类型判断，是否是视频流
        if (player->input_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            player->video_stream_index = i;
            continue;
        } else if (player->input_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            player->audio_stream_index = i;
            continue;
        }
    }
}

/**
 * 初始化解码器上下文
 */
void init_codec_context(struct Player *player, int stream_idx) {
    AVFormatContext *format_ctx = player->input_format_ctx;
    //获取解码器
    LOGI("init_codec_context begin");
    AVCodecContext *codec_ctx = format_ctx->streams[stream_idx]->codec;
    LOGI("init_codec_context end");
    AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (codec == NULL) {
        LOGE("%s", "无法解码");
        return;
    }
    //打开解码器
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        LOGE("%s", "解码器无法打开");
        return;
    }
    player->input_codec_ctx[stream_idx] = codec_ctx;
}

void decode_video(struct Player *player, AVPacket *packet) {
    //像素数据（解码数据）
    AVFrame *yuv_frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();
    int video_stream_idx = player->video_stream_index;
    AVCodecContext *pCodeCtx = player->input_codec_ctx[video_stream_idx];
    //native绘制
    //窗体
    ANativeWindow *nativeWindow = player->nativeWindow;
    //绘制时的缓冲区
    ANativeWindow_Buffer outBuffer;
    int got_frame;

    //解码AVPacket->AVFrame
    avcodec_decode_video2(pCodeCtx, yuv_frame, &got_frame, packet);

    //Zero if no frame could be decompressed
    //非零，正在解码
    if (got_frame) {

        //lock
        //设置缓冲区的属性（宽、高、像素格式）
        ANativeWindow_setBuffersGeometry(nativeWindow, pCodeCtx->width, pCodeCtx->height,
                                         WINDOW_FORMAT_RGBA_8888);
        ANativeWindow_lock(nativeWindow, &outBuffer, NULL);

        //设置rgb_frame的属性（像素格式、宽高）和缓冲区
        //rgb_frame缓冲区与outBuffer.bits是同一块内存
        avpicture_fill((AVPicture *) rgb_frame, (const uint8_t *) outBuffer.bits,
                       PIX_FMT_RGBA,
                       pCodeCtx->width, pCodeCtx->height);

        //YUV->RGBA_8888
        I420ToARGB(yuv_frame->data[0], yuv_frame->linesize[0],
                   yuv_frame->data[2], yuv_frame->linesize[2],
                   yuv_frame->data[1], yuv_frame->linesize[1],
                   rgb_frame->data[0], rgb_frame->linesize[0],
                   pCodeCtx->width, pCodeCtx->height);

        //unlock
        ANativeWindow_unlockAndPost(nativeWindow);

//        usleep(11565);
    }
    av_frame_free(&yuv_frame);
    av_frame_free(&rgb_frame);
}

void decode_video_prepare(JNIEnv *env, struct Player *player, jobject surface) {
    player->nativeWindow = ANativeWindow_fromSurface(env, surface);
}

void decode_audio_prepare(struct Player *player) {
    AVCodecContext *pCodeCtx = player->input_codec_ctx[player->audio_stream_index];

    //解压缩数据
    SwrContext *swrCtx = swr_alloc();
    //重采样设置参数-------------start
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = pCodeCtx->sample_fmt;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = pCodeCtx->sample_rate;
    //输出采样率
    int out_sample_rate = in_sample_rate;
    //获取输入的声道布局
    //根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
    //av_get_default_channel_layout(codecCtx->channels);
    uint64_t in_ch_layout = pCodeCtx->channel_layout;
    //输出的声道布局（立体声）
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(swrCtx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                       in_sample_fmt, in_sample_rate, 0, NULL);
    swr_init(swrCtx);
    //输出的声道个数
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    //重采样设置参数-------------end
    player->swrContext = swrCtx;
    player->in_sample_fmt = in_sample_fmt;
    player->out_sample_fmt = out_sample_fmt;
    player->in_sample_rate = in_sample_rate;
    player->out_sample_rate = out_sample_rate;
    player->out_channel_nb = out_channel_nb;


}

void audio_jni_prepare(JNIEnv *env, jobject jobj, struct Player *player) {
    int out_sample_rate = player->out_sample_rate;
    int out_channel_nb = player->out_channel_nb;
    //JNI begin------------------
    //JasonPlayer
    jclass video_util_class = env->GetObjectClass(jobj);

    //AudioTrack对象
    jmethodID create_audio_track_mid = env->GetMethodID(video_util_class, "createAudioTrack",
                                                        "(II)Landroid/media/AudioTrack;");
    jobject audio_track = env->CallObjectMethod(jobj, create_audio_track_mid, out_sample_rate,
                                                out_channel_nb);

    //调用AudioTrack.play方法
    jclass audio_track_class = env->GetObjectClass(audio_track);
    jmethodID audio_track_play_mid = env->GetMethodID(audio_track_class, "play", "()V");
    env->CallVoidMethod(audio_track, audio_track_play_mid);

    //AudioTrack.write
    jmethodID audio_track_write_mid = env->GetMethodID(audio_track_class, "write", "([BII)I");

    //JNI end------------------
    //跨进程调用局部引用会报错，要用全局对象----------------------
    player->audio_track = env->NewGlobalRef(audio_track);
    player->audio_track_write_mid = audio_track_write_mid;
    //JNI end------------------
}

/**
 * 获取视频当前播放时间
 */
int64_t player_get_current_video_time(Player *player) {
    int64_t current_time = av_gettime();
    return current_time - player->start_time;
}

/**
 * 延迟
 */
void player_wait_for_frame(Player *player, int64_t stream_time,
                           int stream_no) {
    pthread_mutex_lock(&player->mutex);
    for (;;) {
        int64_t current_video_time = player_get_current_video_time(player);
        int64_t sleep_time = stream_time - current_video_time;
        if (sleep_time < -300000ll) {
            // 300 ms late
            int64_t new_value = player->start_time - sleep_time;
            LOGI("player_wait_for_frame[%d] correcting %f to %f because late", stream_no,
                 (av_gettime() - player->start_time) / 1000000.0,
                 (av_gettime() - new_value) / 1000000.0);

            player->start_time = new_value;
            pthread_cond_broadcast(&player->cond);
        }

        if (sleep_time <= MIN_SLEEP_TIME_US) {
            // We do not need to wait if time is slower then minimal sleep time
            break;
        }

        if (sleep_time > 500000ll) {
            // if sleep time is bigger then 500ms just sleep this 500ms
            // and check everything again
            sleep_time = 500000ll;
        }
        //等待指定时长
        int timeout_ret = pthread_cond_timeout_np(&player->cond,
                                                  &player->mutex, sleep_time / 1000ll);

        // just go further
        LOGI("player_wait_for_frame[%d] finish", stream_no);
    }
    pthread_mutex_unlock(&player->mutex);
}


/**
 * 给AVPacket开辟空间，后面会将AVPacket栈内存数据拷贝至这里开辟的空间
 */
void *player_fill_packet() {
    //请参照我在vs中写的代码
    AVPacket *packet = new AVPacket();
    return packet;
}

int getQueueSize(int i, Player *player) {
    if (i == player->audio_stream_index) {
        return PACKET_QUEUE_AUDIO_SIZE;
    } else if (i == player->video_stream_index) {
        return PACKET_QUEUE_VIDEO_SIZE;
    } else {
        return PACKET_QUEUE_SIZE;
    }
}

/**
 * 初始化音频，视频AVPacket队列，长度100
 */
void player_alloc_queues(Player *player) {
    int i;
    //这里，正常是初始化两个队列
    for (i = 0; i < player->captrue_streams_no; ++i) {
//        int queueSize=getQueueSize(i,player);
        Queue *queue = create_queue();
        player->packets[i] = queue;
        //打印视频音频队列地址
        LOGI("stream index:%d,queue:%#x", i, queue);
    }
}


void *packet_free_func(AVPacket *packet) {
    av_free_packet(packet);
    return 0;
}

void decode_audio(struct Player *player, AVPacket *packet) {
    AVCodecContext *pCodeCtx = player->input_codec_ctx[player->audio_stream_index];
    AVFormatContext *input_format_ctx = player->input_format_ctx;
    AVStream *stream = input_format_ctx->streams[player->video_stream_index];

    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    int got_frame = -1;
    avcodec_decode_audio4(pCodeCtx, frame, &got_frame, packet);
    LOGI("%s", "decode_audio");
//16bit 44100 PCM 数据（重采样缓冲区）
    uint8_t *out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_FRME_SIZE);
    //解码一帧成功
    if (got_frame > 0) {
        swr_convert(player->swrContext, &out_buffer, MAX_AUDIO_FRME_SIZE,
                    (const uint8_t **) frame->data,
                    frame->nb_samples);
        //获取sample的size
        int out_buffer_size = av_samples_get_buffer_size(NULL, player->out_channel_nb,
                                                         frame->nb_samples, player->out_sample_fmt,
                                                         1);

        int64_t pts = packet->pts;
        if (pts != AV_NOPTS_VALUE) {
            player->audio_clock = av_rescale_q(pts, stream->time_base, AV_TIME_BASE_Q);
            //				av_q2d(stream->time_base) * pts;
            LOGI("player_write_audio - read from pts");
            player_wait_for_frame(player,
                                  player->audio_clock + AUDIO_TIME_ADJUST_US,
                                  player->audio_stream_index);
        }

        //关联当前线程的JNIEnv
        JavaVM *javaVM = player->javaVM;
        JNIEnv *env = NULL;
        javaVM->AttachCurrentThread(&env, NULL);

        jbyteArray audio_sample_array = env->NewByteArray(out_buffer_size);
        jbyte *sample_bytep = env->GetByteArrayElements(audio_sample_array, NULL);

        memcpy(sample_bytep, out_buffer, out_buffer_size);
        env->ReleaseByteArrayElements(audio_sample_array, sample_bytep, 0);

        env->CallIntMethod(player->audio_track, player->audio_track_write_mid, audio_sample_array,
                           0, out_buffer_size);
        env->DeleteLocalRef(audio_sample_array);
        javaVM->DetachCurrentThread();
//        usleep(1142*11);
    }
//    av_frame_free(&frame);
}

/**
 * 生产者线程：负责不断的读取视频文件中AVPacket，分别放入两个队列中
 */
void *player_read_from_stream(void *arg) {
    Player *player = (Player *) arg;
    int index = 0;
    int ret;
//    AVPacket avPacket, *pkt = &avPacket;
    for (;;) {
        AVPacket *pkt = new AVPacket;
        ret = av_read_frame(player->input_format_ctx, pkt);
        if (ret < 0) {
            break;
        }
        Queue *queue = player->packets[pkt->stream_index];
        LOGE("stream Index=%d audio index=%d count=%d", pkt->stream_index,
             player->audio_stream_index, queue->count);
//       while (need_write_wait(queue)){
//           usleep(1000*10);
//       }
        pthread_mutex_lock(&player->mutex);
        queue_append_last(pkt, queue);
        pthread_cond_signal(&player->cond);
        if (queue_size(queue) > 900 && pkt->stream_index == player->video_stream_index) {
            pthread_cond_wait(&player->cond, &player->mutex);
        }
//        *packet=avPacket;
        pthread_mutex_unlock(&player->mutex);
//        LOGI("queue:%#x, packet:%#x",queue,packet);
    }
}

/**
 * 解码子线程函数（消费）
 */
void *decode_data(void *arg) {
    DecoderData *decoderData = (DecoderData *) arg;
    Player *player = decoderData->player;
    Queue *queue = player->packets[decoderData->stream_index];
    //编码数据
    int video_frame_count, audio_frame_count = 0;
    while (true) {
        pthread_mutex_lock(&player->mutex);
        while (queue_size(queue) < 2) {
            pthread_cond_signal(&player->cond);
            pthread_cond_wait(&player->cond, &player->mutex);
            LOGE("waite count:%d", queue->count);
        }
        AVPacket *packet = (AVPacket *) queue_get_first(queue);
        if (packet) {
            queue_delete_first(queue);
        }
        pthread_mutex_unlock(&player->mutex);
        if (packet->stream_index == player->video_stream_index) {
            decode_video(player, packet);
            LOGE("video_frame_count:%d index:%d", queue->count, packet->stream_index);
        } else if (packet->stream_index == player->audio_stream_index) {
            decode_audio(player, packet);
            LOGE("audio_frame_count:%d index:%d", queue->count, packet->stream_index);
        }
//        av_free_packet(packet);
    }
}

//void* decode_data(void* arg){
//    Player* player=(Player*) arg;
//    //编码数据
//    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
//    AVFormatContext *pFormatCtx = player->input_format_ctx;
//    int video_frame_count = 0;
//    while (av_read_frame(pFormatCtx, packet) >= 0) {
//        if(packet->stream_index==player->video_stream_index){
//          decode_video(player,packet);
//            LOGI("video_frame_count:%d",video_frame_count++);
//        } else if(packet->stream_index==player->audio_stream_index){
//            decode_audio(player,packet);
//        }
//        av_free_packet(packet);
//    }
//}



JNIEXPORT void JNICALL Java_com_example_wangxi_ffmpegdemo_VideoUtils_play
        (JNIEnv *env, jobject jobj, jstring input_jstr, jobject surface) {
    const char *input_cstr = env->GetStringUTFChars(input_jstr, NULL);
    //初始化player
    struct Player *player = new Player;
    env->GetJavaVM(&(player->javaVM));

    init_input_format_ctx(player, input_cstr);
    int video_stream_index = player->video_stream_index;
    int audio_stream_index = player->audio_stream_index;
    init_codec_context(player, video_stream_index);
    init_codec_context(player, audio_stream_index);

    decode_video_prepare(env, player, surface);
    decode_audio_prepare(player);

    audio_jni_prepare(env, jobj, player);

    player_alloc_queues(player);

    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);

    //生产者线程
    pthread_create(&(player->thread_read_from_stream), NULL, player_read_from_stream,
                   (void *) player);
    sleep(1);
    player->start_time = 0;
    //消费者线程
    DecoderData data1 = {player, video_stream_index}, *decoder_data1 = &data1;
    pthread_create(&(player->decode_threads[video_stream_index]), NULL, decode_data,
                   (void *) decoder_data1);

    DecoderData data2 = {player, audio_stream_index}, *decoder_data2 = &data2;
    pthread_create(&(player->decode_threads[audio_stream_index]), NULL, decode_data,
                   (void *) decoder_data2);
//    LOGI("video_queue_count:%d", 13);
    pthread_join(player->thread_read_from_stream, NULL);
    pthread_join(player->decode_threads[video_stream_index], NULL);
    pthread_join(player->decode_threads[audio_stream_index], NULL);


    /*ANativeWindow_release(nativeWindow);
	av_frame_free(&yuv_frame);
	avcodec_close(pCodeCtx);
	avformat_free_context(pFormatCtx);

	(*env)->ReleaseStringUTFChars(env,input_jstr,input_cstr);

	free(player);*/
} ;


};