package com.saisai.catonpublisher.listener;
/**
 * Created by saisai on 2018/6/29 0029.
 */

/**
 * @Title: ParseStatListener
 * @Package com.saisai.catonpublisher
 * @Description:
 * @Author saisai
 * @Date 2018/6/29 0029
 * @Time 16:17
 * @Version
 */
public interface ParseStatListener {
    void onParse(float dropRate,int bitrate,int jitter,int rtt,int maxDelay);
}
