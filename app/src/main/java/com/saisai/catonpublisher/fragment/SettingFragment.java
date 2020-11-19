package com.saisai.catonpublisher.fragment;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.media.ExifInterface;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.Base64;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
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
import com.saisai.catonpublisher.activity.LiveConfigActivity;
import com.saisai.catonpublisher.newencode.SettingConfig;
import com.saisai.catonpublisher.util.GlideImageLoader;
import com.saisai.catonpublisher.util.SPUtils;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

public class SettingFragment extends Fragment implements View.OnClickListener {

    private EditText mEditCaptions;
    private RadioGroup mRgCountDown;
    private RadioGroup mRgCover;
    private RadioGroup mRgShowTime;
    private RadioGroup mRgShowCaptions;
    private SettingConfig mSettingConfig;
    private ImageView mSelectImage;

    private ImagePicker imagePicker;
    ArrayList<ImageItem> images = null;
    private String base64;
    private int degree;

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_setting, container, false);
        mEditCaptions = view.findViewById(R.id.edit_danmu);
        mRgCountDown = view.findViewById(R.id.rg_count_down);
        mRgCover = view.findViewById(R.id.rg_cover);
        mRgShowTime = view.findViewById(R.id.rg_show_time);
        mRgShowCaptions = view.findViewById(R.id.rg_show_danmu);
        mSelectImage = view.findViewById(R.id.select_image);
        mSelectImage.setOnClickListener(this);
        view.findViewById(R.id.btn_store).setOnClickListener(this);
        imagePicker = ImagePicker.getInstance();
        imagePicker.setImageLoader(new GlideImageLoader());
        return view;
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mSettingConfig = SPUtils.getSettingConfig();
        if (mSettingConfig != null) {
            switch (mSettingConfig.countDown) {
                case 0:
                    mRgCountDown.check(R.id.rb_0s);
                    break;
                case 3:
                    mRgCountDown.check(R.id.rb_3s);
                    break;
                case 5:
                    mRgCountDown.check(R.id.rb_5s);
                    break;
                case 10:
                    mRgCountDown.check(R.id.rb_10s);
                    break;
            }
            switch (mSettingConfig.coverType) {
                case Constants.TYPE_COVER.BLACK_TYPE:
                    mRgCover.check(R.id.rb_cover_black);
                    break;
                case Constants.TYPE_COVER.GREY_TYPE:
                    mRgCover.check(R.id.rb_cover_gray);
                    break;
                case Constants.TYPE_COVER.WHITE_TYPE:
                    mRgCover.check(R.id.rb_cover_white);
                    break;
                case Constants.TYPE_COVER.OWN_TYPE:
                    mRgCover.check(R.id.rb_cover_own);
                    mSelectImage.setVisibility(View.VISIBLE);
                    Bitmap bitmap = SPUtils.getBitmap(getContext(), mSettingConfig.base64);
                    if (bitmap != null) {
                        mSelectImage.setImageBitmap(bitmap);
                    }
                    break;
            }
            mRgShowTime.check(mSettingConfig.isShowTime ? R.id.rb_show_time_yes : R.id.rb_show_time_no);
            mRgShowCaptions.check(mSettingConfig.isShowDanmu ? R.id.rb_show_danmu_yes : R.id.rb_show_danmu_no);
            mEditCaptions.setText(mSettingConfig.danmuText);
        }
        mRgCover.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if (checkedId == R.id.rb_cover_own) {
                    mSelectImage.setVisibility(View.VISIBLE);
                } else {
                    mSelectImage.setVisibility(View.GONE);
                }
            }
        });
    }

    /**
     * 选择图片
     */
    private void selectImage() {
        imagePicker.setMultiMode(false);
        imagePicker.setStyle(CropImageView.Style.RECTANGLE);
        imagePicker.setCrop(false);
        Intent intent = new Intent(getActivity(), ImageGridActivity.class);
        intent.putExtra(ImageGridActivity.EXTRAS_IMAGES, images);
        startActivityForResult(intent, 100);
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

    private void storeSetting() {
        switch (getCheckId(mRgCountDown)) {
            case R.id.rb_0s:
                mSettingConfig.countDown = 0;
                break;
            case R.id.rb_3s:
                mSettingConfig.countDown = 3;
                break;
            case R.id.rb_5s:
                mSettingConfig.countDown = 5;
                break;
            case R.id.rb_10s:
                mSettingConfig.countDown = 10;
                break;
        }
        switch (getCheckId(mRgCover)) {
            case R.id.rb_cover_black:
                mSettingConfig.coverType = Constants.TYPE_COVER.BLACK_TYPE;
                break;
            case R.id.rb_cover_gray:
                mSettingConfig.coverType = Constants.TYPE_COVER.GREY_TYPE;
                break;
            case R.id.rb_cover_white:
                mSettingConfig.coverType = Constants.TYPE_COVER.WHITE_TYPE;
                break;
            case R.id.rb_cover_own:
                mSettingConfig.coverType = Constants.TYPE_COVER.OWN_TYPE;
                break;
        }
        mSettingConfig.isShowTime = getCheckId(mRgShowTime) == R.id.rb_show_time_yes;
        mSettingConfig.isShowDanmu = getCheckId(mRgShowCaptions) == R.id.rb_show_danmu_yes;
        mSettingConfig.danmuText = mEditCaptions.getText().toString();
        mSettingConfig.base64 = base64;
        SPUtils.saveSetting(mSettingConfig);
    }

    @SuppressLint("ShowToast")
    @Override
    public void onClick(View v) {
        if (v.getId() == R.id.btn_store) {
            storeSetting();
            Toast.makeText(getContext(), "保存成功", Toast.LENGTH_LONG).show();
        } else if (v.getId() == R.id.select_image) {
            //选择图片
            selectImage();
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == ImagePicker.RESULT_CODE_ITEMS) {
            if (data != null && requestCode == 100) {
                images = (ArrayList<ImageItem>) data.getSerializableExtra(ImagePicker.EXTRA_RESULT_ITEMS);
                File file = new File(images.get(0).path);
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
                    Bitmap bitmap = BitmapFactory.decodeFile(images.get(0).path, options);
                    Matrix matrix = new Matrix();
                    matrix.postRotate(degree);
                    bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
                    ByteArrayOutputStream baos = new ByteArrayOutputStream();
                    bitmap.compress(Bitmap.CompressFormat.PNG, 100, baos);
                    mSelectImage.setImageBitmap(bitmap);
                    byte[] bytes = baos.toByteArray();
                    base64 = new String(Base64.encode(bytes, Base64.DEFAULT));
                }
            } else {
                Toast.makeText(getContext(), "没有数据", Toast.LENGTH_SHORT).show();
            }
        }
    }
}
