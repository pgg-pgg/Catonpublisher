//
// Created by saisai on 2018/6/26 0026.
//

#include <pthread.h>
#include "r2tp/include/r2tp.h"

typedef int(*R2TPCONNECT_EventHandler)(void *connect, int event, int error, void *userdata);

typedef struct {
    R2tpHandle_t pR2tpHandle;
    char serverHost[IP_LEN_MAX];
    int port;
    char key[AUTHORIZATION_MAX];
    int isSending;//0(no), 1(yes)
    pthread_t tid_receive;
    pthread_mutex_t _mutex;

    R2TPCONNECT_EventHandler eventHandler;
    void *eventUserdata;
} R2tpPushSession_t;

typedef enum SEND_STATE{
    STATE_UDPSEND=0,
    STATE_R2TPSEND,
    STATE_SEND_ERROR,
    STATE_STOP
};

typedef enum ERROR_NUM{
    SUCCESS=0,
    OPEN_ERROR,
    CONNECT_ERROR,
    SETOPT_ERROR,
    GETOPT_ERROR,
    SESSION_ERROR,
};

int Open(int state,const char* ip,int port, const char *service_provider, const char *service_name);

int Connect(const char *host, int port, int auth, int encrypt, const char *strkey, const char *sn,const char *desc);

int SetCallback(R2TPCONNECT_EventHandler eventHandler);

int PutFrame(int streamType,int codecType, int flag, long pts, long dts, const char *data, size_t bytes);

int ParseData(float* dropRate,int* bitrate,int* jitter,int* rtt,int* maxDelay);

int Disconnect();

int spsSetTimingFlag(int codecId,int fps,unsigned char* sps_src,int src_len,unsigned char* sps_dst,int *dst_len);

void GetR2tpVersion(int *major,int *minor,int *revision);



