package com.miooc.deskgirl;

import android.app.NativeActivity;
import android.content.res.AssetManager;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import com.deskgirl.demo.DebugServer;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.Random;
import java.util.Timer;
import java.util.TimerTask;

/**
 * Created by h4x on 15/4/22.
 */

/// @brief Main activity of DesktopGirl
public class MainActivity extends NativeActivity
{
    static
    {
        Log.d("MainActivity", "loading libraries");
        System.loadLibrary("main");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);

        dbgserv = new DebugServer(dbgservHandler);
        dbgserv.run();

        iat = new IatImplementation_iFlyTec(this, iatHandler);
        iat.init();

        wakeup = new WakeupImpl_PocketSphinx(this.wkupHandler);
        wakeup.init(this);

        wakeup.beginListening();

        // dbgserv.playMusicTest();

        player = new MediaPlayer();
        player.setVolume(0.8f, 0.8f);
        player.setOnCompletionListener(new MediaPlayer.OnCompletionListener()
        {
            @Override
            public void onCompletion(MediaPlayer mp)
            {
                iat.beginListen();
                setListenState(stateListening);
            }
        });
    }


    // listen state: 0-sleep 1-listening 2-anwser
    final int stateSleeping = 0;
    final int stateListening = 1;
    final int stateAnswering = 2;
    final int stateIATFailed = 3;
    final String[] stateName = { "Sleeping", "IAT", "Answering", "IAT Failed" };
    int listenState = 0;

    void setListenState(int state)
    {
        if(state == stateListening)
            dbgserv.setRenderBgColor(DebugServer.BG_Bright);
        else if(state == stateIATFailed)
            dbgserv.setRenderBgColor(DebugServer.BG_Error);
        else
            dbgserv.setRenderBgColor(DebugServer.BG_Normal);

        // log changes
        String logstr = String.format("Listening: %s -> %s", stateName[listenState], stateName[state]);
        Log.i("MainActivity", logstr);

        // change state
        listenState = state;
    }

    Random rnd = new Random(1234);
    MediaPlayer player;

    DebugServer dbgserv;
    IatImplementation_iFlyTec iat;
    WakeupImpl_PocketSphinx wakeup;

    // ---- handler for iat ----


    String[][][] iatResponses = {
            {{/*question not found*/},{"好像很复杂的样子", "miku也没有理解你的意思", "呜~~"}},
            {{"叫什么", "名字"}, {"可以叫我miku酱", "hatsunemiku", "我是世界第一公主殿下"},},
            {{"嫁给我"}, {"呜~~", "miku会害羞啦", "这样的问题不可以啦", "才不要呢", "哼", "哼2", "哼3", "哼4"}},
            {{"早上好", "早啊", "早呀"}, {"好像已经不早了呢"}},
            {{"可爱", "漂亮", "萌"}, {"miku会害羞啦", "呜~~", "喵~", "啊嘞", "哼", "哼2", "哼3", "哼4"}},
            {{"喵"}, {"喵~"}},
    };

    private void sleepAndListen()
    {
        iat.stopListen();

        // start wakeup listener after a short delay
        // wait for closing of the mic
        new Timer("wakeupStart", false).schedule(new TimerTask()
        {
            @Override
            public void run()
            {
                wakeup.beginListening();
                setListenState(stateSleeping);
            }
        }, 100);
    }

    private void iatListen()
    {
        wakeup.stopListening();
        new Timer("iatStart", false).schedule(new TimerTask()
        {
            @Override
            public void run()
            {
                boolean result = iat.beginListen();
                if(!result)
                {
                    sleepAndListen();
                    setListenState(stateIATFailed);
                }
                else
                {
                    setListenState(stateListening);
                }
            }
        }, 100);
    }

    IatImplementation_iFlyTec.Callback iatHandler = new IatImplementation_iFlyTec.Callback()
    {
        @Override
        public void endOfIatNoResult()
        {
            Log.i("MainActivity", "iat end without result");
            sleepAndListen();
        }

        @Override
        public void didReceiveIatResult(String textResult)
        {
            // dbgserv.processIatResult(textResult);


            Log.i("MainActivity", "didReceiveIatResult: " + textResult);
            Log.i("MainActivity", "current state: " + Integer.toString(listenState));

            if(textResult == null || textResult.length() == 0)
            {
                sleepAndListen();
                return;
            }

            String[] availableResponses = null;
            for(String[][] item: iatResponses)
            {
                String[] keywords = item[0];
                String[] responses = item[1];

                boolean matched = false;
                for(String kwd: keywords)
                {
                    if(textResult.contains(kwd))
                    {
                        matched = true;
                        break;
                    }
                }

                if(matched)
                {
                    availableResponses = responses;
                    break;
                }
            }

            if(availableResponses == null)
                availableResponses = iatResponses[0][1];

            Log.d("MainActivity", "Responsing at: " + availableResponses[0]);

            // play response mp3
            int rid = rnd.nextInt(availableResponses.length);
            String resp = availableResponses[rid];
            String filename = "/sdcard/voice/" + resp + ".m4a";
            try
            {
                player.reset();
                player.setDataSource(filename);
                player.prepare();
                player.start();
            }
            catch (Exception err)
            {
                Log.e("MainActivity", "Cannot play file:" +  filename + " reaseon: " + err.getMessage());
            }

            // pause iat
            iat.stopListen();

            // change state
            setListenState(stateAnswering);
        }
    };

    // ---- handler for wakeup server ----

    WakeupImpl_PocketSphinx.Callback wkupHandler = new WakeupImpl_PocketSphinx.Callback()
    {
        @Override
        public void onWakeup()
        {
            dbgserv.processWakeupEvent();
            iatListen();
        }

        @Override
        public void onError(Exception error)
        {
            Log.e("Wakeup", error.toString());
        }
    };

    // ---- handler for debug server ----

    DebugServer.Callback dbgservHandler = new DebugServer.Callback()
    {
        @Override
        public void onReceiveBeginIat()
        {
            iat.beginListen();
        }

        @Override
        public String onRequireAssetText(String filename)
        {
            try
            {
                AssetManager mgr = MainActivity.this.getAssets();
                BufferedReader reader = new BufferedReader(new InputStreamReader(mgr.open(filename), "utf-8"));

                StringBuilder sb = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null)
                {
                    sb.append(line);
                    sb.append("\n");
                }

                reader.close();

                return sb.toString();
            }
            catch(Exception err)
            {
                return null;
            }
        }
    };
}
