package com.saisai.catonpublisher.util;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.media.Image;
import android.media.ImageReader;
import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Log;
import android.util.Size;
import android.view.Surface;

import java.util.ArrayList;
import java.util.List;

public class Camera2Loader extends CameraLoader {
    private String TAG = "Camera2Loader";
    private Activity activity;
    private CameraDevice cameraInstance;
    private CameraCaptureSession captureSession;
    private ImageReader imageReader;
    private int cameraFacing = CameraCharacteristics.LENS_FACING_BACK;
    private int viewWidth = 0;
    private int viewHeight = 0;

    private CameraManager cameraManager;

    public Camera2Loader(Activity activity) {
        this.activity = activity;
        cameraManager = (CameraManager) activity.getSystemService(Context.CAMERA_SERVICE);
    }

    @Override
    public void onResume(int width, int height) {
        viewWidth = width;
        viewHeight = height;
        setUpCamera();
    }

    @Override
    public void onPause() {
        releaseCamera();
    }
    @Override
    public void switchCamera() {

        switch (cameraFacing){
            case CameraCharacteristics.LENS_FACING_BACK:
                cameraFacing= CameraCharacteristics.LENS_FACING_FRONT;
                break;
            case CameraCharacteristics.LENS_FACING_FRONT:
                cameraFacing= CameraCharacteristics.LENS_FACING_BACK;
                break;
            default:
                break;
        }
        releaseCamera();
        setUpCamera();
    }
    @Override
    public int getCameraOrientation() {
        int degrees;
        switch (activity.getWindowManager().getDefaultDisplay().getRotation()){
            case Surface.ROTATION_0:
                degrees=0;
                break;
            case Surface.ROTATION_90:
                degrees=90;
                break;
            case Surface.ROTATION_180:
                degrees=180;
                break;
            case Surface.ROTATION_270:
                degrees=270;
                break;
            default:
                degrees=0;
                break;
        }

        String cameraId = getCameraId(cameraFacing);
        if (TextUtils.isEmpty(cameraId)){
            return 0;
        }
        try {
            CameraCharacteristics characteristics =cameraManager.getCameraCharacteristics(cameraId);
            int orientation =characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
            if (cameraFacing == CameraCharacteristics.LENS_FACING_FRONT) {
                return (orientation + degrees) % 360;
            } else { // back-facing
                return (orientation - degrees) % 360;
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
            return 0;
        }
    }
    @Override
    public boolean hasMultipleCamera() {
        int size = 0;
        try {
            size = cameraManager.getCameraIdList().length;
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        if (size>1){
            return true;
        }
        else {
            return false;
        }
    }
    @SuppressLint("MissingPermission")
    private void setUpCamera() {
        String cameraId = getCameraId(cameraFacing);
        if (TextUtils.isEmpty(cameraId)) {
            return;
        }
        try {
            cameraManager.openCamera(cameraId,new  CameraDeviceCallback(), null);
        } catch (CameraAccessException e) {
            Log.e(TAG, "Opening camera (ID: $cameraId) failed.");
        }

    }

    private String getCameraId(int facing) {
        String cameraId = "";
        try {
            String carmeraIds[] = cameraManager.getCameraIdList();
            for (String id : carmeraIds) {
                if (cameraManager.getCameraCharacteristics(id).get(CameraCharacteristics.LENS_FACING) == facing) {
                    cameraId = id;

                }
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        return cameraId;
    }

    class CameraDeviceCallback extends CameraDevice.StateCallback {

        @Override
        public void onOpened(@NonNull CameraDevice camera) {
            cameraInstance = camera;
            startCaptureSession();
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice camera) {
            camera.close();
            cameraInstance = null;
        }

        @Override
        public void onError(@NonNull CameraDevice camera, int error) {
            camera.close();
            cameraInstance = null;
        }
    }
    private void startCaptureSession() {
        Size size = chooseOptimalSize();
        imageReader = ImageReader.newInstance(size.getWidth(), size.getHeight(), ImageFormat.YUV_420_888, 2);
        imageReader.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() {
            @Override
            public void onImageAvailable(ImageReader reader) {
                Image image=reader.acquireLatestImage();
                if (null==image){
                    return;
                }
                previewFrameListener.onPreviewFrame(ImageExt.generateNV21Data(image), image.getWidth(), image.getHeight());
                image.close();
            }
        },null);



        try {
            List<Surface> surfaces=new ArrayList<>();
            surfaces.add(imageReader.getSurface());
            cameraInstance.createCaptureSession(surfaces,new CaptureStateCallback(),null);
        } catch (CameraAccessException e) {
            Log.e(TAG, "Failed to start camera session");
        }
    }
    private void releaseCamera() {
        imageReader.close();
        cameraInstance.close();
        captureSession.close();
        imageReader = null;
        cameraInstance = null;
        captureSession = null;
    }
    private Size chooseOptimalSize(){
        if (viewWidth == 0 || viewHeight == 0) {
            return new Size(0, 0);
        }
        String cameraId = getCameraId(cameraFacing);
        if (TextUtils.isEmpty(cameraId)){
            return new Size(0, 0);
        }
        try {
            Size[] outputSizes = cameraManager.getCameraCharacteristics(cameraId).get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP).getOutputSizes(ImageFormat.YUV_420_888);
            int orientation = getCameraOrientation();
            int maxPreviewWidth;
            int maxPreviewHeight;
            if(orientation==90||orientation==270){
                maxPreviewWidth=viewHeight;
                maxPreviewHeight=viewWidth;
            }
            else {
                maxPreviewWidth=viewWidth;
                maxPreviewHeight=viewHeight;
            }
            List<Size> sizes=new ArrayList<>();
            for (Size it:outputSizes){
                if (it.getWidth()<maxPreviewWidth/2&&it.getHeight()<maxPreviewHeight/2){
                    sizes.add(it);
                }
            }
            int rect=0;
            for (Size it2:sizes){
                if (it2.getHeight()*it2.getHeight()>rect){
                    rect=it2.getHeight()*it2.getHeight();
                   return it2;
                }
            }
            return new Size(480, 640);
        }
        catch (CameraAccessException e) {
            e.printStackTrace();
        }
        return new Size(480, 640);
    }

    class CaptureStateCallback extends CameraCaptureSession.StateCallback {
        @Override
        public void onConfigured(@NonNull CameraCaptureSession session) {
            if (null==cameraInstance){
                return;
            }
            captureSession = session;

            try {
                CaptureRequest.Builder builder=  cameraInstance.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                builder.addTarget(imageReader.getSurface());

                session.setRepeatingRequest(builder.build(), null, null);
            } catch (CameraAccessException e ) {
                Log.e(TAG, "Failed to start camera preview because it couldn't access camera", e);
            } catch (IllegalStateException e) {
                Log.e(TAG, "Failed to start camera preview.", e);
            }
        }

        @Override
        public void onConfigureFailed(@NonNull CameraCaptureSession session) {
            Log.e(TAG, "Failed to configure capture session.");
        }
    }
}
