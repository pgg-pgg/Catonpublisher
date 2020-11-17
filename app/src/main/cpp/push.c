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
#include <stdio.h>
#include <pthread.h>
#include "pushstream.h"
#include "mpegts/mpeg-ts.h"
//#include "yuv/include/libyuv.h"


#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)


jclass callbackCls;
JavaVM *javaVM;
int sendType;

int eventCallback(void *connect, int event, int error, void *userdata) {

    if (javaVM != NULL) {

        JNIEnv *env;
        (*javaVM)->AttachCurrentThread(javaVM, (void **) &env, 0);

        jmethodID callBack = (*env)->GetStaticMethodID(env, callbackCls, "eventCallback",
                                                       "(II)V");
        (*env)->CallStaticVoidMethod(env, callbackCls, callBack, event, error);
        return 0;
    }

    return 0;
}


JNIEXPORT jint JNICALL Java_com_saisai_catonpublisher_Jni_connect
        (JNIEnv *env, jobject obj, jstring host, jint port, jint auth, jint encrypt, jstring key, jstring sn, jstring desc) {

    const char *cHost = (*env)->GetStringUTFChars(env, host, 0);
    const char *cKey = (*env)->GetStringUTFChars(env, key, 0);
    const char *cSn = (*env)->GetStringUTFChars(env, sn, 0);
    const char *cDesc = (*env)->GetStringUTFChars(env, desc, 0);
    sendType=STATE_R2TPSEND;

    if(sendType==STATE_UDPSEND){
        return Open(sendType, cHost,port);
    } else if(sendType==STATE_R2TPSEND){
        int ret = Open(sendType, NULL, 0);

        if (ret == 0) {
            return Connect(cHost, port, auth, encrypt, cKey, cSn, cDesc);
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

JNIEXPORT jbyteArray JNICALL Java_com_saisai_catonpublisher_Jni_getR2tpVersion
        (JNIEnv *env, jobject obj) {
    int major;
    int minor;
    int revision;
    char input[128] = {0};
    GetR2tpVersion(&major, &minor, &revision);
    LOGE("r2tp version: %d.%d.%d\n", major, minor, revision);
    sprintf(input, "%d.%d.%d", major, minor, revision);
    return input;
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

