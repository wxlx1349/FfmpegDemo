package com.example.wangxi.ffmpegdemo;

import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.Surface;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.Toast;

import java.io.File;

/**
 * Created by wangxi on 2017/3/11.
 */

public class PlayerActivity extends AppCompatActivity {
    public static final String INPUT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar + "4yuv.yuv";
    VideoUtils videoUtils;
    VideoView videoView;
    private Spinner sp_video;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_player);
        videoView = (VideoView) findViewById(R.id.videoView);
        videoUtils = new VideoUtils();
        sp_video = (Spinner) findViewById(R.id.sp_video);
        //多种格式的视频列表
        String[] videoArray = getResources().getStringArray(R.array.video_list);
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1,
                android.R.id.text1, videoArray);
        sp_video.setAdapter(adapter);
    }


    public void mPlay(View view) {
        String video = sp_video.getSelectedItem().toString();
        final String output = new File(Environment.getExternalStorageDirectory(), video).getAbsolutePath();
        File file = new File(output);
        Toast.makeText(this, "is file excit" + file.exists(), Toast.LENGTH_SHORT).show();
        final Surface surface = videoView.getHolder().getSurface();
//        videoUtils.render(output,surface);
        try {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    videoUtils.play(output, surface);
                }
            }).start();
        } catch (Throwable e) {

        }

    }
}
