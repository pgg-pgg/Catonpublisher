//
// Created by saisai on 2018/6/26 0026.
//

#include <jni.h>
#include <sys/socket.h>
#include <endian.h>
#include <arpa/inet.h>
#include <android/log.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "pushstream.h"
#include "mpegts/mpeg-ts.h"
//#include "yuv/include/libyuv.h"


#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)


jclass callbackCls;
JavaVM *javaVM;
int sendType;

//unsigned char *tsBuf;
//pthread_mutex_t mpegWriteLock;
//int ind = -1;
//int file;
////第一个数据帧不是视频帧则丢掉
//int first = -1;
//
//mpeg_ts_enc_context_t *ts_enc_context = NULL;
//
//void *mpegalloc(void *param, size_t bytes) {
//
//    param = (void *) malloc(bytes);
//    return param;
//}
//
//void mpegfree(void *param, void *packet) {
//
//    free(param);
//    free(packet);
//}
//
//void mpegwrite(void *param, const void *packet, size_t bytes) {
//
//    pthread_mutex_lock(&mpegWriteLock);
//    memcpy(tsBuf + ind, packet, bytes);
//    ind += bytes;
//    if (ind >= TS_PACKET_SIZE) {
//        ind = 0;
//        write(file,tsBuf,TS_PACKET_SIZE);
//        memset(tsBuf, 0, TS_PACKET_SIZE);
//    }
//    pthread_mutex_unlock(&mpegWriteLock);
////    LOGE("ind = %d",ind);
//}





int eventCallback(void *connect, int event, int error, void *userdata) {

    if (javaVM != NULL) {

        JNIEnv *env;
        (*javaVM)->AttachCurrentThread(javaVM, (void **) &env, 0);

        jmethodID callBack = (*env)->GetStaticMethodID(env, callbackCls, "eventCallback",
                                                       "(II)V");
        (*env)->CallStaticVoidMethod(env, callbackCls, callBack, event, error);
//        (*javaVM)->DetachCurrentThread(javaVM);
        return 0;
    }

    return 0;
}


JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_connect
        (JNIEnv *env, jobject obj, jstring host, jint port, jint auth, jint encrypt, jstring key) {

    const char *cHost = (*env)->GetStringUTFChars(env, host, 0);
    const char *cKey = (*env)->GetStringUTFChars(env, key, 0);

//    sendType=STATE_UDPSEND;
    sendType=STATE_R2TPSEND;

    if(sendType==STATE_UDPSEND){
        return Open(sendType, cHost,port);
    } else if(sendType==STATE_R2TPSEND){
        int ret = Open(sendType, NULL, 0);

        if (ret == 0) {
            return Connect(cHost, port, auth, encrypt, cKey);
        } else {
            return ret;
        }
    }

    return -1;
}

JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_setCallback
        (JNIEnv *env, jobject obj) {

    jclass cls = (*env)->GetObjectClass(env, obj);
    callbackCls = (*env)->NewGlobalRef(env, cls);

    (*env)->GetJavaVM(env, &javaVM);
    return SetCallback(eventCallback);
}

JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_push
        (JNIEnv *env, jobject obj, jint streamType, jint codecType, jint flag, jlong pts, jlong dts,
         jbyteArray data,
         jint len) {

    jbyte *Src_data = malloc(sizeof(jbyte) * len);
    (*env)->GetByteArrayRegion(env, data, 0, len, Src_data);
    return PutFrame(streamType, codecType, flag, pts, dts, Src_data, (size_t) len);
}

JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_parseStat
        (JNIEnv *env, jobject obj) {

    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID parseStatM = (*env)->GetMethodID(env, cls, "parseStat", "(FIIII)V");

    float dropRate = 0.0f;
    int bitrate = 0;
    int jitter = 0;
    int rtt = 0;
    int maxDelay = 0;

    ParseData(&dropRate, &bitrate, &jitter, &rtt, &maxDelay);

    (*env)->CallVoidMethod(env, obj, parseStatM, dropRate, bitrate, jitter, rtt, maxDelay);
    (*env)->DeleteLocalRef(env, cls);
    return 0;
}

JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_disconnect
        (JNIEnv *env, jobject obj) {

    return Disconnect();
}


JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_pushUDP
        (JNIEnv *env, jobject obj, jstring host, jint port) {

    const char *cHost = (*env)->GetStringUTFChars(env, host, 0);

    Open(STATE_UDPSEND, cHost, port);

    return 0;
}

JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_getR2tpVersion
        (JNIEnv *env, jobject obj) {
    int major;
    int minor;
    int revision;
    GetR2tpVersion(&major, &minor, &revision);
    LOGE("r2tp version: %d.%d.%d\n", major, minor, revision);
    return 0;
}

JNIEXPORT jbyteArray JNICALL Java_com_saisai_catonpublisher_Jni_spsSetTimingFlag
        (JNIEnv *env, jobject obj, jint codecId, jbyteArray sps, jint len, jint fps) {

    jbyte *sps_src = malloc(sizeof(jbyte) * len);
    (*env)->GetByteArrayRegion(env, sps, 0, len, sps_src);

    unsigned char sps_dst[100] = {0};
    unsigned int dst_len = 0;


    spsSetTimingFlag(codecId,fps,sps_src,len,sps_dst,&dst_len);

    jbyteArray dst = (*env)->NewByteArray(env, dst_len);
    (*env)->SetByteArrayRegion(env, dst, 0, dst_len, sps_dst);

    return dst;
}

//JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_initWrite
//        (JNIEnv *env, jobject obj,jstring path){
//
//    struct mpeg_ts_func_t h;
//    h.alloc = mpegalloc;
//    h.free = mpegfree;
//    h.write = mpegwrite;
//
//    void *param;
//
//    ts_enc_context = mpeg_ts_create(&h, param);
//    if (ts_enc_context == NULL) {
//        LOGE("Open:ts_enc_context == NULL");
//        return -1;
//    }
//
//    //设置 处理的音频和视频格式
//    ts_enc_context->pat.pmts[0].stream_count = 2; // H.264 + AAC
//    ts_enc_context->pat.pmts[0].streams[0].pid = 0x101;
//    ts_enc_context->pat.pmts[0].streams[0].sid = PES_SID_AUDIO;
//    ts_enc_context->pat.pmts[0].streams[0].codecid = PSI_STREAM_AAC;
//    ts_enc_context->pat.pmts[0].streams[1].pid = 0x102;
//    ts_enc_context->pat.pmts[0].streams[1].sid = PES_SID_VIDEO;
//    ts_enc_context->pat.pmts[0].streams[1].codecid = PSI_STREAM_H264;
//
//    pthread_mutex_init(&mpegWriteLock, NULL);
//    if(NULL!=tsBuf){
//        free(tsBuf);
//        tsBuf=NULL;
//    }
//    tsBuf = malloc(sizeof(char) * TS_PACKET_SIZE);
//
//    const char* cPath=(*env)->GetStringUTFChars(env,path,0);
//    file = open(cPath, O_RDWR | O_CREAT);
//}
//
//JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_write
//        (JNIEnv *env, jobject obj,jint type, jint flag, jlong pts, jlong dts, jbyteArray data,
//         jint len){
//
//    uint16_t stream_type;
//    if (type == 0) {//0表示 video
//        first = 1;
//        stream_type = 0x102;
//    } else {
//        if (first < 0) {
//            LOGE("第一帧不是视频帧，丢弃==========");
//            return 0;
//        }
//        stream_type = 0x101;
//    }
//
//    jbyte *Src_data = malloc(sizeof(jbyte) * len);
//    (*env)->GetByteArrayRegion(env, data, 0, len, Src_data);
//    mpeg_ts_write(ts_enc_context, stream_type, flag, pts, dts, (void *) Src_data, (size_t)len);
//    free(Src_data);
//    return 0;
//}


