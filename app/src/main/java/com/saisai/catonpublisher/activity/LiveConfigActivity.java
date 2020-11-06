package com.saisai.catonpublisher.activity;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.ExifInterface;
import android.media.MediaFormat;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;
import android.util.Range;
import android.util.Size;
import android.util.TypedValue;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import com.lzy.imagepicker.ImagePicker;
import com.lzy.imagepicker.bean.ImageItem;
import com.lzy.imagepicker.ui.ImageGridActivity;
import com.lzy.imagepicker.view.CropImageView;
import com.saisai.catonpublisher.Constants;
import com.saisai.catonpublisher.R;
import com.saisai.catonpublisher.newencode.NewPublisherConfig;
import com.saisai.catonpublisher.util.GlideImageLoader;
import com.saisai.catonpublisher.util.SPUtils;
import com.saisai.catonpublisher.util.StateBarTranslucentUtils;
import com.saisai.catonpublisher.util.StatusBarCompat;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import de.hdodenhof.circleimageview.CircleImageView;

public class LiveConfigActivity extends AppCompatActivity implements View.OnClickListener {

    private ImageView mIvBack;
    private TextView mTvTitle;
    private EditText mEditLiveName;
    private EditText mEditLiveNet;
    private EditText mEditLivePort;
    private EditText mEditLiveMbps;
    private EditText mEditLiveSecretKey;
    private EditText mEditLiveDesc;
    private CircleImageView mSelectImage;
    private CheckBox mCbDebug;
    private RadioGroup mRgVideoCodec;
    private RadioGroup mRgResolution;
    private RadioGroup mRgFps;
    private RadioGroup mRgAudioBit;
    private TextView mTvBack;
    private TextView mTvAdd;

    private ImagePicker imagePicker;
    ArrayList<ImageItem> images = null;
    private String base64;
    private Bitmap mBitmap;
    private int degree;
    private File file = null;


    private List<CameraCharacteristics> mCharacteristics;
    private Map<Integer, Map> mSupportResolution;
    private Map<Integer, Map> mSupportFPS;
    private NewPublisherConfig mCurConfig;
    private List<NewPublisherConfig> mConfigList;
    private int mCurPos = -1;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        StateBarTranslucentUtils.setStateBarTranslucent(this);
        setContentView(R.layout.activity_config);
        StatusBarCompat.compat(this);
        mIvBack = findViewById(R.id.iv_back);
        mTvTitle = findViewById(R.id.tv_title);
        mEditLiveName = findViewById(R.id.edit_live_name);
        mEditLiveNet = findViewById(R.id.edit_live_net);
        mEditLivePort = findViewById(R.id.edit_live_port);
        mEditLiveMbps = findViewById(R.id.edit_live_mbps);
        mEditLiveSecretKey = findViewById(R.id.edit_live_secret_key);
        mEditLiveDesc = findViewById(R.id.edit_live_desc);
        mSelectImage = findViewById(R.id.select_image);
        mCbDebug = findViewById(R.id.cb_debug);
        mRgVideoCodec = findViewById(R.id.rg_videocodec);
        mRgResolution = findViewById(R.id.rg_resolution);
        mRgFps = findViewById(R.id.rg_video_framerate);
        mRgAudioBit = findViewById(R.id.rg_audio_bitrate);
        mTvBack = findViewById(R.id.btn_back);
        mTvAdd = findViewById(R.id.btn_store);
        mIvBack.setOnClickListener(this);
        mTvBack.setOnClickListener(this);
        mTvAdd.setOnClickListener(this);
        mSelectImage.setOnClickListener(this);
        initView();
    }

    private void initView() {
        imagePicker = ImagePicker.getInstance();
        imagePicker.setImageLoader(new GlideImageLoader());
        try {
            mCharacteristics = getCameraCharacteristicsList();
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        initSupportParameters();
        if (getIntent().getExtras() != null) {
            String title = getIntent().getExtras().getString(Constants.KEY_TITLE);
            long id = getIntent().getExtras().getLong(Constants.KEY_LIVE_ID, -1L);
            //获取存储的配置信息
            mConfigList = SPUtils.getLiveData();
            if (id != -1) {
                //当前是修改信息
                if (mConfigList != null && mConfigList.size() > 0) {
                    for (int i = 0; i < mConfigList.size(); i++) {
                        NewPublisherConfig config = mConfigList.get(i);
                        if (config.id == id) {
                            mCurConfig = config;
                            mCurPos = i;
                            setData(mCurConfig);
                        }
                    }
                }
            } else {
                mTvTitle.setText(title);
                resetUI();
            }

        }
    }

    /**
     * 设置数据
     */
    private void setData(NewPublisherConfig config) {
        Map<Integer, Size> resolution = mSupportResolution.get(CameraCharacteristics.LENS_FACING_BACK);
        findViewById(R.id.rb_2160p).setEnabled(resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_2160P) != null);
        findViewById(R.id.rb_1080p).setEnabled(resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_1080P) != null);
        findViewById(R.id.rb_720p).setEnabled(resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_720P) != null);
        findViewById(R.id.rb_480p).setEnabled(resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_480P) != null);

        Map<Integer, Range<Integer>> fps = mSupportFPS.get(CameraCharacteristics.LENS_FACING_BACK);
        findViewById(R.id.rb_video_framerate_15).setEnabled(fps.get(NewPublisherConfig.VIDEO_FPS_15) != null);
        findViewById(R.id.rb_video_framerate_24).setEnabled(fps.get(NewPublisherConfig.VIDEO_FPS_24) != null);
        findViewById(R.id.rb_video_framerate_30).setEnabled(fps.get(NewPublisherConfig.VIDEO_FPS_30) != null);
        findViewById(R.id.rb_video_framerate_50).setEnabled(fps.get(NewPublisherConfig.VIDEO_FPS_50) != null);
        mTvTitle.setText(config.liveTitle);
        mEditLiveName.setText(config.liveTitle);
        mEditLiveName.setSelection(config.liveTitle.length());
        mEditLiveNet.setText(config.mHost);
        mEditLivePort.setText(String.valueOf(config.mPort));
        mEditLiveDesc.setText(config.liveDesc);
        mEditLiveSecretKey.setText(config.mKey);
        mEditLiveMbps.setText(String.valueOf(config.mVideoBitrate));
        mCbDebug.setChecked(config.showDebug);
        if (TextUtils.isEmpty(config.imageSrc)) {
            mSelectImage.setImageResource(R.drawable.camera);
        } else {
            Bitmap bitmap = SPUtils.getBitmap(this, config.imageSrc);
            if (bitmap != null) {
                mSelectImage.setImageBitmap(bitmap);
            } else {
                mSelectImage.setImageResource(R.drawable.camera);
            }
        }
        switch (config.mVideoMimeType) {
            case MediaFormat.MIMETYPE_VIDEO_AVC:
                mRgVideoCodec.check(R.id.rb_avc);
                break;
            case MediaFormat.MIMETYPE_VIDEO_HEVC:
                mRgVideoCodec.check(R.id.rb_hevc);
                break;
        }
        switch (config.mAudioBitrate) {
            case NewPublisherConfig.AUDIO_BITRATE_64:
                mRgAudioBit.check(R.id.rb_audio_bitrate_64);
                break;
            case NewPublisherConfig.AUDIO_BITRATE_128:
                mRgAudioBit.check(R.id.rb_audio_bitrate_128);
                break;
        }
        switch (config.mVideoFramerate) {
            case NewPublisherConfig.VIDEO_FPS_15:
                mRgFps.check(R.id.rb_video_framerate_15);
                break;
            case NewPublisherConfig.VIDEO_FPS_24:
                mRgFps.check(R.id.rb_video_framerate_24);
                break;
            case NewPublisherConfig.VIDEO_FPS_30:
                mRgFps.check(R.id.rb_video_framerate_30);
                break;
            case NewPublisherConfig.VIDEO_FPS_50:
                mRgFps.check(R.id.rb_video_framerate_50);
                break;
        }
        switch (config.mVideoResolution) {
            case NewPublisherConfig.VIDEO_RESOLUTION_480P:
                mRgResolution.check(R.id.rb_480p);
                break;
            case NewPublisherConfig.VIDEO_RESOLUTION_720P:
                mRgResolution.check(R.id.rb_720p);
                break;
            case NewPublisherConfig.VIDEO_RESOLUTION_1080P:
                mRgResolution.check(R.id.rb_1080p);
                break;
            case NewPublisherConfig.VIDEO_RESOLUTION_2160P:
                mRgResolution.check(R.id.rb_2160p);
                break;
        }
    }


    /**
     * 选择图片
     */
    private void selectImage() {
        imagePicker.setMultiMode(false);
        imagePicker.setStyle(CropImageView.Style.CIRCLE);
        int width = 300;
        int height = 300;
        width = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, width, getResources().getDisplayMetrics());
        height = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, height, getResources().getDisplayMetrics());
        imagePicker.setFocusWidth(width);
        imagePicker.setFocusHeight(height);
        imagePicker.setOutPutX(800);
        imagePicker.setOutPutY(800);
        Intent intent = new Intent(LiveConfigActivity.this, ImageGridActivity.class);
        intent.putExtra(ImageGridActivity.EXTRAS_IMAGES, images);
        startActivityForResult(intent, 100);
    }

    //重置UI信息
    private void resetUI() {
        Map<Integer, Size> resolution = mSupportResolution.get(CameraCharacteristics.LENS_FACING_BACK);
        findViewById(R.id.rb_2160p).setEnabled(resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_2160P) != null);
        findViewById(R.id.rb_1080p).setEnabled(resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_1080P) != null);
        findViewById(R.id.rb_720p).setEnabled(resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_720P) != null);
        findViewById(R.id.rb_480p).setEnabled(resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_480P) != null);

        Map<Integer, Range<Integer>> fps = mSupportFPS.get(CameraCharacteristics.LENS_FACING_BACK);
        findViewById(R.id.rb_video_framerate_15).setEnabled(fps.get(NewPublisherConfig.VIDEO_FPS_15) != null);
        findViewById(R.id.rb_video_framerate_24).setEnabled(fps.get(NewPublisherConfig.VIDEO_FPS_24) != null);
        findViewById(R.id.rb_video_framerate_30).setEnabled(fps.get(NewPublisherConfig.VIDEO_FPS_30) != null);
        findViewById(R.id.rb_video_framerate_50).setEnabled(fps.get(NewPublisherConfig.VIDEO_FPS_50) != null);
        defaultResolutionSelect();
        defaultFPSSelect();
    }

    /**
     * 默认选择480p
     */
    private void defaultResolutionSelect() {
        int checkId = getCheckId(mRgResolution);
        if (findViewById(checkId).isEnabled()) {
            return;
        }
        if (findViewById(R.id.rb_480p).isEnabled()) {
            ((RadioButton) findViewById(R.id.rb_480p)).setChecked(findViewById(R.id.rb_480p).isEnabled());
            return;
        }
        if (findViewById(R.id.rb_720p).isEnabled()) {
            ((RadioButton) findViewById(R.id.rb_720p)).setChecked(findViewById(R.id.rb_720p).isEnabled());
            return;
        }
        if (findViewById(R.id.rb_1080p).isEnabled()) {
            ((RadioButton) findViewById(R.id.rb_1080p)).setChecked(findViewById(R.id.rb_1080p).isEnabled());
            return;
        }
        if (findViewById(R.id.rb_2160p).isEnabled()) {
            ((RadioButton) findViewById(R.id.rb_2160p)).setChecked(findViewById(R.id.rb_2160p).isEnabled());
        }
    }

    /**
     * 默认选择15帧
     */
    private void defaultFPSSelect() {
        int checkId = getCheckId(mRgFps);
        if (findViewById(checkId).isEnabled()) {
            return;
        }
        if (findViewById(R.id.rb_video_framerate_15).isEnabled()) {
            ((RadioButton) findViewById(R.id.rb_video_framerate_15))
                    .setChecked(findViewById(R.id.rb_video_framerate_15).isEnabled());
            return;
        }
        if (findViewById(R.id.rb_video_framerate_24).isEnabled()) {
            ((RadioButton) findViewById(R.id.rb_video_framerate_24))
                    .setChecked(findViewById(R.id.rb_video_framerate_24).isEnabled());
            return;
        }
        if (findViewById(R.id.rb_video_framerate_30).isEnabled()) {
            ((RadioButton) findViewById(R.id.rb_video_framerate_30))
                    .setChecked(findViewById(R.id.rb_video_framerate_30).isEnabled());
        }
        if (findViewById(R.id.rb_video_framerate_50).isEnabled()) {
            ((RadioButton) findViewById(R.id.rb_video_framerate_50))
                    .setChecked(findViewById(R.id.rb_video_framerate_50).isEnabled());
        }
    }

    /**
     * 检查数据是否可保存
     *
     * @return 数据是否完整
     */
    private boolean checkDataValid(String liveName, String liveNet, String livePort, String liveMbps) {
        //1.必填数据是否填写完整
        if (TextUtils.isEmpty(liveName)) {
            Toast.makeText(this, getResources().getString(R.string.text_notice_title_empty)
                    , Toast.LENGTH_SHORT).show();
            return false;
        }

        if (TextUtils.isEmpty(liveNet)) {
            Toast.makeText(this, getResources().getString(R.string.text_notice_net_empty)
                    , Toast.LENGTH_SHORT).show();
            return false;
        }

        String[] split = liveNet.split("\\.");
        if (split.length != 4) {
            Toast.makeText(LiveConfigActivity.this, "url format error", Toast.LENGTH_SHORT).show();
            return false;
        }

        if (TextUtils.isEmpty(livePort)) {
            Toast.makeText(this, getResources().getString(R.string.text_notice_port_empty)
                    , Toast.LENGTH_SHORT).show();
            return false;
        }
        if (TextUtils.isEmpty(liveMbps)) {
            Toast.makeText(this, getResources().getString(R.string.text_notice_mbps_empty)
                    , Toast.LENGTH_SHORT).show();
            return false;
        }
        return true;
    }

    /**
     * 初始化一些相机的参数
     */
    private void initSupportParameters() {
        if (mCharacteristics == null) {
            return;
        }

        mSupportResolution = new HashMap<>();
        mSupportFPS = new HashMap<Integer, Map>();

        for (CameraCharacteristics characteristics : mCharacteristics) {

            Map<Integer, Range<Integer>> fps;
            fps = mSupportFPS.get(characteristics.get(CameraCharacteristics.LENS_FACING));
            if (fps == null) {
                fps = new HashMap<>();
            }

            //获取camera2的功能支持情况
            Integer integer = characteristics.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL);
            Log.e("tag", "HARDWARE_LEVEL = " + integer);

            //获取fps的范围
            Range<Integer>[] ranges = characteristics.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);

            if (fps.get(NewPublisherConfig.VIDEO_FPS_15) == null) {
                fps.put(NewPublisherConfig.VIDEO_FPS_15,
                        new Range<>(15, 15));
            }
            if (fps.get(NewPublisherConfig.VIDEO_FPS_24) == null) {
                fps.put(NewPublisherConfig.VIDEO_FPS_24,
                        new Range<>(25, 25));
            }
            if (fps.get(NewPublisherConfig.VIDEO_FPS_30) == null) {
                fps.put(NewPublisherConfig.VIDEO_FPS_30,
                        new Range<>(30, 30));
            }
            if (fps.get(NewPublisherConfig.VIDEO_FPS_50) == null) {
                fps.put(NewPublisherConfig.VIDEO_FPS_50,
                        new Range<>(50, 50));
            }
            mSupportFPS.put(characteristics.get(CameraCharacteristics.LENS_FACING), fps);

            //获取摄像头方向（前置还是后置）
            Map<Integer, Size> resolution;
            resolution = mSupportResolution.get(characteristics.get(CameraCharacteristics.LENS_FACING));
            if (resolution == null) {
                resolution = new HashMap<>();
            }

            //获取StreamConfigurationMap，它是管理摄像头支持的所有输出格式和尺寸
            StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map != null) {
                Size[] sizes = map.getOutputSizes(ImageFormat.YUV_420_888);

                if (resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_480P) == null) {
                    resolution.put(NewPublisherConfig.VIDEO_RESOLUTION_480P,
                            NewPublisherConfig.getResolution(NewPublisherConfig.VIDEO_RESOLUTION_480P, sizes));
                }
                if (resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_720P) == null) {
                    resolution.put(NewPublisherConfig.VIDEO_RESOLUTION_720P,
                            NewPublisherConfig.getResolution(NewPublisherConfig.VIDEO_RESOLUTION_720P, sizes));
                }
                if (resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_1080P) == null) {
                    resolution.put(NewPublisherConfig.VIDEO_RESOLUTION_1080P,
                            NewPublisherConfig.getResolution(NewPublisherConfig.VIDEO_RESOLUTION_1080P, sizes));
                }
                if (resolution.get(NewPublisherConfig.VIDEO_RESOLUTION_2160P) == null) {
                    resolution.put(NewPublisherConfig.VIDEO_RESOLUTION_2160P,
                            NewPublisherConfig.getResolution(NewPublisherConfig.VIDEO_RESOLUTION_2160P, sizes));
                }
                mSupportResolution.put(characteristics.get(CameraCharacteristics.LENS_FACING), resolution);

            }
        }
        Log.e("tag", "support video resolution = " + mSupportResolution);
    }

    /**
     * 获取相机配置
     *
     * @return
     * @throws CameraAccessException
     */
    private List<CameraCharacteristics> getCameraCharacteristicsList() throws CameraAccessException {
        List<CameraCharacteristics> list = new ArrayList<>();
        CameraManager manager = (CameraManager) this.getSystemService(Context.CAMERA_SERVICE);
        if (manager != null) {
            String[] cameraIdList = manager.getCameraIdList();
            for (String id : cameraIdList) {
                CameraCharacteristics characteristics =
                        manager.getCameraCharacteristics(id);
                list.add(characteristics);
            }

            Log.e("tag", "camera num = " + list.size());
        }
        return list;
    }

    /**
     * 存储配置信息
     */
    private void storeCurrentConfig() {
        String liveName = mEditLiveName.getText().toString();
        String liveNet = mEditLiveNet.getText().toString();
        String livePort = mEditLivePort.getText().toString();
        String liveMbps = mEditLiveMbps.getText().toString();
        String secretKey = mEditLiveSecretKey.getText().toString();
        String liveDesc = mEditLiveDesc.getText().toString();
        String switchVideoCodec = MediaFormat.MIMETYPE_VIDEO_AVC;
        int videoCodecId = getCheckId(mRgVideoCodec);
        if (videoCodecId == R.id.rb_avc) {
            switchVideoCodec = MediaFormat.MIMETYPE_VIDEO_AVC;
        } else if (videoCodecId == R.id.rb_hevc) {
            switchVideoCodec = MediaFormat.MIMETYPE_VIDEO_HEVC;
        }

        int switchResolution = -1;
        int resolutionId = getCheckId(mRgResolution);
        if (resolutionId == R.id.rb_480p) {
            switchResolution = NewPublisherConfig.VIDEO_RESOLUTION_480P;
        } else if (resolutionId == R.id.rb_720p) {
            switchResolution = NewPublisherConfig.VIDEO_RESOLUTION_720P;
        } else if (resolutionId == R.id.rb_1080p) {
            switchResolution = NewPublisherConfig.VIDEO_RESOLUTION_1080P;
        } else if (resolutionId == R.id.rb_2160p) {
            switchResolution = NewPublisherConfig.VIDEO_RESOLUTION_2160P;
        }

        int switchVideoFrameRate = -1;
        int videoFrameRate = getCheckId(mRgFps);

        if (videoFrameRate == R.id.rb_video_framerate_15) {
            switchVideoFrameRate = NewPublisherConfig.VIDEO_FPS_15;
        } else if (videoFrameRate == R.id.rb_video_framerate_24) {
            switchVideoFrameRate = NewPublisherConfig.VIDEO_FPS_24;
        } else if (videoFrameRate == R.id.rb_video_framerate_30) {
            switchVideoFrameRate = NewPublisherConfig.VIDEO_FPS_30;
        } else if (videoFrameRate == R.id.rb_video_framerate_50) {
            switchVideoFrameRate = NewPublisherConfig.VIDEO_FPS_50;
        }

        int switchAudioBitrate = -1;
        int audioBitrate = getCheckId(mRgAudioBit);
        if (audioBitrate == R.id.rb_audio_bitrate_64) {
            switchAudioBitrate = NewPublisherConfig.AUDIO_BITRATE_64;
        } else if (audioBitrate == R.id.rb_audio_bitrate_128) {
            switchAudioBitrate = NewPublisherConfig.AUDIO_BITRATE_128;
        }

        if (checkDataValid(liveName, liveNet, livePort, liveMbps)) {
            float mbps = Float.parseFloat(liveMbps);
            if (mbps * 1024 * 1024 < NewPublisherConfig.MIN_VIDEO_BITRATE || mbps * 1024 * 1024 > NewPublisherConfig.MAX_VIDEO_BITRATE) {
                Toast.makeText(LiveConfigActivity.this, "Video bitrate size between 500Kbps and 20Mbps!", Toast.LENGTH_SHORT).show();
                return;
            }
            int videoMbps = (int) mbps;
            NewPublisherConfig config = new NewPublisherConfig();
            if (mCurConfig != null) {
                config.id = mCurConfig.id;
            } else {
                config.id = System.currentTimeMillis();
            }
            config.liveTitle = liveName;
            config.mHost = liveNet;
            config.mPort = Integer.parseInt(livePort);
            config.mKey = secretKey;
            config.liveDesc = liveDesc;
            config.showDebug = mCbDebug.isChecked();
            config.mSwitchCameraFacing = NewPublisherConfig.CAMERA2_FACING_BACK;
            config.mSupportResolution = mSupportResolution;
            config.mSupportFps = mSupportFPS;
            config.mVideoMimeType = switchVideoCodec;
            config.mVideoResolution = switchResolution;
            config.mVideoBitrate = videoMbps;
            config.mVideoFramerate = switchVideoFrameRate;
            config.mAudioMimeType = MediaFormat.MIMETYPE_AUDIO_AAC;
            config.mAudioBitrate = switchAudioBitrate;
            config.mAudioSamplerate = NewPublisherConfig.AUDIO_RATE_HZ;
            config.mAudioResource = MediaRecorder.AudioSource.MIC;
            config.imageSrc = base64;
            if (mConfigList == null) {
                mConfigList = new ArrayList<NewPublisherConfig>();
                mConfigList.add(config);
            } else {
                if (mCurPos != -1) {
                    mConfigList.set(mCurPos, config);
                } else {
                    mConfigList.add(config);
                }
            }
            SPUtils.saveLiveData(mConfigList);
            Toast.makeText(this, R.string.text_store_success, Toast.LENGTH_SHORT).show();
            Intent intent = new Intent(LiveConfigActivity.this, Camera2Activity.class);
            intent.putExtra(Constants.KEY_TITLE, config.liveTitle);
            intent.putExtra(Constants.KEY_LIVE_ID, config.id);
            startActivity(intent);
            finish();
        }

    }

    private int getCheckId(RadioGroup rg) {
        int count = rg.getChildCount();
        for (int i = 0; i < count; i++) {
            RadioButton childAt = (RadioButton) rg.getChildAt(i);
            if (childAt.isChecked()) {
                return childAt.getId();
            }
        }
        return -1;
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.iv_back:
            case R.id.btn_back:
                //返回上一级
                finish();
                break;
            case R.id.btn_store:
                //保存配置信息
                storeCurrentConfig();
                break;
            case R.id.select_image:
                //选择图片
                selectImage();
                break;

        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == ImagePicker.RESULT_CODE_ITEMS) {
            if (data != null && requestCode == 100) {
                images = (ArrayList<ImageItem>) data.getSerializableExtra(ImagePicker.EXTRA_RESULT_ITEMS);
                file = new File(images.get(0).path);
                // 从指定路径下读取图片，并获取其EXIF信息
                ExifInterface exifInterface = null;
                try {
                    exifInterface = new ExifInterface(images.get(0).path);
                    // 获取图片的旋转信息
                    int orientation = exifInterface.getAttributeInt(ExifInterface.TAG_ORIENTATION,
                            ExifInterface.ORIENTATION_NORMAL);
                    switch (orientation) {
                        case ExifInterface.ORIENTATION_ROTATE_90:
                            degree = 90;
                            break;
                        case ExifInterface.ORIENTATION_ROTATE_180:
                            degree = 180;
                            break;
                        case ExifInterface.ORIENTATION_ROTATE_270:
                            degree = 270;
                            break;
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
                if (file.exists()) {
                    BitmapFactory.Options options = new BitmapFactory.Options();
                    mBitmap = BitmapFactory.decodeFile(images.get(0).path, options);
                    Matrix matrix = new Matrix();
                    matrix.postRotate(degree);
                    mBitmap = Bitmap.createBitmap(mBitmap, 0, 0, mBitmap.getWidth(), mBitmap.getHeight(), matrix, true);
                    ByteArrayOutputStream baos = new ByteArrayOutputStream();
                    mBitmap.compress(Bitmap.CompressFormat.PNG, 100, baos);
                    mSelectImage.setImageBitmap(mBitmap);
                    byte[] bytes = baos.toByteArray();
                    base64 = new String(Base64.encode(bytes, Base64.DEFAULT));
                }
            } else {
                Toast.makeText(this, "没有数据", Toast.LENGTH_SHORT).show();
            }
        }
    }

}
