
CatonPublisher SDK

可采用项目中的example采集音频和视频数据推到RRS服务器，也可以自定义采集音视频推到RRS服务器

1、使用SDK的example：
    
    支持：
        视频编码：h264（mediacodec）
        视频分辨率：480P、720P、1080P
        视频码率：500Kbps-20Mbps
        音频编码：aac（mediacodec）
        音频采样率：44100
        音频码率：64000、128000
        
    参考ConfigActivity.java中参数配置
    
        PublisherConfig config = new PublisherConfig.Builder()
                //设置连接的服务器ip地址、端口号、key
                .setConnectParams(ip.substring(2, ip.length()), port, edtKey.getText().toString())
                //是否显示推流状态
                .setShowDebug(cbDebug.isChecked())
                //设置要开启的摄像头
                .setCameraId(switchCameraId)
                //设置预览的视频分辨率
                .setVideoResolution(switchResolution)
                //设置视频帧率
                .setFrameRate(switchVideoFrameRate)
                //设置视频码率
                .setInitAverageVideoBitrate(switchVideoBitrate)
                //设置音频码率
                .setAudioBitrate(switchAudioBitrate)
                //设置音频采样率
                .setSampleAudioRateInHz(PublisherConfig.AUDIO_RATE_HZ)
                .build();
        //初始化推流配置
        Publisher.getInstance().init(config);
        //跳转到推流Activity
        VideoActivity.intentTo(ConfigActivity.this);
        
2、自定义音视频采集编码（仅使用推流模块）

    支持：
        视频编码：h264
        音频编码：aac

    1）设置监听
       //设置推流过程中的事件监听
       Jni.getInstance().setPushListener(this);
       //设置推流状态的监听
       Jni.getInstance().setParseStatListener(this);
    2）连接RRS（在子线程中调用）
        int result=Jni.getInstance().connect(ip,port,key);
        if(result==0){
            //success
        }
    3）设置回调函数
        //设置之后有事件发生时，事件监听会收到回调信息
        Jni.getInstance().setCallback();
        
    4）推流
        //接收每一帧编码后的数据
        Jni.getInstance().push(mediaType,isKeyFrame,pts,dts,mFrameByte,length);
    
    5）获取当前推流状态
        //每调用一次，推流状态监听函数回调一次
        Jni.getInstance().parseStat();
        
    6）断开RRS连接
        Jni.getInstance().disconnect();