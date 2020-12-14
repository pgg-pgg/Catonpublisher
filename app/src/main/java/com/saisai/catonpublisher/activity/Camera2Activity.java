package com.saisai.catonpublisher.activity;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.WindowManager;
import android.widget.Chronometer;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.saisai.catonpublisher.BuildConfig;
import com.saisai.catonpublisher.Constants;
import com.saisai.catonpublisher.R;
import com.saisai.catonpublisher.camera.WlCameraView;
import com.saisai.catonpublisher.listener.IConnectStateListener;
import com.saisai.catonpublisher.newencode.NewPublisher;
import com.saisai.catonpublisher.newencode.NewPublisherConfig;
import com.saisai.catonpublisher.newencode.RRSConnectRunnable;
import com.saisai.catonpublisher.newencode.SettingConfig;
import com.saisai.catonpublisher.util.ImageExt;
import com.saisai.catonpublisher.util.SPUtils;
import com.saisai.catonpublisher.util.StateBarTranslucentUtils;
import com.saisai.catonpublisher.util.StatusBarCompat;
import com.tencent.bugly.crashreport.CrashReport;

public class Camera2Activity extends AppCompatActivity implements View.OnClickListener {
    private TextView tvBitrate;
    private TextView tvRtt;
    private TextView tvDroprate;
    private TextView tvJitter;
    private TextView tvMaxdelay;
    private TextView mBtnStart;
    private Chronometer mTvTime;
    private TextView mTvDanmu;
    private TextView mTvCountDown;
    private ImageView mIvAudio;
    private ImageView mIvVideo;
    private LinearLayout mLlVerContainer;
    private LinearLayout mLlLandContainer;
    private ImageView mIvLandAudio;
    private ImageView mIvLandVideo;
    private TextView mBtnLandStart;
    private View mViewBlack;
    private boolean showDebug;
    private View mDebugView;
    private boolean mIsAudioValid;
    private boolean mIsVideoValid;
    private TextView mTvSetting;
    private ImageView mIvBackIcon;
    private RelativeLayout mRlTitle;
    private View mPlaceView;
    private FrameLayout mFlBack;

    private WlCameraView mTextureView;
    private NewPublisherConfig mConfig;
    private SettingConfig mSettingConfig;
    private AudioManager mAudioManager;
    private MyHandler mHandler = new MyHandler();
    private boolean isCountDowning = false;
    boolean isConnect = false;
    boolean isVerity = true;

    private class MyHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case 1:
                    try {
                        int count = (int) msg.obj;
                        mTvCountDown.setText(String.valueOf(count));
                        if (count > 0) {
                            Message msg1 = obtainMessage();
                            msg1.what = 1;
                            msg1.obj = count - 1;
                            sendMessageDelayed(msg1, 1000);
                        } else {
                            NewPublisher.getInstance().init(Camera2Activity.this, mTextureView.getEglContext(), mTextureView.getTextureId(), isVerity);
                            NewPublisher.getInstance().startPush(new IConnectStateListener() {
                                @Override
                                public void onConnectState(int result) {
                                    if (result == 0) {
                                        mBtnStart.post(new Runnable() {
                                            @Override
                                            public void run() {
                                                isCountDowning = false;
                                                mTvCountDown.setVisibility(View.GONE);
                                                isConnect = true;
                                                mBtnStart.setText(getString(R.string.live_finish));
                                                mBtnLandStart.setText(getString(R.string.live_finish));
                                                mBtnStart.setBackgroundColor(Camera2Activity.this.getResources().getColor(R.color.red));
                                                mBtnLandStart.setBackgroundColor(Camera2Activity.this.getResources().getColor(R.color.red));
                                                mTvTime.setBase(SystemClock.elapsedRealtime());//计时器清零
                                                mTvTime.start();
                                            }
                                        });
                                    } else {
                                        mBtnStart.post(new Runnable() {
                                            @Override
                                            public void run() {
                                                Toast.makeText(Camera2Activity.this, Camera2Activity.this.getString(R.string.no_connect_server), Toast.LENGTH_SHORT).show();
                                            }
                                        });
                                    }
                                }
                            });
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                        Log.e("owner:handlemsg", e.getMessage());
                        CrashReport.postCatchedException(e);
                    }

                    break;
                default:
                    break;
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            long liveId = getIntent().getExtras().getLong(Constants.KEY_LIVE_ID, 0L);
            mConfig = SPUtils.getNewPublisherConfig(liveId);
            mSettingConfig = SPUtils.getSettingConfig();
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);   //应用运行时，保持屏幕高亮，不锁屏
            if (mConfig == null) {
                finish();
            }
            NewPublisher.getInstance().setConfig(mConfig, mSettingConfig);
            StateBarTranslucentUtils.setStateBarTranslucent(this);
            setContentView(R.layout.activity_camera2);
            StatusBarCompat.compat(this);
            mAudioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
            initView();
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("owner:onCreate", e.getMessage());
            CrashReport.postCatchedException(e);
        }

    }

    private void initView() {
        mTextureView = findViewById(R.id.previewSurface);
        mTextureView.resize(true);
        isVerity = true;
        mDebugView = findViewById(R.id.layout_debug);
        tvDroprate = findViewById(R.id.tv_droprate);
        tvBitrate = findViewById(R.id.tv_bitrate);
        tvJitter = findViewById(R.id.tv_jitter);
        tvRtt = findViewById(R.id.tv_rtt);
        tvMaxdelay = findViewById(R.id.tv_maxdelay);
        TextView mTvLiveTitle = findViewById(R.id.tv_live_title);
        mTvTime = findViewById(R.id.tv_time);
        mTvDanmu = findViewById(R.id.tv_danmu);
        mIvAudio = findViewById(R.id.iv_image_audio);
        mIvVideo = findViewById(R.id.iv_image_video);
        mBtnStart = findViewById(R.id.btn_start);
        mLlVerContainer = findViewById(R.id.ll_control_container);
        mLlLandContainer = findViewById(R.id.ll_land_control_container);
        mBtnLandStart = findViewById(R.id.btn_land_start);
        mIvLandVideo = findViewById(R.id.iv_land_image_video);
        mIvLandAudio = findViewById(R.id.iv_land_image_audio);
        mLlVerContainer.setVisibility(View.VISIBLE);
        mLlLandContainer.setVisibility(View.GONE);
        mBtnStart.setBackgroundColor(getResources().getColor(R.color.select_text_color));
        mBtnLandStart.setBackgroundColor(getResources().getColor(R.color.select_text_color));
        mTvCountDown = findViewById(R.id.count_text);
        mViewBlack = findViewById(R.id.view_black);
        mIvAudio.setOnClickListener(this);
        mIvLandAudio.setOnClickListener(this);
        mIvVideo.setOnClickListener(this);
        mIvLandVideo.setOnClickListener(this);
        mBtnStart.setOnClickListener(this);
        mBtnLandStart.setOnClickListener(this);
        findViewById(R.id.trans_camera).setOnClickListener(this);
        findViewById(R.id.ib_switch).setOnClickListener(this);
        mFlBack = findViewById(R.id.iv_back);
        mFlBack.setOnClickListener(this);
        mTvSetting = findViewById(R.id.tv_setting);
        mIvBackIcon = findViewById(R.id.iv_back_icon);
        mRlTitle = findViewById(R.id.rl_title);
        mPlaceView = findViewById(R.id.place_view);
        mPlaceView.setVisibility(View.VISIBLE);
        mIvBackIcon.setEnabled(true);
        mTvSetting.setOnClickListener(this);
        findViewById(R.id.root).setOnClickListener(this);
        mTvLiveTitle.setText(mConfig.liveTitle);
        mTvTime.setText(R.string.live_time);
        if (mSettingConfig != null) {
            if (TextUtils.isEmpty(mSettingConfig.danmuText) || !mSettingConfig.isShowDanmu) {
                mTvDanmu.setVisibility(View.GONE);
            } else {
                mTvDanmu.setText(mSettingConfig.danmuText);
                mTvDanmu.setVisibility(View.VISIBLE);
            }
            if (mSettingConfig.isShowTime) {
                mTvTime.setBase(SystemClock.elapsedRealtime());
                mTvTime.setVisibility(View.VISIBLE);
            } else {
                mTvTime.setVisibility(View.GONE);
            }
        }
        showDebug = NewPublisher.getInstance().showDebug();
        NewPublisher.getInstance().setOnTransferStatListener(new NewPublisher.TransferStatListener() {
            @Override
            public void onStat(final float dropRate, final int bitrate, final int jitter, final int rtt, final int maxDelay) {
                runOnUiThread(new Runnable() {
                    @SuppressLint("SetTextI18n")
                    @Override
                    public void run() {
                        if (!showDebug) {
                            return;
                        }
                        tvDroprate.setText("dropRate = " + dropRate + "%");
                        tvBitrate.setText("bitrate = " + bitrateFormat(bitrate));
                        tvJitter.setText("jitter = " + jitter);
                        tvRtt.setText("rtt = " + rtt);
                        tvMaxdelay.setText("maxDelay = " + maxDelay);
                    }
                });
            }
        });
        if (showDebug && BuildConfig.DEBUG) {
            mDebugView.setVisibility(View.VISIBLE);
        } else {
            mDebugView.setVisibility(View.GONE);
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        try {
            if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {
                mTextureView.resize(true);
                mLlVerContainer.setVisibility(View.VISIBLE);
                mLlLandContainer.setVisibility(View.GONE);
                mRlTitle.setBackgroundColor(getResources().getColor(R.color.select_text_color));
                mPlaceView.setBackgroundColor(getResources().getColor(R.color.select_text_color));
                isVerity = true;
            } else if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                mTextureView.resize(false);
                mLlVerContainer.setVisibility(View.GONE);
                mLlLandContainer.setVisibility(View.VISIBLE);
                mRlTitle.setBackgroundColor(getResources().getColor(R.color.translate));
                mPlaceView.setBackgroundColor(getResources().getColor(R.color.translate));
                isVerity = false;
            }
            if (mIsVideoValid) {
                Bitmap bitmap = productBitmapBySetting();
                mTextureView.setBitmap(bitmap);
                NewPublisher.getInstance().setBitmap(bitmap);
            } else {
                Bitmap bitmapByColor = ImageExt.getBitmapByColor("#00000000");
                mTextureView.setBitmap(bitmapByColor);
                NewPublisher.getInstance().setBitmap(bitmapByColor);
            }
            NewPublisher.getInstance().init(Camera2Activity.this, mTextureView.getEglContext(), mTextureView.getTextureId(), isVerity);

            mTextureView.previewAngle(this);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("owner:onConfiguration", e.getMessage());
            CrashReport.postCatchedException(e);
        }
    }

    public String bitrateFormat(int bitrate) {
        if (bitrate < 1024) {
            return bitrate + "bps";
        } else if (bitrate < 1048576) {
            float temp = bitrate * 1.0f / 1024;
            float b = (float) (Math.round(temp * 10)) / 10;
            return b + "Kbps";
        } else if (bitrate < 1073741824) {
            float temp = bitrate * 1.0f / 1048576;
            float b = (float) (Math.round(temp * 10)) / 10;
            return b + "Mbps";
        } else if (bitrate < 1099511627776L) {
            float temp = bitrate * 1.0f / 1073741824;
            float b = (float) (Math.round(temp * 10)) / 10;
            return b + "Gbps";
        }
        return "";
    }

    @SuppressLint("SourceLockedOrientationActivity")
    private void changeOrientation() {
        if (getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        } else if (getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT) {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }
    }

    @Override
    protected void onDestroy() {
        mTextureView.onDestroy();
        if (isConnect) {
            NewPublisher.getInstance().stopPush();
        }
        NewPublisher.getInstance().setConnect(false);
        NewPublisher.getInstance().close();
        super.onDestroy();
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        try {
            mTextureView.postDelayed(new Runnable() {
                @Override
                public void run() {
                    NewPublisher.getInstance().init(Camera2Activity.this, mTextureView.getEglContext(), mTextureView.getTextureId(), isVerity);
                }
            }, 1000);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("owner:onRestart", e.getMessage());
            CrashReport.postCatchedException(e);
        }
    }

    private void setAudioValid(boolean isValid) {
        //关闭麦克风
        if (mAudioManager != null) {
            mAudioManager.setMicrophoneMute(isValid);
        }
    }

    private Bitmap productBitmapBySetting() {
        if (mSettingConfig == null) {
            return ImageExt.getBitmapByColor("#00000000");
        } else {
            switch (mSettingConfig.coverType) {
                case Constants.TYPE_COVER.BLACK_TYPE:
                    return ImageExt.getBitmapByColor("#000000");
                case Constants.TYPE_COVER.GREY_TYPE:
                    return ImageExt.getBitmapByColor("#808080");
                case Constants.TYPE_COVER.WHITE_TYPE:
                    return ImageExt.getBitmapByColor("#ffffff");
                case Constants.TYPE_COVER.OWN_TYPE:
                    Bitmap bitmap = SPUtils.getBitmap(this, mSettingConfig.base64);
                    if (bitmap == null) {
                        return ImageExt.getBitmapByColor("#00000000");
                    }
                    return SPUtils.initBitmap(bitmap, isVerity);
            }
        }
        return ImageExt.getBitmapByColor("#00000000");
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.ib_switch:
                //转屏
                try {
                    changeOrientation();
                } catch (Exception e) {
                    e.printStackTrace();
                    Log.e("owner:ib_switch", e.getMessage());
                    CrashReport.postCatchedException(e);
                }
                break;
            case R.id.iv_back:
                //返回
                finish();
                break;
            case R.id.tv_setting:
                //进入设置页面
                Intent intent = new Intent(Camera2Activity.this, LiveConfigActivity.class);
                intent.putExtra(Constants.KEY_TITLE, mConfig.liveTitle);
                intent.putExtra(Constants.KEY_LIVE_ID, mConfig.id);
                startActivity(intent);
                finish();
                if (isConnect) {
                    NewPublisher.getInstance().stopPush();
                }
                break;
            case R.id.trans_camera:
                //转换摄像头
                try {
                    Bitmap bitmapByColor1 = ImageExt.getBitmapByColor("#000000");
                    mTextureView.setBitmap(bitmapByColor1);
                    NewPublisher.getInstance().setBitmap(bitmapByColor1);
                    mTextureView.switchCamera();
                    mViewBlack.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            Bitmap bitmapByColor = ImageExt.getBitmapByColor("#00000000");
                            mTextureView.setBitmap(bitmapByColor);
                            NewPublisher.getInstance().setBitmap(bitmapByColor);
                        }
                    }, 800);
                } catch (Exception e) {
                    e.printStackTrace();
                    Log.e("owner:trans_camera", e.getMessage());
                    CrashReport.postCatchedException(e);
                }
                break;
            case R.id.iv_land_image_audio:
            case R.id.iv_image_audio:
                //启动/禁用音频
                if (mIsAudioValid) {
                    mIsAudioValid = false;
                    mIvAudio.setImageResource(R.drawable.audio_image);
                    mIvLandAudio.setImageResource(R.drawable.audio_image);
                } else {
                    mIsAudioValid = true;
                    mIvAudio.setImageResource(R.drawable.no_audio_image);
                    mIvLandAudio.setImageResource(R.drawable.no_audio_image);
                }
                setAudioValid(mIsAudioValid);
                break;
            case R.id.iv_land_image_video:
            case R.id.iv_image_video:
                //启动/禁用视频
                try {
                    if (mIsVideoValid) {
                        mIsVideoValid = false;
                        Bitmap bitmapByColor = ImageExt.getBitmapByColor("#00000000");
                        mTextureView.setBitmap(bitmapByColor);
                        NewPublisher.getInstance().setBitmap(bitmapByColor);
                        mIvVideo.setImageResource(R.drawable.video_image);
                        mIvLandVideo.setImageResource(R.drawable.video_image);
                    } else {
                        mIsVideoValid = true;
                        Bitmap bitmap = productBitmapBySetting();
                        mTextureView.setBitmap(bitmap);
                        NewPublisher.getInstance().setBitmap(bitmap);
                        mIvVideo.setImageResource(R.drawable.no_video_image);
                        mIvLandVideo.setImageResource(R.drawable.no_video_image);
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    Log.e("owner:iv_image_video", e.getMessage());
                    CrashReport.postCatchedException(e);
                }
                break;
            case R.id.btn_land_start:
            case R.id.btn_start:
                //开始/结束推流
                try {
                    if (isCountDowning) return;
                    if (!isConnect) {
                        if (mSettingConfig.countDown != 0) {
                            isCountDowning = true;
                            mTvCountDown.setVisibility(View.VISIBLE);
                            Message msg = mHandler.obtainMessage();
                            msg.what = 1;
                            msg.obj = mSettingConfig.countDown;
                            mHandler.sendMessage(msg);
                        } else {
                            NewPublisher.getInstance().init(Camera2Activity.this, mTextureView.getEglContext(), mTextureView.getTextureId(), isVerity);
                            NewPublisher.getInstance().startPush(new IConnectStateListener() {
                                @Override
                                public void onConnectState(int result) {
                                    if (result == 0) {
                                        mBtnStart.post(new Runnable() {
                                            @Override
                                            public void run() {
                                                isConnect = true;
                                                mBtnStart.setText(getString(R.string.live_finish));
                                                mBtnLandStart.setText(getString(R.string.live_finish));
                                                mBtnStart.setBackgroundColor(Camera2Activity.this.getResources().getColor(R.color.red));
                                                mBtnLandStart.setBackgroundColor(Camera2Activity.this.getResources().getColor(R.color.red));
                                                mTvTime.setBase(SystemClock.elapsedRealtime());//计时器清零
                                                mTvTime.start();
                                                mTvSetting.setEnabled(!isConnect);
                                                mIvBackIcon.setEnabled(!isConnect);
                                                mFlBack.setEnabled(!isConnect);
                                            }
                                        });
                                    } else {
                                        mBtnStart.post(new Runnable() {
                                            @Override
                                            public void run() {
                                                Toast.makeText(Camera2Activity.this, Camera2Activity.this.getString(R.string.no_connect_server), Toast.LENGTH_SHORT).show();
                                            }
                                        });
                                    }
                                }
                            });
                        }
                    } else {
                        showNormalMoreButtonDialog();
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    Log.e("owner:btn_start", e.getMessage());
                    CrashReport.postCatchedException(e);
                }
                break;
        }
    }

    private void showNormalMoreButtonDialog() {
        DialogInterface.OnClickListener setListener = null;

        AlertDialog.Builder normalMoreButtonDialog = new AlertDialog.Builder(this);
        normalMoreButtonDialog.setTitle(getString(R.string.dialog_normal_more_button_text));
        setListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                switch (which) {
                    case DialogInterface.BUTTON_POSITIVE:
                        dialog.dismiss();
                        isConnect = false;
                        mBtnStart.setText(getString(R.string.live_ready));
                        mBtnLandStart.setText(getString(R.string.live_ready));
                        mBtnStart.setBackgroundColor(getResources().getColor(R.color.select_text_color));
                        mBtnLandStart.setBackgroundColor(getResources().getColor(R.color.select_text_color));
                        NewPublisher.getInstance().stopPush();
                        mTvTime.stop();
                        mTvTime.setBase(SystemClock.elapsedRealtime());//计时器清零
                        mTvSetting.setEnabled(!isConnect);
                        mIvBackIcon.setEnabled(!isConnect);
                        mFlBack.setEnabled(!isConnect);
                        break;
                    case DialogInterface.BUTTON_NEGATIVE:
                        dialog.dismiss();
                        break;
                }
            }
        };
        normalMoreButtonDialog.setPositiveButton(getString(R.string.dialog_btn_confirm_hint_text), setListener);
        normalMoreButtonDialog.setNegativeButton(getString(R.string.dialog_btn_cancel_hint_text), setListener);
        normalMoreButtonDialog.create().show();
    }
}
