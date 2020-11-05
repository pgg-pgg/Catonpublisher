package com.saisai.catonpublisher.newencode;

import android.content.Context;
import android.graphics.Bitmap;


public class WlPushEncodec extends WlBasePushEncoder{

    private WlEncodecPushRender wlEncodecPushRender;

    public WlPushEncodec(Context context, int textureId) {
        super(context);
        wlEncodecPushRender = new WlEncodecPushRender(context, textureId);
        setRender(wlEncodecPushRender);
        setmRenderMode(1);
    }

    public void setBitmap(Bitmap bitmap) {
        if (wlEncodecPushRender != null) {
            wlEncodecPushRender.setBitmap(bitmap);
        }
    }
}
