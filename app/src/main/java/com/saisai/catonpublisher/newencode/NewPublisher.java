package com.saisai.catonpublisher.newencode;

import android.content.Context;
import android.graphics.Bitmap;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaCodec;
import android.os.Environment;
import android.util.Log;
import android.util.Range;
import android.util.Size;
import android.view.Surface;

import com.alibaba.fastjson.JSONObject;
import com.saisai.catonpublisher.Jni;
import com.saisai.catonpublisher.listener.IConnectStateListener;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLContext;

import static com.saisai.catonpublisher.newencode.Encoder.FRAME_I;
import static com.saisai.catonpublisher.newencode.Encoder.FRAME_P;
import static com.saisai.catonpublisher.newencode.Encoder.STREAM_AUDIO;
import static com.saisai.catonpublisher.newencode.Encoder.STREAM_VIDEO;

/**
 * Created by saisai on 2019/3/12 0012.
 */

public class NewPublisher extends RRSConnectRunnable.ConnectListener {

    String TAG = NewPublisher.class.getSimpleName();

    public static final int CAMERA_IS_SUPPORT = 0;
    public static final int CAMERA_NOT_SUPPORT_RESOLUTION = -1;
    public static final int CAMERA_NOT_SUPPORT_FPS = -2;

    private static NewPublisher instance = null;
    private WlPushEncodec pushEncodec;
    private NewPublisherConfig mPublisherConfig;
    private RRSConnectRunnable mConnectRunnable;

    boolean isConnect = false;
    private byte[] mAudioFrame;

    private NewPublisher() {

    }

    public static NewPublisher getInstance() {
        if (instance == null) {
            synchronized (NewPublisher.class) {
                if (instance == null) {
                    instance = new NewPublisher();
                }
            }
        }
        return instance;
    }

    public void setConfig(NewPublisherConfig publishConfig) {
        mPublisherConfig = publishConfig;
    }

    public void init(Context context, EGLContext eglContext, int textureId, boolean isVerity) {
        try {
            Size outputSize = getOutputSize();
            Range<Integer> fps = getFps();
            if (pushEncodec != null) {
                pushEncodec.stopRecord();
            }
            pushEncodec = new WlPushEncodec(context, textureId);
            int width = isVerity ? outputSize.getHeight() : outputSize.getWidth();
            int height = isVerity ? outputSize.getWidth() : outputSize.getHeight();
            pushEncodec.initEncodec(eglContext, mPublisherConfig, width, height, fps.getLower());
            pushEncodec.setOnMediaInfoListener(new WlBasePushEncoder.OnMediaInfoListener() {
                @Override
                public void outputVideo(ByteBuffer buffer, MediaCodec.BufferInfo bufferInfo) {
                    if (!isConnect) {
                        return;
                    }
                    Log.e(TAG, "video encoder start time = " + bufferInfo.presentationTimeUs / 1000);

                    buffer.position(bufferInfo.offset);
                    buffer.limit(bufferInfo.offset + bufferInfo.size);
                    byte[] config = pushEncodec.videoEncodecThread.config;
                    byte[] temp;
                    if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_KEY_FRAME) {
                        temp = new byte[bufferInfo.size + config.length];
                        System.arraycopy(config, 0, temp, 0, config.length);
                        buffer.get(temp, config.length, bufferInfo.size);
                        Jni.getInstance().push(
                                STREAM_VIDEO,
                                pushEncodec.videoEncodecThread.mVideoCodecId,
                                FRAME_I,
                                bufferInfo.presentationTimeUs / 1000 * 90,
                                bufferInfo.presentationTimeUs / 1000 * 90,
                                temp,
                                temp.length);
                    } else {
                        temp = new byte[bufferInfo.size];
                        buffer.get(temp);
                        Jni.getInstance().push(
                                STREAM_VIDEO,
                                pushEncodec.videoEncodecThread.mVideoCodecId,
                                FRAME_P,
                                bufferInfo.presentationTimeUs / 1000 * 90,
                                bufferInfo.presentationTimeUs / 1000 * 90,
                                temp,
                                temp.length);
                    }
                }

                @Override
                public void outputAudio(ByteBuffer buffer, MediaCodec.BufferInfo bufferInfo) {
                    if (!isConnect) {
                        return;
                    }

                    //给adts头字段空出7的字节
                    int length = bufferInfo.size + 7;
                    if (mAudioFrame == null || mAudioFrame.length < length) {
                        mAudioFrame = new byte[length];
                    }
                    addADTStoPacket(mAudioFrame, length);
                    buffer.get(mAudioFrame, 7, bufferInfo.size);

                    Jni.getInstance().push(
                            STREAM_AUDIO,
                            pushEncodec.audioEncodecThread.mAudioCodecId,
                            FRAME_I,
                            bufferInfo.presentationTimeUs / 1000 * 90,
                            bufferInfo.presentationTimeUs / 1000 * 90,
                            mAudioFrame,
                            length);
                }
            });
            pushEncodec.startRecord();
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("owner:NewPublisher:init", e.getMessage());
        }
    }

    public void setBitmap(Bitmap bitmap) {
        if (pushEncodec != null) {
            pushEncodec.setBitmap(bitmap);
        }
    }

    public void startPush(IConnectStateListener callback) {
        mConnectRunnable = new RRSConnectRunnable(mPublisherConfig.mHost,
                mPublisherConfig.mPort,
                mPublisherConfig.mAuth,
                mPublisherConfig.mEncrypt,
                mPublisherConfig.mKey,
                this, callback);

        mConnectRunnable.start();

    }

    public void stopPush() {
        mConnectRunnable.disConnect();
    }

    public void close() {
        if (pushEncodec != null) {
            pushEncodec.stopRecord();
        }
    }

    public Size getOutputSize() {

        Object size = mPublisherConfig.mSupportResolution
                .get(mPublisherConfig.mSwitchCameraFacing)
                .get(mPublisherConfig.mVideoResolution);
        if (size instanceof JSONObject) {
            Integer width = (Integer) ((JSONObject) size).get("width");
            Integer height = (Integer) ((JSONObject) size).get("height");
            return new Size(width, height);
        }

        return null;

    }

    public Range<Integer> getFps() {
        Object range = mPublisherConfig.mSupportFps
                .get(mPublisherConfig.mSwitchCameraFacing)
                .get(mPublisherConfig.mVideoFramerate);
        if (range instanceof JSONObject) {
            Integer lower = (Integer) ((JSONObject) range).get("lower");
            Integer upper = (Integer) ((JSONObject) range).get("upper");
            return new Range<Integer>(lower, upper);
        }
        return null;
    }

    public boolean showDebug() {
        return mPublisherConfig.showDebug;
    }

    public int getCurrentCameraFacing() {
        return mPublisherConfig.mSwitchCameraFacing;
    }

    /**
     * 检查指定摄像头是否支持当前配置的参数
     *
     * @param cameraFacing
     * @return 0：支持，-1：分辨率不支持，-2：帧率不支持
     */
    public int checkSupportCamera(int cameraFacing) {

        if (mPublisherConfig.mSupportResolution
                .get(cameraFacing)
                .get(mPublisherConfig.mVideoResolution) == null) {
            return CAMERA_NOT_SUPPORT_RESOLUTION;
        }

        if (mPublisherConfig.mSupportFps
                .get(cameraFacing)
                .get(mPublisherConfig.mVideoFramerate) == null) {
            return CAMERA_NOT_SUPPORT_FPS;
        }

        return CAMERA_IS_SUPPORT;
    }

    @Override
    public void onParse(float dropRate, int bitrate, int jitter, int rtt, int maxDelay) {

        Log.e(TAG, "dropRate = " + dropRate + ",bitrate = " + bitrate + ",rtt = " + rtt);
        if (this.onTransferStatListener != null) {
            this.onTransferStatListener.onStat(dropRate, bitrate, jitter, rtt, maxDelay);
        }
    }

    @Override
    public void event(int event, int error) {

    }

    @Override
    void onConnected() {
        isConnect = true;
    }

    @Override
    void onDisConnected() {
        isConnect = false;
    }


    /**
     * 给编码出的aac裸流添加adts头字段
     *
     * @param packet    要空出前7个字节，否则会搞乱数据
     * @param packetLen
     */
    private void addADTStoPacket(byte[] packet, int packetLen) {
        int profile = 2;  //AAC LC
        int freqIdx = 4;  //44.1KHz
        int chanCfg = 2;  //CPE
        packet[0] = (byte) 0xFF;
        packet[1] = (byte) 0xF9;
        packet[2] = (byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
        packet[3] = (byte) (((chanCfg & 3) << 6) + (packetLen >> 11));
        packet[4] = (byte) ((packetLen & 0x7FF) >> 3);
        packet[5] = (byte) (((packetLen & 7) << 5) + 0x1F);
        packet[6] = (byte) 0xFC;
    }


    public interface TransferStatListener {
        void onStat(float dropRate, int bitrate, int jitter, int rtt, int maxDelay);
    }

    private TransferStatListener onTransferStatListener;

    public void setOnTransferStatListener(TransferStatListener onTransferStatListener) {
        this.onTransferStatListener = onTransferStatListener;
    }
}
