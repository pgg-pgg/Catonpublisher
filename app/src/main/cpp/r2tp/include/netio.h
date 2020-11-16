#ifndef __NET_IO_H__
#define __NET_IO_H__


typedef void * NetHandle_t;
#define INVALID_NETHANDLE (NetHandle_t)(~0)

typedef struct {
    NetHandle_t (*NetOpen) (int ip_ver);
    int (*NetClose) (NetHandle_t);
    int (*NetRecv) (NetHandle_t hdl, void *buf, int size, int flags);
    int (*NetRecvFrom) (NetHandle_t hdl, void *buf, int size, int flags, void *src, int *srclen);
    int (*NetSendto) (NetHandle_t hdl, const void *buf, int size, void *dest, int destlen);
    int (*NetSetSndBuf) (NetHandle_t hdl, int size);
    int (*NetSetRcvBuf) (NetHandle_t hdl, int size);
    int (*NetBindDevice) (NetHandle_t hdl, const char *dev);
    int (*NetBind) (NetHandle_t hdl, const char *ip, unsigned short port, int ip_ver);
    int (*NetHasSelect) (NetHandle_t hdl);
} NetIoOps_t;

/*
#include <stdint.h>
#if UINTPTR_MAX == UINT32_MAX
    #define FD_TO_NETHANDLE(fd) ((long)fd)
    #define NETHANDLE_TO_FD(hdl) (long)(hdl)
#elif UINTPTR_MAX == UINT64_MAX
    #define FD_TO_NETHANDLE(fd) ((long)fd)
    #define NETHANDLE_TO_FD(hdl) (long)(hdl)
#else
    #error "Environment not 32 or 64-bit."
#endif
*/
#define FD_TO_NETHANDLE(fd) (NetHandle_t)((long)fd)
#define NETHANDLE_TO_FD(hdl) (int)((long)(hdl))


#endif