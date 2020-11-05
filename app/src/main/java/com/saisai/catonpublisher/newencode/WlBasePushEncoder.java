package com.saisai.catonpublisher.newencode;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.view.Surface;

import com.saisai.catonpublisher.Jni;
import com.saisai.catonpublisher.egl.EglHelper;
import com.saisai.catonpublisher.egl.WLEGLSurfaceView;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLContext;

public abstract class WlBasePushEncoder {

    private Surface mSurface;
    private EGLContext eglContext;

    private int width;
    private int height;
    int mFramerate;

    private long audioPts = 0;
    private int sampleRate;

    private WlEGLMediaThread wlEGLMediaThread;
    public VideoEncoder videoEncodecThread;
    public AudioEncoder audioEncodecThread = null;
    private WLEGLSurfaceView.WlGLRender wlGLRender;

    public final static int RENDERMODE_WHEN_DIRTY = 0;
    public final static int RENDERMODE_CONTINUOUSLY = 1;
    private int mRenderMode = RENDERMODE_CONTINUOUSLY;
    private int mMinAudioBufferSize;

    private OnMediaInfoListener onMediaInfoListener;
    private NewPublisherConfig mConfig;


    public WlBasePushEncoder(Context context) {
    }

    public void setRender(WLEGLSurfaceView.WlGLRender wlGLRender) {
        this.wlGLRender = wlGLRender;
    }

    public void setmRenderMode(int mRenderMode) {
        if (wlGLRender == null) {
            throw new RuntimeException("must set render before");
        }
        this.mRenderMode = mRenderMode;
    }

    public void setOnMediaInfoListener(OnMediaInfoListener onMediaInfoListener) {
        this.onMediaInfoListener = onMediaInfoListener;
    }

    public void initEncodec(EGLContext eglContext, NewPublisherConfig config, int width, int height, int framerate) {
        this.width = width;
        this.height = height;
        this.eglContext = eglContext;
        this.mFramerate = framerate;
        this.mConfig = config;
        initMediaEncodec();
    }

    public void startRecord() {
        if (mSurface != null && eglContext != null) {
            audioPts = 0;
            wlEGLMediaThread.isCreate = true;
            wlEGLMediaThread.isChange = true;
            wlEGLMediaThread.start();
            videoEncodecThread.start();
            audioEncodecThread.start();
        }
    }

    public void stopRecord() {
        if (wlEGLMediaThread != null && videoEncodecThread != null && audioEncodecThread != null) {
            videoEncodecThread.stopEncoder();
            audioEncodecThread.stopEncoder();
            wlEGLMediaThread.onDestory();
            videoEncodecThread = null;
            wlEGLMediaThread = null;
            audioEncodecThread = null;
        }
    }

    private void initMediaEncodec() {
        if (wlEGLMediaThread != null) {
            wlEGLMediaThread.isExit = true;
        }
        if (videoEncodecThread != null) {
            videoEncodecThread.stopEncoder();
        }
        if (audioEncodecThread != null) {
            audioEncodecThread.stopEncoder();
        }
        wlEGLMediaThread = new WlEGLMediaThread(new WeakReference<>(this));
        videoEncodecThread = new VideoEncoder(mConfig.mVideoMimeType, width, height, mConfig.mVideoBitrate, mFramerate) {
            @Override
            protected byte[] spsSetting(byte[] sps) {
                return Jni.getInstance().spsSetTimingFlag(mVideoCodecId, sps, sps.length, mConfig.mVideoFramerate);
            }

            @Override
            protected void onSurfaceCreate(Surface surface) {
                mSurface = surface;
            }

            @Override
            public void output(ByteBuffer buffer, MediaCodec.BufferInfo bufferInfo) {
                onMediaInfoListener.outputVideo(buffer, bufferInfo);
            }
        };
        audioEncodecThread = new AudioEncoder(initAudioRecorder(mConfig), mMinAudioBufferSize, mConfig.mAudioMimeType,
                mConfig.mAudioBitrate, mConfig.mAudioSamplerate) {
            @Override
            public void output(ByteBuffer buffer, MediaCodec.BufferInfo bufferInfo) {
                onMediaInfoListener.outputAudio(buffer, bufferInfo);
            }
        };
    }
    private AudioRecord initAudioRecorder(NewPublisherConfig config) {
        mMinAudioBufferSize = AudioRecord.getMinBufferSize(config.mAudioSamplerate, AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        return new AudioRecord(config.mAudioResource,
                config.mAudioSamplerate,
                AudioFormat.CHANNEL_IN_STEREO,
                AudioFormat.ENCODING_PCM_16BIT, mMinAudioBufferSize);
    }

    static class WlEGLMediaThread extends Thread {
        private WeakReference<WlBasePushEncoder> encoder;
        private EglHelper eglHelper;
        private Object object;

        private boolean isExit = false;
        private boolean isCreate = false;
        private boolean isChange = false;
        private boolean isStart = false;

        public WlEGLMediaThread(WeakReference<WlBasePushEncoder> encoder) {
            this.encoder = encoder;
        }

        @Override
        public void run() {
            super.run();
            isExit = false;
            isStart = false;
            object = new Object();
            eglHelper = new EglHelper();
            eglHelper.initEgl(encoder.get().mSurface, encoder.get().eglContext);

            while (true) {
                if (isExit) {
                    release();
                    break;
                }

                if (isStart) {
                    if (encoder.get().mRenderMode == RENDERMODE_WHEN_DIRTY) {
                        synchronized (object) {
                            try {
                                object.wait();
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    } else if (encoder.get().mRenderMode == RENDERMODE_CONTINUOUSLY) {
                        try {
                            Thread.sleep(1000 / 60);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    } else {
                        throw new RuntimeException("mRenderMode is wrong value");
                    }
                }
                onCreate();
                onChange(encoder.get().width, encoder.get().height);
                onDraw();
                isStart = true;
            }

        }

        private void onCreate() {
            if (isCreate && encoder.get().wlGLRender != null) {
                isCreate = false;
                encoder.get().wlGLRender.onSurfaceCreated();
            }
        }

        private void onChange(int width, int height) {
            if (isChange && encoder.get().wlGLRender != null) {
                isChange = false;
                encoder.get().wlGLRender.onSurfaceChanged(width, height);
            }
        }

        private void onDraw() {
            if (encoder.get().wlGLRender != null && eglHelper != null) {
                encoder.get().wlGLRender.onDrawFrame();
                if (!isStart) {
                    encoder.get().wlGLRender.onDrawFrame();
                }
                eglHelper.swapBuffers();

            }
        }

        private void requestRender() {
            if (object != null) {
                synchronized (object) {
                    object.notifyAll();
                }
            }
        }

        public void onDestory() {
            isExit = true;
            requestRender();
        }

        public void release() {
            if (eglHelper != null) {
                eglHelper.destoryEgl();
                eglHelper = null;
                object = null;
                encoder = null;
            }
        }
    }

    public interface OnMediaInfoListener {
        void outputVideo(ByteBuffer buffer, MediaCodec.BufferInfo bufferInfo);

        void outputAudio(ByteBuffer buffer, MediaCodec.BufferInfo bufferInfo);
    }

}
