
#ifndef __R2TP_H__
#define __R2TP_H__

#ifdef WIN32
#define R2TP_EXPORT __declspec(dllexport)
#define R2TP_IMPORT __declspec(dllimport)

#ifdef DLLEXPORT
#define R2TP_API R2TP_EXPORT
#else
#define R2TP_API R2TP_IMPORT
#endif
#else
#define R2TP_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif


//#ifdef NAMESPACE_V3

//#define R2TP_DECLARE(FUNC)  r2tpv3_##FUNC

//R2TP_DECLARE(open)

#define r2tp_open r2tpv3_open
#define r2tp_close r2tpv3_close
#define r2tp_bind r2tpv3_bind
#define r2tp_add_interface r2tpv3_add_interface
#define r2tp_remove_interface r2tpv3_remove_interface
#define r2tp_connect r2tpv3_connect
#define r2tp_listen r2tpv3_listen
#define r2tp_accept r2tpv3_accept
#define r2tp_setopt r2tpv3_setopt
#define r2tp_getopt r2tpv3_getopt
#define r2tp_recv r2tpv3_recv
#define r2tp_send r2tpv3_send
#define r2tp_set_log r2tpv3_set_log
#define r2tp_get_version r2tpv3_get_version
//#endif

#include <stdint.h>

typedef void * R2tpHandle_t;
typedef int (*EventHandler) (R2tpHandle_t handle, int event, void *param, void *userdata);

/*Callback for authorization on server side.*/
typedef int (*AuthorizeFunc) (R2tpHandle_t handle, const unsigned char *pAuth, int length, void *userdata);


#define R2TP_VERSION (3)
#define IP_LEN_MAX (64)
#define AUTHORIZATION_MAX (128)

/*logging level*/
enum {
	RLOG_CRIT,
	RLOG_ERR,
	RLOG_WARNING,
	RLOG_INFO,
	RLOG_DEBUG
};
/*
enum{
	R2TP_SET_RCVBUF,
	R2TP_SET_SNDBUF,
	R2TP_GET_BITRATE,
	R2TP_SET_PWD,
	R2TP_GET_STAT,
	R2TP_SET_CONN_TMO,
	R2TP_SET_RCV_TMO,
	R2TP_GET_BANDWIDTH
};



enum{
	R2TP_SENDER,
	R2TP_RECEIVER,
	R2TP_SERVPULL,
	R2TP_SRECV
};

enum R2TP_STATE{
	UNINT,
	INIT ,
	LISTENING,
	CONNECTING,
	CONNECTED,
	BROKEN, 
	CLOSING,
	CLOSED, 
	NONEXIST
};
*/

enum R2TP_STATE{
	CONNECTING,
	CONNECTED,
	BROKEN
};

enum R2TP_MODE {
	R2TP_MODE_SERVER,
	R2TP_MODE_CLIENT,
	R2TP_MODE_RENDEZVOUS
};

enum R2TP_EVENT {
    EVENT_CONGEST, //Param type : uint32_t *; value: estimated bandwidth.
    EVENT_DECONGEST //Param type : uint32_t *; value: estimated bandwidth.
};

enum{
	R2TP_EINVALIDPARAM = (-500),
	R2TP_EBOROKENPIPE,
	R2TP_ENOMEM,
	R2TP_EINVALIDSOCK,
	R2TP_EINVALIDDEV,
	R2TP_EDUPPACK,
	R2TP_EINVALIDPACK,
	R2TP_ENOTFOUND,
	R2TP_EDROPEXCEED,
	R2TP_EDUPNACK,
	R2TP_EINTERUPT, //-490
	R2TP_EDISORDER,
	R2TP_EOUTOFRANGE,
	R2TP_ELISTENING,
	R2TP_ESTATE,
	R2TP_EBUSY,
	R2TP_ESOCK,
	R2TP_EAUTHORIZE,
	R2TP_ETIMEOUT,
	R2TP_ECONCURRENT, //Exceed concurrent connection limitation.
	
	R2TP_EOK = 0
};

enum {
	R2TP_IPV4,
	R2TP_IPV6
};

/*R2TP options*/
typedef enum {
	RO_SNDBUF, // value type: int; Sender buffer in packets number;
	RO_RCVBUF, // value type: int; Receiver buffer in packets number;
	RO_MULTIPATH, // value type: int; If supports multiple path; 1: multi-path; 0: single path.
	RO_STAT, //value type :R2tpStat_t; 
	RO_PACED, //value type: int; If sending packets by pace: 1; if not: 0
	RO_ENABLEFEC, //value type: int; If supporting FEC ; 1: Y, 0: N
	RO_MAXRATE, //value type: int; Maximum source rate (Bytes per sec).
	RO_AGGRESSIVE, //value type: int (default 1);; In aggressive mode, all sub-channels will transfer total traffic in BW detecting stage. 1:Y, 0: N
	RO_REALTIME, // value type: int (default 1); In real-time mode,sending packets maybe dropped instead of blocked. 1:Y; 0:N
	RO_NODELAY,   //(default 1); Nagle algorithm
	RO_CALLBACK_AUTH, //value type: AuthCallbackParam_t .callback data for authorization on Server side.
	RO_AUTHORIZATION, //value type: Authorization_t . Auth info on Client side.
	RO_CALLBACK_EVENT, // callback for event. type: EventHandler
	RO_SOCKSNDBUF,  // socket send buffer size in bytes. type: int
	RO_SOCKRCVBUF,   // socket receive buffer size in bytes. type: int
	RO_R2TPSNDBUF,   // max packet number in r2tp send buffer,  type: int
	RO_R2TPRCVBUF,   // max packet number in r2tp receive buffer,  type: int
	RO_FPDELAYOUTPUT // file pointer of packet delay output.
}R2tpOption_t;

enum {
	R2TP_EPOLL_IN,
	R2TP_EPOLL_OUT,
	R2TP_EPOLL_ERR,
	R2TP_EPOLL_CONGEST,
	R2TP_EPOLL_DECONGEST
};

typedef struct {
	int rtt;
	int jitter;
	int dropped;
	int loss;
	int received; /*total packets in previous time window*/
	int bitrate; /*NOTE: This field had been changed to bytes per second*/
	int state;
	int maxDelay;
	int64_t totalReceived; /*total packets in this session*/
	uint32_t totalLoss; /*total packets lost in this session*/
}NetStat_t;

typedef struct _r2tp_stat{
	char name[64]; //interface name of sub-channel; will return summary of all channels if not set.
    NetStat_t recvStat;//network statistics of incoming stream.
    NetStat_t sendStat; // network statistics of outgoing stream.
}R2tpStat_t;

typedef struct {
	int ip_version;  /*Address type : R2TP_IPV4 or R2TP_IPV6; R2TP_IPV6 not supported yet*/
	char ip[IP_LEN_MAX];
	unsigned short port;
}R2tpAddr_t;

typedef struct{
    unsigned char data[AUTHORIZATION_MAX];
    int length;
}Authorization_t;

typedef struct{
    AuthorizeFunc callback;
    void *pUserdata;
}AuthCallbackParam_t;

#define R2TP_MAXFDSIZE (1024)
typedef struct _r2tp_fdset{
	void* r2tp_fdset[R2TP_MAXFDSIZE];
	char flags[R2TP_MAXFDSIZE];
	int num;
}r2tp_fdset;




/**
  * @brief Open a r2tp session.
  * @param flags Not used.
  * @return r2tp handle.
  */
R2TP_API R2tpHandle_t r2tpv3_open(int flags);

/**
  * @brief Close a r2tp session.
  * @param handle R2tp handle.
  * @return  Error code.
  */
R2TP_API int r2tpv3_close(R2tpHandle_t handle);

/**
  * @brief 
  * @param handle R2tp handle.
  * @return  Error code.
  */
R2TP_API int r2tpv3_bind(R2tpHandle_t handle, const char *dev ,const char *ip, unsigned short port, int priority);

/**
  * @brief  Append a network interface/device to session.
  * @param handle R2tp handle.
  * @param dev Interface/device name.  
  * @param priority Data transfer priority through this interface, [0-100] 100 indicates the highest priority. 
  * @return  Error code.
  */
R2TP_API int r2tpv3_add_interface(R2tpHandle_t handle, const char *dev, int priority);

/**
  * @brief  Remove a network interface/device from session.
  * @param handle R2tp handle.
  * @param dev Interface/device name.
  * @return  Error code.
  */
R2TP_API int r2tpv3_remove_interface(R2tpHandle_t handle, const char *dev);

/**
  * @brief  Connect to the server with addresses specified by  servAddrs.
  * @param handle R2tp handle.
  * @param servAddrs Server addresses.  
  * @param nAddr Number of address. 
  * @param timeout Timeout in us.
  * @return  Error code.
  */
R2TP_API int r2tpv3_connect(R2tpHandle_t handle, R2tpAddr_t *servAddrs, int nAddr, int timeout);

/**
  * @brief  Listen to incoming connections.
  * @param handle R2tp handle.
  * @return  Error code.
  */
R2TP_API int r2tpv3_listen(R2tpHandle_t handle, int concurrent);

/**
  * @brief  r2tp_accept returns a new r2tp session established by the session in listen state (called by r2tp_listen).
  * @param handle R2tp handle.
  * @param usec Timeout in microseconds.  
  * @return  Newly created session.
  */
R2TP_API R2tpHandle_t r2tpv3_accept(R2tpHandle_t handle, int usec);

/**
  * @brief  Set  options associated with the r2tp handle.
  * @param handle R2tp handle.
  * @param opt TO BE FILLED.
  * @param val .  
  * @param size .    
  * @return  Error code.
  */
R2TP_API int r2tpv3_setopt(R2tpHandle_t handle, int opt, void *val, int size);


/**
  * @brief  Get  options associated with the r2tp handle.
  * @param handle R2tp handle.
  * @param opt TO BE FILLED.
  * @param val .  
  * @param size .    
  * @return  Error code.
  */
R2TP_API int r2tpv3_getopt(R2tpHandle_t handle, int opt, void *val, int size);

/**
  * @brief  Receive data 
  * @param handle R2tp handle.
  * @param buffer 
  * @param len  Size of buffer.    
  * @param timeout Timeout in microseconds.
  * @return  Returns number of bytes received, or returns a negative number (error code) if an error occurs.
  */
R2TP_API int r2tpv3_recv(R2tpHandle_t handle, void * buffer, int len, int timeout);

/**
  * @brief  Send  data 
  * @param handle R2tp handle.
  * @param data Data to send. 
  * @param len  Number of bytes about data.    
  * @return  Returns number of bytes received, or returns a negative number (error code) if an error occurs.
  */
R2TP_API int r2tpv3_send(R2tpHandle_t handle, const void * data, int len, int flags);

/**
  * @Deprecated in r2tp v3.
  * @brief Monitor r2tp handles, waiting util one or more handles become readable or writeable .
  * @param nfds  Not used. Just for keeping the prototype same as POSIX select.
  * @param readfds Handle set for read monitoring.
  * @param writefds Handle set for write monitoring.
  * @param exceptfds Handle set for exception monitoring.
  * @param timeout An upper bound on the amount of time in millisecond elasped before r2tp_select returns.
  *    NULL means indefinite blocking.
  * @return Number of handles, 0 means timeout, returns -1 if an error occured.
  */
R2TP_API int r2tpv3_select(int nfds, r2tp_fdset *readfds, r2tp_fdset *writefds, r2tp_fdset *exceptfds, int *timeout);

/**
  * @brief Add a handle to set.
  */
R2TP_API int r2tpv3_fd_set(R2tpHandle_t handle, r2tp_fdset *set);

/**
  * @brief Remove a handle from set.
  */
R2TP_API int r2tpv3_fd_clr(R2tpHandle_t handle, r2tp_fdset *set);

/**
  * @brief Remove all handle from set.
  */
R2TP_API void r2tpv3_fd_zero(r2tp_fdset *set);

R2TP_API int r2tpv3_fd_isset(R2tpHandle_t handle, r2tp_fdset *set);


//R2TP_API int r2tp_set_callback(R2tpHandle_t handle, EventHandler callback, void *userdata);


R2TP_API int r2tpv3_set_log(int level);

R2TP_API void r2tpv3_get_version(int *major, int *minor, int *revision);

R2TP_API void * r2tpv3_epoll_create();

R2TP_API void  r2tpv3_epoll_close(R2tpHandle_t handle);

R2TP_API int r2tpv3_epoll_add(R2tpHandle_t handle, void *sess);

R2TP_API int r2tpv3_epoll_remove(void *handle, void *sess);

R2TP_API int r2tpv3_epoll_wait();

R2TP_API int r2tpv3_pull_connect(void *handle, R2tpAddr_t *servAddrs, int nAddr, char *sn, int timeout);

#ifdef __cplusplus
}
#endif

#endif

