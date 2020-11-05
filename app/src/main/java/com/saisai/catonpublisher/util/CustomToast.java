package com.saisai.catonpublisher.util;

import android.content.Context;
import android.view.Gravity;
import android.widget.Toast;


/**
 * Created by saisai on 2016/10/26.
 */

public class CustomToast {
    public static final int LENGTH_MAX = -1;
    private boolean mCanceled = true;
    private android.os.Handler mHandler;
    private Context mContext;
    private Toast mToast;

    private CustomToast(Context context) {
        this(context,new android.os.Handler());
    }


    private CustomToast(Context context, android.os.Handler h) {
        mContext = context;
        mHandler = h;
        mToast = Toast.makeText(mContext,"", Toast.LENGTH_SHORT);
        mToast.setGravity(Gravity.CENTER, 0, 0);
    }

    private static CustomToast customToast=null;

    public static CustomToast makeText(Context context){
        if(customToast==null){
            synchronized (CustomToast.class){
                if(customToast==null){
                    customToast=new CustomToast(context);

                }
            }
        }
        return customToast;
    }


    public void show(int resId,int duration) {

        try {

            mToast.setText(resId);
            if(duration != LENGTH_MAX) {
                mToast.setDuration(duration);
                mToast.show();
            } else if(mCanceled) {
                mToast.setDuration(Toast.LENGTH_LONG);
                mCanceled = false;
                showUntilCancel();
            }
        }catch (Exception e){

        }
    }

    /**
     * @param text 要显示的内容
     * @param duration 显示的时间长
     * 根据LENGTH_MAX进行判断
     * 如果不匹配，进行系统显示
     * 如果匹配，永久显示，直到调用hide()
     */
    public void show(String text, int duration) {
        mToast.setText(text);
        if(duration != LENGTH_MAX) {
            mToast.setDuration(duration);
            mToast.show();
        } else {
            if(mCanceled) {
                mToast.setDuration(Toast.LENGTH_LONG);
                mCanceled = false;
                showUntilCancel();
            }
        }
    }

    /**
     * 隐藏Toast
     */
    public void hide(){
        mToast.cancel();
        mCanceled = true;
    }

    public boolean isShowing() {
        return !mCanceled;
    }

    private void showUntilCancel() {
        if(mCanceled)
            return;
        mToast.show();
        mHandler.postDelayed(new Runnable() {
            public void run() {
                showUntilCancel();
            }
        },3000);
    }
}
