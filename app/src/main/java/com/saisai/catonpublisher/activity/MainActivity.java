package com.saisai.catonpublisher.activity;

import android.Manifest;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.RequiresApi;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.TextView;

import com.saisai.catonpublisher.R;
import com.saisai.catonpublisher.adapter.MainVpAdapter;
import com.saisai.catonpublisher.newencode.NewPublisherConfig;
import com.saisai.catonpublisher.newencode.SettingConfig;
import com.saisai.catonpublisher.permission.DialogUtil;
import com.saisai.catonpublisher.permission.PermissionHelper;
import com.saisai.catonpublisher.permission.RequestCode;
import com.saisai.catonpublisher.permission.XPermissionUtils;
import com.saisai.catonpublisher.util.SPUtils;
import com.saisai.catonpublisher.util.StateBarTranslucentUtils;
import com.saisai.catonpublisher.util.StatusBarCompat;
import com.saisai.catonpublisher.view.NoScrollViewPager;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private TextView mTvLive;
    private TextView mTvSetting;
    private NoScrollViewPager mVpMain;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        StateBarTranslucentUtils.setStateBarTranslucent(this);
        setContentView(R.layout.activity_home_page);
        StatusBarCompat.compat(this);
        mTvLive = findViewById(R.id.main_live);
        mTvSetting = findViewById(R.id.main_setting);
        mTvSetting.setOnClickListener(this);
        mTvLive.setOnClickListener(this);
        mVpMain = findViewById(R.id.vp_main);
        mVpMain.setAdapter(new MainVpAdapter(getSupportFragmentManager()));
        mVpMain.setNoScroll(true);
        selectTab(true);
        List<NewPublisherConfig> list = SPUtils.getLiveData();
        if (list == null) {
            NewPublisherConfig config = new NewPublisherConfig();
            config.liveTitle = "添加直播";
            list = new ArrayList<>();
            list.add(config);
            SPUtils.saveLiveData(list);
        }
        SettingConfig settingConfig = SPUtils.getSettingConfig();
        if (settingConfig == null) {
            SPUtils.saveSetting(new SettingConfig());
        }
    }

    private void selectTab(boolean isLive) {
        if (isLive) {
            mTvLive.setTextColor(ContextCompat.getColor(this, R.color.select_text_color));
            mTvSetting.setTextColor(ContextCompat.getColor(this, R.color.normal_text_color));
            mVpMain.setCurrentItem(0);
        } else {
            mTvLive.setTextColor(ContextCompat.getColor(this, R.color.normal_text_color));
            mTvSetting.setTextColor(ContextCompat.getColor(this, R.color.select_text_color));
            mVpMain.setCurrentItem(1);
        }
    }

    //危险权限（运行时权限）
    static final String[] PERMISSIONS = new String[]{
            Manifest.permission.CAMERA,
            Manifest.permission.CAPTURE_AUDIO_OUTPUT,
            Manifest.permission.MODIFY_AUDIO_SETTINGS,
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_CONTACTS,
            Manifest.permission.ACCESS_FINE_LOCATION
    };
    boolean havePermission = false;

    @Override
    protected void onStart() {
        super.onStart();
        XPermissionUtils.requestPermissions(this, RequestCode.MORE, PERMISSIONS,
                new XPermissionUtils.OnPermissionListener() {
                    @Override
                    public void onPermissionGranted() {
                        havePermission = true;
                        if (!PermissionHelper.isCameraEnable()) {
                            DialogUtil.showPermissionManagerDialog(MainActivity.this, "相机");
                            havePermission = false;
                        }
                        if (!PermissionHelper.isAudioEnable()) {
                            DialogUtil.showPermissionManagerDialog(MainActivity.this, "录音");
                            havePermission = false;
                        }
                    }

                    @Override
                    public void onPermissionDenied(final String[] deniedPermissions, boolean alwaysDenied) {
                        havePermission = true;
                        if (alwaysDenied) { // 拒绝后不再询问 -> 提示跳转到设置
//                            DialogUtil.showPermissionManagerDialog(VideoActivity.this, "相机");

                            for (String per : deniedPermissions) {
                                if (per.equals(Manifest.permission.CAMERA)) {
                                    DialogUtil.showPermissionManagerDialog(MainActivity.this, "相机");
                                    havePermission = false;
                                    continue;
                                }
                                if (per.equals(Manifest.permission.RECORD_AUDIO)) {
                                    DialogUtil.showPermissionManagerDialog(MainActivity.this, "录音");
                                    havePermission = false;
                                    continue;
                                }
                            }

                        } else {    // 拒绝 -> 提示此公告的意义，并可再次尝试获取权限
                            new AlertDialog.Builder(MainActivity.this).setTitle("温馨提示")
                                    .setMessage("我们需要相机权限才能正常使用该功能")
                                    .setNegativeButton("取消", null)
                                    .setPositiveButton("验证权限", new DialogInterface.OnClickListener() {
                                        @RequiresApi(api = Build.VERSION_CODES.M)
                                        @Override
                                        public void onClick(DialogInterface dialog, int which) {
                                            XPermissionUtils.requestPermissionsAgain(MainActivity.this, deniedPermissions,
                                                    RequestCode.MORE);
                                        }
                                    })
                                    .show();
                            havePermission = false;
                        }

                    }
                });
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.main_live:
                selectTab(true);
                break;
            case R.id.main_setting:
                selectTab(false);
                break;
        }
    }
}
