//
// Created by saisai on 2019/3/7 0007.
//

enum {
    FPS_UNKNOW,
    FPS_15,
    FPS_24,
    FPS_25,
    FPS_2997,
    FPS_30,
    FPS_5994,
    FPS_60,
    FPS_MAX
};

void *h264_sps_parser_open();

int h264_sps_set_timing_flag(void *handle, unsigned char* to, unsigned int* toSize, unsigned char *from, unsigned int fromSize, int frame_rate_index);

void h264_sps_parser_close(void *handle);


void *h265_sps_parser_open();

int h265_sps_set_timing_flag(void *handle, unsigned char* to, unsigned int* toSize, unsigned char *from, unsigned int fromSize, int frame_rate_index);

void h265_sps_parser_close(void *handle);