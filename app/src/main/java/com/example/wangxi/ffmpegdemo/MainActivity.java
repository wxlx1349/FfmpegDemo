package com.example.wangxi.ffmpegdemo;

import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    public static final String INPUT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar + "4.avi";
    public static final String OUTPUT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separatorChar + "4yuv.yuv";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

    }

    public void decode(View view) {
        File file=new File(INPUT_PATH);
        Toast.makeText(this,"is file excit"+file.exists(),Toast.LENGTH_SHORT).show();
        String input = new File(Environment.getExternalStorageDirectory(),"01.mp4").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(),"1yuv.yuv").getAbsolutePath();
        VideoUtils.decode(input, output);
    }

    public void sound(View view){
        String input = new File(Environment.getExternalStorageDirectory(),"3.flv").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(),"3.pcm").getAbsolutePath();
        new VideoUtils().sound(input, output);
    }

    public void toPlay(View view){
        startActivity(new Intent(this,PlayerActivity.class));
    }

}
