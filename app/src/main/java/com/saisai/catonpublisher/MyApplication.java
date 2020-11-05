package com.saisai.catonpublisher;

import android.app.Application;

import com.saisai.catonpublisher.util.SPUtils;
import com.tencent.bugly.crashreport.CrashReport;

/**
 * Created by saisai on 2019/2/12 0012.
 */

public class MyApplication extends Application{

    public static MyApplication app=null;

    @Override
    public void onCreate() {
        super.onCreate();
        app=this;
        CrashReport.initCrashReport(getApplicationContext(), "03b3058b63", true);
    }
}
