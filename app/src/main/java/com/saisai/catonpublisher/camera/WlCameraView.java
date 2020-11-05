package com.saisai.catonpublisher.camera;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.camera2.CameraCharacteristics;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.ViewGroup;
import android.view.WindowManager;

import com.saisai.catonpublisher.egl.WLEGLSurfaceView;
import com.saisai.catonpublisher.newencode.NewPublisher;


public class WlCameraView extends WLEGLSurfaceView {

    private WlCameraRender wlCameraRender;
    private Camera2Loader wlCamera;

    private int textureId = -1;

    public WlCameraView(Context context) {
        this(context, null);
    }

    public WlCameraView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WlCameraView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        wlCameraRender = new WlCameraRender(context);
        wlCamera = new Camera2Loader(context);
        wlCamera.setCameraSwitchFacing(NewPublisher.getInstance().getCurrentCameraFacing());
        wlCamera.setFps(NewPublisher.getInstance().getFps());
        wlCamera.setLargest(NewPublisher.getInstance().getOutputSize());
        setRender(wlCameraRender);
        previewAngle(context);
        wlCameraRender.setOnSurfaceCreateListener(new WlCameraRender.OnSurfaceCreateListener() {
            @Override
            public void onSurfaceCreate(SurfaceTexture surfaceTexture, int tid) {
                wlCamera.setSurfaceTexture(surfaceTexture);
                textureId = tid;
                wlCamera.openCamera();
            }
        });
    }

    public void onDestroy() {
        if (wlCamera != null) {
            wlCamera.destroyCamera();
        }
    }

    public void previewAngle(Context context) {
        int angle = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay().getRotation();
        wlCameraRender.resetMatrix();
        switch (angle) {
            case Surface.ROTATION_0:
                Log.d("ywl5320", "0");
                if (wlCamera.getCameraSwitchFacing() == CameraCharacteristics.LENS_FACING_BACK) {
                    wlCameraRender.setAngle(90, 0, 0, 1);
                    wlCameraRender.setAngle(180, 1, 0, 0);
                } else {
                    wlCameraRender.setAngle(90f, 0f, 0f, 1f);
                }

                break;
            case Surface.ROTATION_90:
                Log.d("ywl5320", "90");
                if (wlCamera.getCameraSwitchFacing() == CameraCharacteristics.LENS_FACING_BACK) {
                    wlCameraRender.setAngle(180, 0, 0, 1);
                    wlCameraRender.setAngle(180, 0, 1, 0);
                } else {
                    wlCameraRender.setAngle(180f, 0f, 0f, 1f);
                }
                break;
            case Surface.ROTATION_180:
                Log.d("ywl5320", "180");
                if (wlCamera.getCameraSwitchFacing() == CameraCharacteristics.LENS_FACING_BACK) {
                    wlCameraRender.setAngle(90f, 0.0f, 0f, 1f);
                    wlCameraRender.setAngle(180f, 0.0f, 1f, 0f);
                } else {
                    wlCameraRender.setAngle(-90, 0f, 0f, 1f);
                }
                break;
            case Surface.ROTATION_270:
                Log.d("ywl5320", "270");
                if (wlCamera.getCameraSwitchFacing() == CameraCharacteristics.LENS_FACING_BACK) {
                    wlCameraRender.setAngle(180f, 0.0f, 1f, 0f);
                } else {
                    wlCameraRender.setAngle(0f, 0f, 0f, 1f);
                }
                break;
        }
    }

    public void resize(boolean isVer) {
        Size outputSize = NewPublisher.getInstance().getOutputSize();
        ViewGroup.LayoutParams layoutParams = getLayoutParams();
        if (isVer) {
            layoutParams.width = 1080;
            layoutParams.height = 1920;
        } else {
            layoutParams.width = 1920;
            layoutParams.height = 1080;
        }
        setLayoutParams(layoutParams);
    }

    public void setBitmap(Bitmap bitmap) {
        if (wlCameraRender != null) {
            wlCameraRender.setBitmap(bitmap);
        }
    }


    public void switchCamera() {
        wlCamera.switchCamera();
        previewAngle(getContext());
    }

    public int getTextureId() {
        return textureId;
    }
}
