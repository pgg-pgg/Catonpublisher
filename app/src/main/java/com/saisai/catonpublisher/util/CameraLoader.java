package com.saisai.catonpublisher.util;

public abstract class CameraLoader {
    public abstract int getCameraOrientation();
    public abstract void switchCamera();
    public abstract boolean hasMultipleCamera();
    public abstract void onPause();
    public abstract void onResume(int width,int height);
    public interface PreviewFrameListener{
        public void  onPreviewFrame(byte[] data, int width, int height);
    }
    public PreviewFrameListener previewFrameListener;

    public void setPreviewFrameListener(PreviewFrameListener previewFrameListener) {
        this.previewFrameListener = previewFrameListener;
    }
}
