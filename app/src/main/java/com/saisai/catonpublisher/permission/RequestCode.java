package com.saisai.catonpublisher.permission;
/**
 * Created by saisai on 2018/6/29 0029.
 */

/**
 * @Title: RequestCode
 * @Package com.saisai.catonpublisher.permission
 * @Description:
 * @Author saisai
 * @Date 2018/6/29 0029
 * @Time 18:09
 * @Version
 */
public interface RequestCode {
    /**
     * 电话
     */
    int PHONE = 0x00;
    /**
     * 位置
     */
    int LOCATION = 0x01;
    /**
     * 相机
     */
    int CAMERA = 0x02;
    /**
     * 语音
     */
    int AUDIO = 0x04;
    /**
     * 存储
     */
    int EXTERNAL = 0x08;
    /**
     * 多个
     */
    int MORE = 0x10;
}
