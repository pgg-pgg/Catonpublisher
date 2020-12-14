package com.saisai.catonpublisher.newencode;

import com.saisai.catonpublisher.Constants;

import java.io.Serializable;

public class SettingConfig implements Serializable {

    public int countDown = 0;
    public int coverType = Constants.TYPE_COVER.BLACK_TYPE;
    public boolean isShowTime = true;
    public boolean isShowDanmu = true;
    public String danmuText = "";
    public String base64 = "";
    public String mServiceProvider = "Caton";
    public String mServiceName = "CatonPublisher";
}
