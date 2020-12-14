package com.saisai.catonpublisher;

import com.saisai.catonpublisher.listener.ParseStatListener;
import com.saisai.catonpublisher.listener.PushListener;

/**
 * @Title: Jni
 * @Package com.saisai.catonpublisher.capture
 * @Description:
 * @Author saisai
 * @Date 2018/6/27 0027
 * @Time 15:03
 * @Version
 */
public class Jni {
    static {
//        System.loadLibrary("yuv");
        System.loadLibrary("crypto");
        System.loadLibrary("ssl");
        System.loadLibrary("r2tpv3");
        System.loadLibrary("push");
    }

    private static Jni instance = null;

    private Jni() {
    }

    public static Jni getInstance() {
        if (instance == null) {
            synchronized (Jni.class) {
                if (instance == null) {
                    instance = new Jni();
                }
            }
        }
        return instance;
    }

    /*事件回调事件码*/
    public static final int STATE_UDPSEND = 0;
    public static final int STATE_R2TPSEND = 1;
    public static final int STATE_SEND_ERROR = 2;
    public static final int STATE_STOP = 3;

    /*接口调用返回码*/
    public static final int SUCCESS = 0;
    public static final int OPEN_ERROR = 1;
    public static final int CONNECT_ERROR = 2;
    public static final int SETOPT_ERROR = 3;
    public static final int GETOPT_ERROR = 4;
    public static final int SESSION_ERROR = 5;


    public static final int R2TP_EINVALIDPARAM = -500;
    public static final int R2TP_EBOROKENPIPE = -499;
    public static final int R2TP_ENOMEM = -498;
    public static final int R2TP_EINVALIDSOCK = -497;
    public static final int R2TP_EINVALIDDEV = -496;
    public static final int R2TP_EDUPPACK = -495;
    public static final int R2TP_EINVALIDPACK = -494;
    public static final int R2TP_ENOTFOUND = -493;
    public static final int R2TP_EDROPEXCEED = -492;
    public static final int R2TP_EDUPNACK = -491;
    public static final int R2TP_EINTERUPT = -490;
    public static final int R2TP_EDISORDER = -489;
    public static final int R2TP_EOUTOFRANGE = -488;
    public static final int R2TP_ELISTENING = -487;
    public static final int R2TP_ESTATE = -486;
    public static final int R2TP_EBUSY = -485;
    public static final int R2TP_ESOCK = -484;
    public static final int R2TP_EAUTHORIZE = -483;
    public static final int R2TP_ETIMEOUT = -482;
    public static final int R2TP_ECONCURRENT = -481;//Exceed concurrent connection limitation.



    /**
     * 连接服务器
     *
     * @param host
     * @param port
     * @param key
     * @return
     */
    public native int connect(String host, int port, int auth, int encrypt, String key, String sn, String desc, String service_provider, String service_name);

    /**
     * 将一帧数据推向服务器
     *
     * @param streamType Video{@link #STREAM_VIDEO} or Audio{@link #STREAM_AUDIO}
     * @param codecType AVC{@link #CODEC_VODEO_AVC} or HEVC{@link #CODEC_VODEO_HEVC} or {@link #CODEC_VODEO_AAC}
     * @param flag I frame{@link #FRAME_I} or P frame{@link #FRAME_P}
     * @param pts
     * @param dts
     * @param data
     * @param len
     * @return
     */
    public native int push(int streamType,int codecType, int flag, long pts, long dts, byte[] data, int len);

    /**
     * 断开服务器连接
     *
     * @return
     */
    public native int disconnect();

    /**
     * 设置事件回调函数没，设置成功后，当触发某些事件时会回调java层的eventCallback函数
     *
     * @return
     */
    public native int setCallback();

    /**
     * 调用该接口之后，会在native层回调java层的parseStat函数，将当前的调试信息回调回来
     *
     * @return
     */
    public native int parseStat();

    /**
     * native层回调该函数
     *
     * @param dropRate
     * @param bitrate
     * @param jitter
     * @param rtt
     * @param maxDelay
     */
    public void parseStat(float dropRate, int bitrate, int jitter, int rtt, int maxDelay) {

        if (this.parseStatListener != null) {
            this.parseStatListener.onParse(dropRate, bitrate, jitter, rtt, maxDelay);
        }
    }

    /**
     * native层回调该函数，返回相应的事件和错误码
     *
     * @param event
     * @param error
     */
    public static void eventCallback(int event, int error) {

        if (pushListener != null) {
            pushListener.event(event, error);
        }
    }

    private static PushListener pushListener;

    public void setPushListener(PushListener pushListener) {
        this.pushListener = pushListener;
    }

    private ParseStatListener parseStatListener;

    public void setParseStatListener(ParseStatListener parseStatListener) {
        this.parseStatListener = parseStatListener;
    }

    public native String getR2tpVersion();

    public native byte[] spsSetTimingFlag(int codecType,byte[] sps, int len, int fps);

    public native byte[] h265SpsSetTimingFlag(byte[] sps, int len, int fps);

    public native int initWrite(String path);

    public native int write(int type, int flag, long pts, long dts, byte[] data, int len);
}
