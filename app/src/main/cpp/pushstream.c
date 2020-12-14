//
// Created by saisai on 2018/6/26 0026.
//

#include <malloc.h>
#include <android/log.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/in.h>
#include <endian.h>
#include <arpa/inet.h>
#include "pushstream.h"
#include "ssl/include/openssl/sha.h"
#include "mpegts/mpeg-ts.h"
#include "r2tp/include/r2tp.h"
#include "sps_process.h"
#include "openssl/sha.h"
#include "openssl/crypto.h"

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)


#define TYPE    "VideoConference"
#define SN        "VideoConference"
#define SALT    "3450b54a052210ddd7c482b1c0583de9"
#define TS_PACKET_SIZE 1316

FILE *tsFile = NULL;

pthread_mutex_t mpegWriteLock;
unsigned char *tsBuf;
int ind = 0;
mpeg_ts_enc_context_t *ts_enc_context = NULL;
R2tpPushSession_t *r2tpConnectPush;
//第一个数据帧不是视频帧则丢掉
int first = -1;
int AUDIO_PID = -1;
int VIDEO_PID = -1;

int fd;
struct sockaddr_in addr;
int send_state;

void *mpegalloc(void *param, size_t bytes) {

    if (send_state == STATE_R2TPSEND) {
        if (NULL == r2tpConnectPush) {
            return NULL;
        }
        if (!r2tpConnectPush->isSending) {
            return NULL;
        }
    }
//    else if (send_state == STATE_STOP) {
//        return NULL;
//    }
    uint8_t *packet = (uint8_t *) malloc(bytes);
    return packet;
}

void mpegfree(void *param, void *packet) {

    free(packet);
}

void mpegwrite(void *param, const void *packet, size_t bytes) {

//    pthread_mutex_lock(&mpegWriteLock);

    if (send_state == STATE_R2TPSEND) {
        if (NULL == r2tpConnectPush) {
            return;
        }
        if (!r2tpConnectPush->isSending) {
            return;
        }
    } else if (send_state == STATE_STOP) {
//        pthread_mutex_unlock(&mpegWriteLock);
        return;
    }
    memcpy(tsBuf + ind, packet, bytes);
    ind += bytes;
    if (ind >= TS_PACKET_SIZE) {
        ind = 0;
        switch (send_state) {
            case STATE_UDPSEND: {
                ssize_t ret = sendto(fd, tsBuf, 1316, 0, (struct sockaddr_in *) &addr,
                                     sizeof(addr));
                LOGE("UDP send ret = %d", ret);
                break;
            }
            case STATE_R2TPSEND: {
                pthread_mutex_lock(&r2tpConnectPush->_mutex);
                if (NULL != r2tpConnectPush->pR2tpHandle) {
                    fwrite(tsBuf, sizeof(char), 1316, tsFile);
                    int ret = r2tp_send(r2tpConnectPush->pR2tpHandle, tsBuf, TS_PACKET_SIZE, 0);
                    if (ret != 1316 && NULL != r2tpConnectPush->eventHandler) {
                        r2tpConnectPush->eventHandler(NULL, STATE_SEND_ERROR, ret, NULL);
                    }
                }
                pthread_mutex_unlock(&r2tpConnectPush->_mutex);
                break;
            }
        }

        memset(tsBuf, 0, TS_PACKET_SIZE);
    }
//    pthread_mutex_unlock(&mpegWriteLock);
}


int Open(int state, const char *ip, int port, const char *service_provider, const char* service_name) {

    send_state = state;

    if (state == STATE_R2TPSEND) {
        r2tpConnectPush = (R2tpPushSession_t *) malloc(sizeof(R2tpPushSession_t));
        if (NULL == r2tpConnectPush) {

            LOGE("Open:malloc R2tpPushSession_t error");
            return SESSION_ERROR;
        }
        r2tpConnectPush->pR2tpHandle = r2tp_open(0);
        if (NULL == r2tpConnectPush->pR2tpHandle) {
            free(r2tpConnectPush);
            r2tpConnectPush = NULL;
            LOGE("Open:r2tp_open error");
            return OPEN_ERROR;
        }
    } else {

        fd = socket(AF_INET, SOCK_DGRAM, 0);
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &addr.sin_addr);
    }


    struct mpeg_ts_func_t h;
    h.alloc = mpegalloc;
    h.free = mpegfree;
    h.write = mpegwrite;

    void *param;

    ts_enc_context = mpeg_ts_create(&h, param, service_provider, service_name);
    if (ts_enc_context == NULL) {
        LOGE("Open:ts_enc_context == NULL");
        return -1;
    }


//    //设置 处理的音频和视频格式
//    ts_enc_context->pat.pmts[0].stream_count = 2; // H.264 + AAC
//    ts_enc_context->pat.pmts[0].streams[0].pid = 0x101;
//    ts_enc_context->pat.pmts[0].streams[0].sid = PES_SID_AUDIO;
//    ts_enc_context->pat.pmts[0].streams[0].codecid = PSI_STREAM_AAC;
//    ts_enc_context->pat.pmts[0].streams[1].pid = 0x102;
//    ts_enc_context->pat.pmts[0].streams[1].sid = PES_SID_VIDEO;
//    ts_enc_context->pat.pmts[0].streams[1].codecid = PSI_STREAM_H265;


    if (NULL != tsBuf) {
        free(tsBuf);
        tsBuf = NULL;
    }
    tsBuf = malloc(sizeof(char) * TS_PACKET_SIZE);

    return 0;
}

const char *salt = "3450b54a052210ddd7c482b1c0583de9";
const char *version = "2";    //固定为2
const char *type = "102";  //"102"	Caton Publisher

//采用shsprintfa1加密生成加密字段mdString
static void sha1_str(const char *input, char *mdString) {
    SHA_CTX c;
    unsigned char md[SHA_DIGEST_LENGTH];
    SHA1_Init(&c);
    SHA1_Update(&c, input, strlen(input));
    SHA1_Final(md, &c);
    OPENSSL_cleanse(&c, sizeof(c));
    int i;
    for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&mdString[i * 2], "%02x", (unsigned int) md[i]);
    }
}

//生成认证字符串
static void genAuthString(const char *input, char *output, int encrypt, char *sn, char *desc) {
    char mdString[SHA_DIGEST_LENGTH * 2 + 1];
    memset(mdString, 0, sizeof(mdString));
    sha1_str(input, mdString);
    sprintf(output, "%s:%s:%s:%s:%s:%d", version, type, desc, sn, mdString, encrypt);
}

//设置认证信息
static int
setAuthorization(void *handle, int bAuth, int encrypt, const char *key, char *sn, char *desc) {
    Authorization_t auth;
    memset(&auth, 0, sizeof(Authorization_t));

    char input[128] = {0};
    char output[128] = {0};
    if (bAuth) {
        sprintf(input, "%s:%s:%s:%s", type, sn, key, salt);
    } else {
        sprintf(input, "%s:%s:%s:%s", type, sn, "", salt);
    }

    genAuthString(input, output, encrypt, sn, desc);
    strcpy(auth.data, output);
    auth.length = strlen(output);
    return r2tp_setopt(handle, RO_AUTHORIZATION, &auth, sizeof(auth));
}


//设置加密方式 AES-128/256
static int setEncryptMode(void *handle, int encrypt, const char *key) {
    char encrypt_key[128] = {0};
    int ret = 0;

    sprintf(encrypt_key, "%s:%s", key, salt);

    ret = r2tp_setopt(handle, RO_ENCRYPT_MODE, (void *) &encrypt, sizeof(encrypt));

    if (ENCRYPT_NONE != encrypt) {
        ret = r2tp_setopt(handle, RO_ENCRYPT_KEY, (void *) encrypt_key, strlen(encrypt_key));
    }

    return ret;
}

int Connect(const char *serverHost, int port, int bAuth, int encrypt, const char *key, const char *sn,const char *desc) {

    memset(tsBuf, 0, sizeof(char) * TS_PACKET_SIZE);
    ind = 0;

    if (NULL == r2tpConnectPush) {
        LOGE("Connect:session == NULL");
        return SESSION_ERROR;
    }

    if (NULL == serverHost) {
        LOGE("Connect:NULL == serverHost");
        return -1;
    }

    memset(r2tpConnectPush->serverHost, 0, sizeof(r2tpConnectPush->serverHost));
    strncpy(r2tpConnectPush->serverHost, serverHost, sizeof(r2tpConnectPush->serverHost));
    r2tpConnectPush->serverHost[sizeof(r2tpConnectPush->serverHost) - 1] = '\0';
    r2tpConnectPush->port = port;
    memset(r2tpConnectPush->key, 0, sizeof(r2tpConnectPush->key));
    strncpy(r2tpConnectPush->key, key, sizeof(r2tpConnectPush->key));
    r2tpConnectPush->key[sizeof(r2tpConnectPush->key) - 1] = '\0';
    r2tpConnectPush->isSending = 0;
    pthread_mutex_init(&r2tpConnectPush->_mutex, NULL);
    r2tpConnectPush->eventHandler = NULL;
    r2tpConnectPush->eventUserdata = NULL;
    int major;
    int minor;
    int revision;
    char newSN[1024] = {0};
    r2tp_get_version(&major, &minor, &revision);
    sprintf(newSN, "%s%s%d.%d", sn, "%%", major, minor);
    setAuthorization(r2tpConnectPush->pR2tpHandle, bAuth, encrypt, key, newSN, desc);
    setEncryptMode(r2tpConnectPush->pR2tpHandle, encrypt, key);
    /*connect server*/
    R2tpAddr_t serverAddr = {0};
    serverAddr.ip_version = R2TP_IPV4;
    sprintf(serverAddr.ip, "%s", serverHost);
    serverAddr.port = port;


    int result = -1;
    if ((result = r2tpv3_connect(r2tpConnectPush->pR2tpHandle, &serverAddr, 1, 3000 * 1000)) != 0) {
        LOGE("Connect:r2tp_connect error");
        return result;
    }

    r2tpConnectPush->isSending = 1;

    tsFile = fopen("/storage/emulated/0/h264.ts", "w");

    return 0;
}

int SetCallback(R2TPCONNECT_EventHandler eventHandler) {

    if (NULL == r2tpConnectPush) {
        LOGE("SetCallback:session == NULL");
        return SESSION_ERROR;
    }

    r2tpConnectPush->eventHandler = eventHandler;
    return 0;
}

int PutFrame(int streamType, int codecType, int flag, long pts, long dts, const char *data,
             size_t bytes) {

    if (send_state == STATE_R2TPSEND) {
        if (NULL == r2tpConnectPush) {
            LOGE("PutFrame:r2tpConnectPush==NULL");
            return SESSION_ERROR;
        }

        pthread_mutex_lock(&r2tpConnectPush->_mutex);
        if (!r2tpConnectPush->isSending) {
            pthread_mutex_unlock(&r2tpConnectPush->_mutex);
            LOGE("PutFrame:no sending");
            return -1;
        }
        pthread_mutex_unlock(&r2tpConnectPush->_mutex);
    }

    if (ts_enc_context == NULL) {
        LOGE("PutFrame:ts==NULL");
        return -1;
    }

    LOGE("codecType = %d", codecType);
    uint16_t stream_id;
    if (streamType == 0) {//0表示 video
        first = 1;
        //判断视频流是否初始化
        if (VIDEO_PID == -1) {
            VIDEO_PID = mpeg_ts_add_stream(ts_enc_context, codecType, NULL, 0);
        }
        stream_id = VIDEO_PID;
    } else if (streamType == 1) {
        if (first < 0) {
            LOGE("第一帧不是视频帧，丢弃==========");
            return 0;
        }
//        //判断视频流是否初始化
        if (AUDIO_PID == -1) {
            AUDIO_PID = mpeg_ts_add_stream(ts_enc_context, codecType, NULL, 0);
        }
        stream_id = AUDIO_PID;
    }

    if (VIDEO_PID == -1 || AUDIO_PID == -1) {
        LOGE("音频或视频没有初始化完成，请等待。。。。");
        return 0;
    }

    pthread_mutex_lock(&mpegWriteLock);
    mpeg_ts_write(ts_enc_context, stream_id, flag, pts, dts, (void *) data, bytes);
    pthread_mutex_unlock(&mpegWriteLock);

    free(data);
    data = NULL;
    return 0;
}

int ParseData(float *dropRate, int *bitrate, int *jitter, int *rtt, int *maxDelay) {

    if (send_state != STATE_R2TPSEND) {
        return 0;
    }

    if (NULL == r2tpConnectPush) {
        LOGE("ParseData:session == null");
        return SESSION_ERROR;
    }

    if (!r2tpConnectPush->isSending) {
        LOGE("ParseData:push stream stop");
        return -1;
    }

    R2tpStat_t st;
    int statLen = sizeof(st);
    memset(&st, 0, sizeof(st));
    pthread_mutex_lock(&r2tpConnectPush->_mutex);
    if (r2tp_getopt(r2tpConnectPush->pR2tpHandle, RO_STAT, &st, &statLen) == -1) {
        LOGE("ParsePullStreamData:r2tp_getopt error");
        pthread_mutex_unlock(&r2tpConnectPush->_mutex);
        return GETOPT_ERROR;
    }
    pthread_mutex_unlock(&r2tpConnectPush->_mutex);

    uint32_t totalLoss = st.sendStat.totalLoss;
    int64_t totalReceived = st.sendStat.totalReceived;
    if (totalLoss > 0 && totalReceived > 0) {
        *dropRate = (float) (totalLoss * 100) * 1.0f / (totalLoss + totalReceived);
    }

    *bitrate = st.sendStat.bitrate * 8;
    *jitter = st.sendStat.jitter;
    *rtt = st.sendStat.rtt;
    *maxDelay = st.sendStat.maxDelay;

    if (*dropRate < 0 || *dropRate > 100)
        *dropRate = 0;
    if (*bitrate < 0)
        *bitrate = 0;
    if (*jitter < 0)
        *jitter = 0;
    if (*rtt < 0)
        *rtt = 0;
    if (*maxDelay < 0)
        *maxDelay = 0;

    return 0;
}

int Disconnect() {

    pthread_mutex_lock(&mpegWriteLock);

    if (tsBuf != NULL) {
        free(tsBuf);
        tsBuf = NULL;
    }
    ind = 0;
    first = -1;
    VIDEO_PID = -1;
    AUDIO_PID = -1;

    if (tsFile != NULL) {
        fclose(tsFile);
        tsFile = NULL;
    }

    if (send_state == STATE_UDPSEND) {
        close(fd);
    }

    send_state = STATE_STOP;

    mpeg_ts_destroy(ts_enc_context);

    if (NULL == r2tpConnectPush) {
        LOGE("NULL == r2tpConnectPush");
        pthread_mutex_unlock(&mpegWriteLock);
        return 0;
    }


    pthread_mutex_lock(&r2tpConnectPush->_mutex);
    if (r2tpConnectPush->isSending) {
        r2tpConnectPush->isSending = 0;
    }
    r2tp_close(r2tpConnectPush->pR2tpHandle);

    pthread_mutex_unlock(&r2tpConnectPush->_mutex);

    free(r2tpConnectPush);

    r2tpConnectPush = NULL;

    pthread_mutex_unlock(&mpegWriteLock);

    return 0;
}

int
spsSetTimingFlag(int codecId, int fps, unsigned char *sps_src, int src_len, unsigned char *sps_dst,
                 int *dst_len) {
    int framerate_index = FPS_UNKNOW;
    switch (fps) {
        case 15: {
            framerate_index = FPS_15;
            break;
        }
        case 24: {
            framerate_index = FPS_24;
            break;
        }
        case 25: {
            framerate_index = FPS_25;
            break;
        }
        case 30: {
            framerate_index = FPS_30;
            break;
        }
    }

    void *h;
    if (codecId == PSI_STREAM_H264) {
        h = h264_sps_parser_open();
        h264_sps_set_timing_flag(h, sps_dst, dst_len, sps_src, src_len, framerate_index);
        h264_sps_parser_close(h);
    } else if (codecId == PSI_STREAM_H265) {
        h = h265_sps_parser_open();
        h265_sps_set_timing_flag(h, sps_dst, dst_len, sps_src, src_len, framerate_index);
        h265_sps_parser_close(h);
    }
    return 0;
}

void GetR2tpVersion(int *major, int *minor, int *revision) {

    r2tp_get_version(major, minor, revision);
}

