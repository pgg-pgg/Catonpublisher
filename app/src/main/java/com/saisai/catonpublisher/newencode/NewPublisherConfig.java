package com.saisai.catonpublisher.newencode;

import android.hardware.camera2.CameraCharacteristics;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.Range;
import android.util.Size;

import com.alibaba.fastjson.JSONObject;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Created by saisai on 2019/3/12 0012.
 */

public class NewPublisherConfig implements Parcelable {

    /* Camera2 id */
    public static final int CAMERA2_FACING_FRONT = CameraCharacteristics.LENS_FACING_FRONT;
    public static final int CAMERA2_FACING_BACK = CameraCharacteristics.LENS_FACING_BACK;

    /* Video Resolution */
    public static final int VIDEO_RESOLUTION_480P = 0;
    public static final int VIDEO_RESOLUTION_720P = 1;
    public static final int VIDEO_RESOLUTION_1080P = 2;
    public static final int VIDEO_RESOLUTION_2160P = 3;

    /* Video FPS */
    public static final int VIDEO_FPS_15 = 15;
    public static final int VIDEO_FPS_24 = 25;
    public static final int VIDEO_FPS_30 = 30;
    public static final int VIDEO_FPS_50 = 50;

    public static final int MIN_VIDEO_BITRATE = 500 * 1024;//bps
    public static final int MAX_VIDEO_BITRATE = 20 * 1024 * 1024;//bps

    public static final int AUDIO_BITRATE_64 = 64000;
    public static final int AUDIO_BITRATE_128 = 128000;
    public static final int AUDIO_RATE_HZ = 44100;

    public long id;

    /* RRS parameters */
    public String mHost;
    public int mPort;
    public String mKey;

    /* Switch camera facing */
    public int mSwitchCameraFacing;

    /* Camera2 support preview size */
    public Map<Integer, Map> mSupportResolution;

    /* Camera2 support fps */
    public Map<Integer, Map> mSupportFps;

    /* Video parameters */
    public String mVideoMimeType;
    public int mVideoResolution;
    public int mVideoBitrate;
    public int mVideoFramerate;

    /* Audio parameters */
    public String mAudioMimeType;
    public int mAudioBitrate;
    public int mAudioSamplerate;
    public int mAudioResource;


    /* show debug msg */
    public boolean showDebug;

    public String liveTitle;

    public String liveDesc;

    public String imageSrc;
    public NewPublisherConfig() {

    }

    public static Size getResolution(int videoResolution, Size[] sizes) {

        int height = 0;
        switch (videoResolution) {
            case VIDEO_RESOLUTION_480P:
                height = 480;
                break;
            case VIDEO_RESOLUTION_720P:
                height = 720;
                break;
            case VIDEO_RESOLUTION_1080P:
                height = 1080;
                break;
            case VIDEO_RESOLUTION_2160P:
                height = 2160;
                break;
        }

        List<Size> list = new ArrayList<>();
        List<Integer> difference = new ArrayList<>();
        int defaultWidth = height * 16 / 9;

        for (Size size : sizes) {
            if (size.getHeight() == height) {
                list.add(size);
                int dif = size.getWidth() - defaultWidth;
                difference.add(Math.abs(dif));
            }
        }
        if (list.size() == 0) {
            return null;
        }

        if (list.size() == 1) {
            return list.get(0);
        }


        int index = -1;
        for (int i = 0; i < difference.size(); i++) {
            if (i == 0) {
                index = i;
                continue;
            } else {
                if (difference.get(index) >= difference.get(i)) {
                    index = i;
                    continue;
                }
            }
        }

        return list.get(index);
    }

    public static Range<Integer> getFps(int videoFps, Range<Integer>[] ranges) {

        for (Range<Integer> range : ranges) {
            if (range.getLower() == range.getUpper() && range.getLower() == videoFps) {
                return range;
            }
        }
        return null;
    }


    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeLong(this.id);
        dest.writeString(this.mHost);
        dest.writeInt(this.mPort);
        dest.writeString(this.mKey);
        dest.writeInt(this.mSwitchCameraFacing);
        dest.writeInt(this.mSupportResolution.size());
        for (Map.Entry<Integer, Map> entry : this.mSupportResolution.entrySet()) {
            dest.writeValue(entry.getKey());
            dest.writeParcelable((Parcelable) entry.getValue(), flags);
        }
        dest.writeInt(this.mSupportFps.size());
        for (Map.Entry<Integer, Map> entry : this.mSupportFps.entrySet()) {
            dest.writeValue(entry.getKey());
            dest.writeParcelable((Parcelable) entry.getValue(), flags);
        }
        dest.writeString(this.mVideoMimeType);
        dest.writeInt(this.mVideoResolution);
        dest.writeInt(this.mVideoBitrate);
        dest.writeInt(this.mVideoFramerate);
        dest.writeString(this.mAudioMimeType);
        dest.writeInt(this.mAudioBitrate);
        dest.writeInt(this.mAudioSamplerate);
        dest.writeInt(this.mAudioResource);
        dest.writeByte(this.showDebug ? (byte) 1 : (byte) 0);
        dest.writeString(this.liveTitle);
        dest.writeString(this.liveDesc);
        dest.writeString(this.imageSrc);
    }

    protected NewPublisherConfig(Parcel in) {
        this.id = in.readLong();
        this.mHost = in.readString();
        this.mPort = in.readInt();
        this.mKey = in.readString();
        this.mSwitchCameraFacing = in.readInt();
        int mSupportResolutionSize = in.readInt();
        this.mSupportResolution = new HashMap<Integer, Map>(mSupportResolutionSize);
        for (int i = 0; i < mSupportResolutionSize; i++) {
            Integer key = (Integer) in.readValue(Integer.class.getClassLoader());
            Map value = in.readParcelable(Map.class.getClassLoader());
            this.mSupportResolution.put(key, value);
        }
        int mSupportFpsSize = in.readInt();
        this.mSupportFps = new HashMap<Integer, Map>(mSupportFpsSize);
        for (int i = 0; i < mSupportFpsSize; i++) {
            Integer key = (Integer) in.readValue(Integer.class.getClassLoader());
            Map value = in.readParcelable(Map.class.getClassLoader());
            this.mSupportFps.put(key, value);
        }
        this.mVideoMimeType = in.readString();
        this.mVideoResolution = in.readInt();
        this.mVideoBitrate = in.readInt();
        this.mVideoFramerate = in.readInt();
        this.mAudioMimeType = in.readString();
        this.mAudioBitrate = in.readInt();
        this.mAudioSamplerate = in.readInt();
        this.mAudioResource = in.readInt();
        this.showDebug = in.readByte() != 0;
        this.liveTitle = in.readString();
        this.liveDesc = in.readString();
        this.imageSrc = in.readString();
    }

    public static final Creator<NewPublisherConfig> CREATOR = new Creator<NewPublisherConfig>() {
        @Override
        public NewPublisherConfig createFromParcel(Parcel source) {
            return new NewPublisherConfig(source);
        }

        @Override
        public NewPublisherConfig[] newArray(int size) {
            return new NewPublisherConfig[size];
        }
    };
}
