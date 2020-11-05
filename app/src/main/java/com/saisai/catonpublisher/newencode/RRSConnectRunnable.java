package com.saisai.catonpublisher.newencode;

import android.util.Log;

import com.saisai.catonpublisher.Jni;
import com.saisai.catonpublisher.listener.IConnectStateListener;
import com.saisai.catonpublisher.listener.ParseStatListener;
import com.saisai.catonpublisher.listener.PushListener;

/**
 * Created by saisai on 2019/3/12 0012.
 */
public class RRSConnectRunnable extends Thread {

    String mHost;
    int mPort;
    String mKey;

    //是否需要连接
    boolean connect = true;

    ConnectListener mOnConnectListener;
    IConnectStateListener mOnConnectStateListener;

    public RRSConnectRunnable(String host, int port, String key, ConnectListener connectListener, IConnectStateListener stateListener) {
        this.mHost = host;
        this.mPort = port;
        this.mKey = key;
        this.mOnConnectListener=connectListener;
        this.mOnConnectStateListener = stateListener;

        Jni.getInstance().setParseStatListener(connectListener);
        Jni.getInstance().setPushListener(connectListener);
    }

    @Override
    public void run() {
        super.run();
        boolean isFirst = true;
        int result = -1;
        do {
            if (!isFirst) {
                try {
                    Thread.sleep(1500);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            isFirst = false;

            result = Jni.getInstance().connect(this.mHost, this.mPort, this.mKey);
            mOnConnectStateListener.onConnectState(result);
            //判断 是否 需要继续 连接
            if (!connect) {
                return;
            }

            Log.e("rrs connect", "runable :connect result = "+result);

        } while (result != 0);

        if (this.mOnConnectListener != null) {
            this.mOnConnectListener.onConnected();
        }

        Jni.getInstance().setCallback();

        while (connect) {

            Jni.getInstance().parseStat();
            try {
                Thread.sleep(1500);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

    }

    public void disConnect() {
        connect = false;
        if(mOnConnectListener!=null){
            mOnConnectListener.onDisConnected();
        }
        Jni.getInstance().disconnect();
    }

    public static abstract class ConnectListener implements PushListener, ParseStatListener {
        abstract void onConnected();

        abstract void onDisConnected();
    }
}
