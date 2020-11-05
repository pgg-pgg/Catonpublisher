#include "mpeg-types.h"
#include "mpeg-util.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>

#ifdef ANDROID

#include <jni.h>
#include <android/log.h>
#include <strings.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif


#define H264_NAL_IDR 5
#define H264_NAL_AUD 9

int h264_find_nalu(const uint8_t* p, size_t bytes)
{
    size_t i;
    for (i = 2; i + 1 < bytes; i++)
    {
        if (0x01 == p[i] && 0x00 == p[i - 1] && 0x00 == p[i - 2])
        {
            for (i -= 2; i > 0 && 0 == p[i - 1]; --i)
            {
                // filter trailing zero
            }
            return i;
        }
    }

    return -1;
}

/// @return -1-not found, other-AUD position(include start code)
int find_h264_access_unit_delimiter(const uint8_t* p, size_t bytes)
{
//	LOGE("find 开始");
	size_t i;
	for (i = 2; i + 1 < bytes; i++)
	{
		if (0x01 == p[i] && 0x00 == p[i - 1] && 0x00 == p[i - 2] && H264_NAL_AUD == (p[i + 1] & 0x1f))
		{
            for (i -= 2; i > 0 && 0 == p[i - 1]; --i)
            {
                // filter trailing zero
            }
            return i;
		}
	}
//	LOGE("find 结束");

	return -1;
}

int find_h264_keyframe(const uint8_t* p, size_t bytes)
{
	size_t i;
	uint8_t type;
	for (i = 2; i + 1 < bytes; i++)
	{
		if (0x01 == p[i] && 0x00 == p[i - 1] && 0x00 == p[i - 2])
		{
			type = p[i + 1] & 0x1f;
			if (H264_NAL_IDR >= type && 1 <= type)
				return H264_NAL_IDR == type ? 1 : 0;
		}
	}

	return 0;
}

