package com.saisai.catonpublisher.newencode;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;

import com.saisai.catonpublisher.Jni;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;

import javax.microedition.khronos.egl.EGLContext;

/**
 * Created by saisai on 2019/3/11 0011.
 */

public abstract class VideoEncoder extends Thread implements Encoder {

    String TAG = VideoEncoder.class.getSimpleName();

    String mMimeType;
    int mVideoCodecId;
    int mWidth;
    int mHeight;
    int mBitrate;
    int mFramerate;
    private MediaCodec mMediaCodec;
    public byte[] config;
    boolean encoding = false;

    private Surface mMediaCodecInputSurface;
    private MediaCodec.BufferInfo mBufferInfo;
    private MediaFormat mVideoFormat;

    public VideoEncoder(String mimeType, int width, int height, int bitrate, int framerate) {
        this.mMimeType = mimeType;
        if (mMimeType.equals(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            mVideoCodecId = CODEC_VODEO_AVC;
        } else if (mMimeType.equals(MediaFormat.MIMETYPE_VIDEO_HEVC)) {
            mVideoCodecId = CODEC_VODEO_HEVC;
        }
        this.mWidth = width;
        this.mHeight = height;
        this.mBitrate = bitrate;
        this.mFramerate = framerate;

        try {
            initMediaCodec();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void initMediaCodec() throws IOException {

        mVideoFormat = MediaFormat.createVideoFormat(this.mMimeType, this.mWidth, this.mHeight);
        mVideoFormat.setInteger(MediaFormat.KEY_FRAME_RATE, this.mFramerate);
        mVideoFormat.setInteger(MediaFormat.KEY_BIT_RATE, this.mBitrate * 1024 * 1024);
        mVideoFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
        mVideoFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        mVideoFormat.setInteger(MediaFormat.KEY_BITRATE_MODE, MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR);
        mMediaCodec = MediaCodec.createEncoderByType(this.mMimeType);
        mMediaCodec.configure(mVideoFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);

        mMediaCodecInputSurface = mMediaCodec.createInputSurface();
        onSurfaceCreate(mMediaCodecInputSurface);
        mBufferInfo = new MediaCodec.BufferInfo();
    }

    public boolean connect = false;


    @Override
    public void run() {
        super.run();

        mMediaCodec.start();
        encoding = true;

        ByteBuffer[] outputBuffers = mMediaCodec.getOutputBuffers();
        while (encoding) {
            try {
                int outputBufferIndex = mMediaCodec.dequeueOutputBuffer(mBufferInfo, 0);
                if (outputBufferIndex >= 0) {
                    ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
                    if (outputBuffer == null) {
                        throw new RuntimeException("Video encoder outputbuffer is null.");
                    }
                    if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                        MediaFormat format = mMediaCodec.getOutputFormat();
                        if (mVideoCodecId == CODEC_VODEO_AVC) {
                            ByteBuffer spsb = format.getByteBuffer("csd-0");
                            ByteBuffer ppsb = format.getByteBuffer("csd-1");
                            byte[] sps = new byte[spsb.capacity()];
                            byte[] pps = new byte[ppsb.capacity()];
                            spsb.get(sps);
                            ppsb.get(pps);
                            sps = spsSetting(sps);
                            config = new byte[sps.length + pps.length];
                            System.arraycopy(sps, 0, config, 0, sps.length);
                            System.arraycopy(pps, 0, config, sps.length, pps.length);
                        } else if (mVideoCodecId == CODEC_VODEO_HEVC) {
                            ByteBuffer spsb = format.getByteBuffer("csd-0");
                            byte[] sps = new byte[spsb.capacity()];
                            spsb.get(sps);
                            sps = spsSetting(sps);
                            config = new byte[sps.length];
                            System.arraycopy(sps, 0, config, 0, sps.length);
                        }
                        mBufferInfo.size = 0;
                    }
                    if (mBufferInfo.size != 0) {
                        output(outputBuffer, mBufferInfo);
                    }
                    mMediaCodec.releaseOutputBuffer(outputBufferIndex, false);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        if (mMediaCodecInputSurface != null) {
            mMediaCodecInputSurface.release();
            mMediaCodecInputSurface = null;
        }
        if (mMediaCodec != null) {
            mMediaCodec.stop();
            mMediaCodec.release();
            mMediaCodec = null;
        }
    }


    protected abstract void onSurfaceCreate(Surface surface);

    protected abstract byte[] spsSetting(byte[] sps);

    @Override
    public void stopEncoder() {
        encoding = false;
    }
}
