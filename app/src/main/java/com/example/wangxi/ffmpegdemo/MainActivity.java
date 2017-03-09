package com.example.wangxi.ffmpegdemo;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    public static final String INPUT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar + "5.avi";
    public static final String OUTPUT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar + "5yuv.avi";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

    }

    public void decode(View view) {
        VideoUtils.decode(INPUT_PATH, OUTPUT_PATH);
    }

}
