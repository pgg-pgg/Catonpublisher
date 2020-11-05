package com.saisai.catonpublisher.listener;
/**
 * Created by saisai on 2018/6/29 0029.
 */

/**
 * @Title: PushListener
 * @Package com.saisai.catonpublisher
 * @Description:
 * @Author saisai
 * @Date 2018/6/29 0029
 * @Time 15:30
 * @Version
 */
public interface PushListener {
    //异步，不要阻塞他
    void event(int event,int error);

}
