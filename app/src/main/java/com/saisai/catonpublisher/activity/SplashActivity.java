package com.saisai.catonpublisher.activity;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import com.saisai.catonpublisher.R;
import com.saisai.catonpublisher.util.StateBarTranslucentUtils;
import com.saisai.catonpublisher.util.StatusBarCompat;

/**
 * Created by pgg on 2020/08/29.
 */
public class SplashActivity extends AppCompatActivity {

    private Handler handler = new Handler();
    private Runnable runnableToMain = new Runnable() {
        @Override
        public void run() {
            Intent intent = new Intent(SplashActivity.this, MainActivity.class);
            startActivity(intent);
            finish();
        }
    };


    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        StateBarTranslucentUtils.setStateBarTranslucent(this);
        setContentView(R.layout.activity_splash);
        StatusBarCompat.compat(this);
        handler.postDelayed(runnableToMain, 2000);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        //防止内存泄漏
        handler.removeCallbacks(runnableToMain);
    }
}
