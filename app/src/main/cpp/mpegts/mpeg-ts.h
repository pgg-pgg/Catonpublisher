#ifndef _mpeg_ts_h_
#define _mpeg_ts_h_

#include <stdint.h>
#include <stddef.h>

#include "mpeg-ts-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mpeg_ts_func_t
{
	/// alloc new packet
	/// @param[in] param use-defined parameter(by mpeg_ps_create)
	/// @param[in] bytes alloc memory size in byte(default 188)
	/// @return memory pointer
	void* (*alloc)(void* param, size_t bytes);

	/// free packet
	/// @param[in] param use-defined parameter(by mpeg_ps_create)
	/// @param[in] packet PS packet pointer(alloc return pointer)
	void (*free)(void* param, void* packet);

	/// callback on PS packet done
	/// @param[in] param use-defined parameter(by mpeg_ps_create)
	/// @param[in] packet PS packet pointer(alloc return pointer)
	/// @param[in] bytes packet size
	void (*write)(void* param, const void* packet, size_t bytes);
};

typedef struct _mpeg_ts_enc_context_t
{
	struct pat_t pat;

    int64_t pat_period;
    int64_t pat_packet_period;
    int64_t pat_packet_count;
	int64_t pcr_period;
	int64_t pcr_clock; // last pcr time
	int64_t sdt_period;

//	unsigned int pat_period;
//	unsigned int pat_packet_period;
//	unsigned int pat_packet_count;
//	int64_t pcr_period;
//	int64_t pcr_clock; // last pcr time

	struct mpeg_ts_func_t func;
	void* param;

	uint8_t payload[1024]; // maximum PAT/PMT payload length
} mpeg_ts_enc_context_t;

void* mpeg_ts_create(const struct mpeg_ts_func_t *func, void* param, const char *service_provider, const char *service_name);
int mpeg_ts_destroy(void* ts);
int mpeg_ts_add_stream(void* ts, int codecid, const void* extradata, size_t extradata_size);
int mpeg_ts_write(void* ts, int stream, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes);
int mpeg_ts_reset(void* ts);

typedef void (*ts_dumuxer_onpacket)(void* param, int stream, int codecid, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes);

struct ts_demuxer_t;
struct ts_demuxer_t* ts_demuxer_create(ts_dumuxer_onpacket onpacket, void* param);
int ts_demuxer_destroy(struct ts_demuxer_t* demuxer);
size_t ts_demuxer_input(struct ts_demuxer_t* demuxer, const uint8_t* data, size_t bytes);
size_t ts_demuxer_flush(struct ts_demuxer_t* demuxer);

#ifdef __cplusplus
}
#endif
#endif /* !_mpeg_ts_h_ */
