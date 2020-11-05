package com.saisai.catonpublisher.camera;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Matrix;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.support.annotation.NonNull;
import android.support.annotation.RequiresApi;
import android.util.Log;
import android.util.Range;
import android.util.Size;
import android.view.Surface;
import android.widget.Toast;

import com.saisai.catonpublisher.newencode.NewPublisher;
import com.saisai.catonpublisher.newencode.NewPublisherConfig;
import com.saisai.catonpublisher.util.CustomToast;

import java.util.Arrays;

public class Camera2Loader {

    public Camera2Loader(Context context) {
        this.context = context;
        HandlerThread handlerThread = new HandlerThread("Camera2");
        handlerThread.start();
        mBackgroundHandler = new Handler(handlerThread.getLooper());
    }

    private Context context;
    private Size largest;
    private CameraCaptureSession mCaptureSession;
    private String mCameraId;
    private Handler mBackgroundHandler;
    private CaptureRequest.Builder mPreviewBuilder = null;
    private CameraDevice cameraDevice;    //camera
    private SurfaceTexture surfaceTexture;
    //默认开启前置摄像头
    private int cameraSwitchFacing = CameraCharacteristics.LENS_FACING_FRONT;
    private Range<Integer> fps;

    public void setCameraSwitchFacing(int cameraSwitchFacing) {
        this.cameraSwitchFacing = cameraSwitchFacing;
    }

    public void setLargest(Size largest) {
        this.largest = largest;
    }

    public void setFps(Range<Integer> fps) {
        this.fps = fps;
    }

    public void setSurfaceTexture(SurfaceTexture surfaceTexture) {
        this.surfaceTexture = surfaceTexture;
    }

    public int getCameraSwitchFacing() {
        return cameraSwitchFacing;
    }

    /*open camera*/
    @SuppressLint({"MissingPermission", "NewApi"})
    public void openCamera() {
        /*获得相机管理服务*/
        CameraManager manager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        try {
            setUpCameraOutputs();
            manager.openCamera(mCameraId, mCameraOpenCallback, mBackgroundHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void switchCamera() {
        int checkCameraFacing = cameraSwitchFacing == NewPublisherConfig.CAMERA2_FACING_BACK
                ? NewPublisherConfig.CAMERA2_FACING_FRONT : NewPublisherConfig.CAMERA2_FACING_BACK;

        if (NewPublisher.getInstance().checkSupportCamera(checkCameraFacing)
                != NewPublisher.CAMERA_IS_SUPPORT) {
            StringBuilder sb = new StringBuilder();

            if (cameraSwitchFacing == NewPublisherConfig.CAMERA2_FACING_BACK) {
                sb.append("后置摄像头不支持该");
            } else if (cameraSwitchFacing == NewPublisherConfig.CAMERA2_FACING_FRONT) {
                sb.append("前置摄像头不支持该");
            }

            if (NewPublisher.getInstance().checkSupportCamera(checkCameraFacing)
                    == NewPublisher.CAMERA_NOT_SUPPORT_RESOLUTION) {
                sb.append("分辨率，不能切换！");
            } else if (NewPublisher.getInstance().checkSupportCamera(checkCameraFacing)
                    == NewPublisher.CAMERA_NOT_SUPPORT_FPS) {
                sb.append("帧率，不能切换！");
            }

            CustomToast.makeText(context).show(sb.toString(), Toast.LENGTH_SHORT);
            return;
        }
        destroyCamera();
        cameraSwitchFacing = checkCameraFacing;
        openCamera();
    }


    @RequiresApi(api = Build.VERSION_CODES.M)
    private void setUpCameraOutputs() {
        CameraManager manager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        try {
            for (String cameraId : manager.getCameraIdList()) {
                CameraCharacteristics characteristics
                        = manager.getCameraCharacteristics(cameraId);
                Integer facing = characteristics.get(CameraCharacteristics.LENS_FACING);
                if (facing != null && facing != cameraSwitchFacing) {
                    continue;
                }
                StreamConfigurationMap map = characteristics.get(
                        CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                if (map == null) {
                    continue;
                }
                mCameraId = cameraId;
                return;
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException e) {
        }
    }

    private void configureTransform(int viewWidth, int viewHeight) {
        if (null == largest) {
            return;
        }

        int rotation = ((Activity)context).getWindowManager().getDefaultDisplay().getRotation();
        Matrix matrix = new Matrix();
        RectF viewRect = new RectF(0, 0, viewWidth, viewHeight);
        RectF bufferRect = new RectF(0, 0, largest.getHeight(), largest.getWidth());
        float centerX = viewRect.centerX();
        float centerY = viewRect.centerY();
        if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
            bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY());
            matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL);
            float scale = Math.max(
                    (float) viewHeight / largest.getHeight(),
                    (float) viewWidth / largest.getWidth());
            matrix.postScale(scale, scale, centerX, centerY);
            matrix.postRotate(90 * (rotation - 2), centerX, centerY);
        } else if (Surface.ROTATION_180 == rotation) {
            matrix.postRotate(180, centerX, centerY);
        }
//        mTextureView.setTransform(matrix);
    }

    /**
     * 摄像头打开回调
     */
    private CameraDevice.StateCallback mCameraOpenCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(@NonNull CameraDevice camera) {

            Log.e("tag", "thread name = " + Thread.currentThread().getName());
            cameraDevice = camera;
            try {
                surfaceTexture.setDefaultBufferSize(largest.getWidth(), largest.getHeight());
                Surface surface = new Surface(surfaceTexture);
                mPreviewBuilder = camera.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
                mPreviewBuilder.addTarget(surface);
                mPreviewBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, fps);
                camera.createCaptureSession(Arrays.asList(surface), mSessionCallback, mBackgroundHandler);
            } catch (CameraAccessException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice camera) { }

        @Override
        public void onError(@NonNull CameraDevice camera, int error) { }
    };


    /**
     * 录制视频回调
     */
    private CameraCaptureSession.StateCallback mSessionCallback = new CameraCaptureSession.StateCallback() {
        @Override
        public void onConfigured(@NonNull CameraCaptureSession session) {
            try {
                session.setRepeatingRequest(mPreviewBuilder.build(), null, mBackgroundHandler);
                mCaptureSession = session;
            } catch (CameraAccessException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onConfigureFailed(@NonNull CameraCaptureSession session) {
            Log.e("tag", "配置失败");
        }
    };

    /**
     * 销毁摄像头
     */
    public void destroyCamera() {
        if (mCaptureSession != null) {
            mCaptureSession.close();
            mCaptureSession = null;
        }
        if (cameraDevice != null) {
            cameraDevice.close();
            cameraDevice = null;
        }
    }
}
