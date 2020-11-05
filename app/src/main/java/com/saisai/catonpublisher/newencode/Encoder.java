package com.saisai.catonpublisher.newencode;

import android.media.MediaCodec;

import java.nio.ByteBuffer;

/**
 * Created by saisai on 2019/3/11 0011.
 */

public interface Encoder {

    /*数据帧*/
    int FRAME_I=1;
    int FRAME_P=0;

    /*流数据类型*/
    int STREAM_VIDEO=0;
    int STREAM_AUDIO=1;

    /*视频编码格式*/
    int CODEC_VODEO_AVC=0x1b;
    int CODEC_VODEO_HEVC=0x24;

    /*音频编码格式*/
    int CODEC_VODEO_AAC=0x0f;

    long TIMEOUT_US = 1000;

    void output(ByteBuffer buffer, MediaCodec.BufferInfo bufferInfo);

    void stopEncoder();
}
