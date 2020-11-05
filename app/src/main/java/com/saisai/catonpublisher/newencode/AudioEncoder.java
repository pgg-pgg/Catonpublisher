package com.saisai.catonpublisher.newencode;

import android.media.AudioRecord;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Created by saisai on 2019/3/11 0011.
 */

public abstract class AudioEncoder extends Thread implements Encoder {

    String TAG = AudioEncoder.class.getSimpleName();

    AudioRecord mAudioRecord;
    int mMinBufferSize;
    String mMimeType;
    int mAudioCodecId;
    int mBitRate;
    int mSampleRate;
    private MediaCodec mMediaCodec;
    boolean encoding = false;
    private MediaCodec.BufferInfo mBufferInfo;

    public AudioEncoder(AudioRecord audioRecord, int minBufferSize, String mimeType, int bitrate, int samplerate) {

        this.mAudioRecord = audioRecord;
        this.mMinBufferSize = minBufferSize;
        this.mMimeType = mimeType;
        if(this.mMimeType.equals(MediaFormat.MIMETYPE_AUDIO_AAC)){
            this.mAudioCodecId=CODEC_VODEO_AAC;
        }
        this.mBitRate = bitrate;
        this.mSampleRate = samplerate;

        try {
            initMediacodec();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void initMediacodec() throws IOException {
        MediaFormat audioFormat = MediaFormat.createAudioFormat(this.mMimeType, this.mSampleRate, 2);
        audioFormat.setInteger(MediaFormat.KEY_BIT_RATE, this.mBitRate);
        audioFormat.setInteger(MediaFormat.KEY_BITRATE_MODE, MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR);
        audioFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);

        mMediaCodec = MediaCodec.createEncoderByType(this.mMimeType);
        mMediaCodec.configure(audioFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        mBufferInfo = new MediaCodec.BufferInfo();
    }

    @Override
    public void run() {
        super.run();

        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
        mMediaCodec.start();
        mAudioRecord.startRecording();

        encoding = true;
        WriteThread thread = new WriteThread();
        thread.start();

        while (encoding) {
            try {
                //将数据输入编码器
                int inputBufferIndex = mMediaCodec.dequeueInputBuffer(TIMEOUT_US);
                if (inputBufferIndex >= 0) {
                    ByteBuffer inputBuffer = mMediaCodec.getInputBuffer(inputBufferIndex);
                    inputBuffer.clear();

                    int len = mAudioRecord.read(inputBuffer, mMinBufferSize);
                    if (len == AudioRecord.ERROR_INVALID_OPERATION || len == AudioRecord.ERROR_BAD_VALUE) {
                        mMediaCodec.queueInputBuffer(inputBufferIndex, 0, 0, System.nanoTime() / 1000, 0);
                    } else {
                        mMediaCodec.queueInputBuffer(inputBufferIndex, 0, len, System.nanoTime() / 1000, 0);
                    }
                }


            } catch (Exception e) {

            }

        }

        try {
            synchronized (AudioEncoder.class) {
                if (mAudioRecord != null) {
                    mAudioRecord.stop();
                    mAudioRecord.release();
                    mAudioRecord = null;
                } else {
                    Log.e("tag", "mAudioRecord==null");
                }

                if (mMediaCodec != null) {
                    mMediaCodec.stop();
                    mMediaCodec.release();
                    mMediaCodec = null;
                } else {
                    Log.e("tag", "mMediaCodec==null");
                }
            }

        } catch (Exception e) {

        }


    }

    @Override
    public void stopEncoder() {
        encoding = false;
    }

    class WriteThread extends Thread {
        @Override
        public void run() {
            super.run();

            while (encoding) {
                try {
                    //获取编码后的数据
                    int outputBufferIndex = mMediaCodec.dequeueOutputBuffer(mBufferInfo, TIMEOUT_US);
                    if (outputBufferIndex >= 0) {

                        ByteBuffer outputBuffer = mMediaCodec.getOutputBuffer(outputBufferIndex);
                        if (outputBuffer == null) {
                            throw new RuntimeException("Audio encoder outputBuffer is null.");
                        }

                        output(outputBuffer, mBufferInfo);

                        mMediaCodec.releaseOutputBuffer(outputBufferIndex, false);
                    }
                } catch (Exception e) {

                }

            }
            try {
                synchronized (AudioEncoder.class) {
                    if (mAudioRecord != null) {
                        mAudioRecord.stop();
                        mAudioRecord.release();
                        mAudioRecord = null;
                    } else {
                        Log.e("tag", "mAudioRecord==null");
                    }

                    if (mMediaCodec != null) {
                        mMediaCodec.stop();
                        mMediaCodec.release();
                        mMediaCodec = null;
                    } else {
                        Log.e("tag", "mMediaCodec==null");
                    }
                }
            } catch (Exception e) {

            }

        }
    }

}
