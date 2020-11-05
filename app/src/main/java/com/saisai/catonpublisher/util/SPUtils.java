package com.saisai.catonpublisher.util;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.text.TextUtils;
import android.util.Base64;

import com.alibaba.fastjson.JSON;
import com.saisai.catonpublisher.MyApplication;
import com.saisai.catonpublisher.newencode.NewPublisherConfig;
import com.saisai.catonpublisher.newencode.SettingConfig;

import java.io.ByteArrayInputStream;
import java.util.List;

/**
 * Created by saisai on 2019/2/12 0012.
 */

public class SPUtils {
    public static String get(String key){
//        Log.e("tag",MyApplication.app+"");
        return MyApplication.app.getSharedPreferences("Config", Context.MODE_PRIVATE).getString(key,"");
    }
    public static void save(String key, String value){
        MyApplication.app.getSharedPreferences("Config", Context.MODE_PRIVATE).edit().putString(key,value).apply();
    }

    public static int getInt(String key){
        return MyApplication.app.getSharedPreferences("Config", Context.MODE_PRIVATE).getInt(key,-1);
    }
    public static void saveInt(String key, int value){
        MyApplication.app.getSharedPreferences("Config", Context.MODE_PRIVATE).edit().putInt(key,value).apply();
    }
    public static boolean getBoolean(String key){
        return MyApplication.app.getSharedPreferences("Config", Context.MODE_PRIVATE).getBoolean(key,true);
    }
    public static void saveBoolean(String key, boolean value){
        MyApplication.app.getSharedPreferences("Config", Context.MODE_PRIVATE).edit().putBoolean(key,value).apply();
    }

    /**
     * 存储配置信息
     * @param list 数据列表
     */
    public static void saveLiveData(List<NewPublisherConfig> list) {
        String json = JSON.toJSONString(list);
        save("live_data", json);
    }

    /**
     * 获取所有配置信息
     * @return 配置信息列表
     */
    public static List<NewPublisherConfig> getLiveData() {
        String liveData = get("live_data");
        if(TextUtils.isEmpty(liveData)) return null;
        return JSON.parseArray(liveData, NewPublisherConfig.class);
    }

    public static Bitmap getBitmap(Context context, String imageBase64) {
        if (TextUtils.isEmpty(imageBase64)) {
            return null;
        }
        byte[] base64Bytes = Base64.decode(imageBase64.getBytes(),
                Base64.DEFAULT);
        ByteArrayInputStream bais = new ByteArrayInputStream(base64Bytes);
        Bitmap ret = BitmapFactory.decodeStream(bais);
        if (ret != null) {
            return ret;
        } else {
            return null;
        }
    }

    public static Bitmap zoomImg(Bitmap bm, int newWidth, int newHeight) {
        int w = bm.getWidth(); // 得到图片的宽，高
        int h = bm.getHeight();
        int retX;
        int retY;
        double wh = (double) w / (double) h;
        double nwh = (double) newWidth / (double) newHeight;
        if (wh > nwh) {
            retX = h * newWidth / newHeight;
            retY = h;
        } else {
            retX = w;
            retY = w * newHeight / newWidth;
        }
        int startX = w > retX ? (w - retX) / 2 : 0;//基于原图，取正方形左上角x坐标
        int startY = h > retY ? (h - retY) / 2 : 0;
        Bitmap bit = Bitmap.createBitmap(bm, startX, startY, retX, retY, null, false);
        bm.recycle();
        return bit;
    }

    public static NewPublisherConfig getNewPublisherConfig(long id) {
        List<NewPublisherConfig> liveData = getLiveData();
        if (liveData != null && liveData.size() > 0) {
            for (NewPublisherConfig config: liveData) {
                if (config.id == id) {
                    return config;
                }
            }
        }
        return null;
    }

    /**
     * 保存设置信息
     * @param config 设置信息
     */
    public static void saveSetting(SettingConfig config){
        String s = JSON.toJSONString(config);
        save("live_setting", s);
    }

    /**
     * 获取设置信息
     * @return 返回设置信息
     */
    public static SettingConfig getSettingConfig() {
        String liveSetting = get("live_setting");
        if(TextUtils.isEmpty(liveSetting)) return null;
        return JSON.parseObject(liveSetting, SettingConfig.class);
    }

}
