package com.deskgirl.demo;

import android.media.MediaPlayer;
import android.os.Environment;
import android.util.Log;
import fi.iki.elonen.NanoHTTPD;
import org.apache.http.impl.io.ContentLengthInputStream;

import java.lang.reflect.Constructor;
import java.security.interfaces.DSAParams;
import java.util.Map;

/**
 * Created by h4x on 15/4/23.
 */
public class DebugServer extends NanoHTTPD
{
    Callback handler;
    MediaPlayer player;

    public DebugServer(Callback h)
    {
        super(8081);
        this.handler = h;
    }

    // printf(html, posename)
    String html = null;

    @Override
    public Response serve(IHTTPSession session)
    {
        Method method = session.getMethod();
        String uri = session.getUri();
        System.out.println(method + " '" + uri + "' ");

        Map<String, String> parms = session.getParms();

        if(parms.containsKey("action"))
        {
            String act = parms.get("action");
            if(act.equals("nextPose"))
            {
                Log.d("DebugServer", "calling nextPose");
                this.jniNextPose();
            }
            else if(act.equals("listen"))
            {
                if(handler != null)
                    handler.onReceiveBeginIat();
            }
            else if(act.equals("translation"))
            {
                if(parms.containsKey("x")
                    && parms.containsKey("y")
                    && parms.containsKey("z"))
                {
                    try
                    {
                        float x = Float.parseFloat(parms.get("x"));
                        float y = Float.parseFloat(parms.get("y"));
                        float z = Float.parseFloat(parms.get("z"));
                        jniSetupTranslate(x, y, z);
                    }
                    catch (NumberFormatException err)
                    {
                        Log.w("DebugServer", "Bad translation format. " + err.toString());
                    }
                }
            }
            else if(act.equals("playVmd"))
            {
                jniPlayVmd(parms.get("path"));
            }
            else if(act.equals("playLip"))
            {
                jniPlayLip(Float.parseFloat(parms.get("lipdur")));
            }
        }

        String msg;
        if(html == null && handler != null)
            html = handler.onRequireAssetText("DbgRoot/index.html");

        if(html == null)
            msg = "404 Not Found";
        else
            msg = String.format(html, jniGetCurPoseName());  // comp the html

        return new NanoHTTPD.Response(msg);
    }

    public void run()
    {
        try { this.start(); }
        catch (Exception e) { Log.e("DbgServer", e.toString()); }
    }

    public void processIatResult(String textResult)
    {
        if(textResult.contains("你好"))
        {
            jniDoPose(PoseHello);
        }
        else if(textResult.contains("春哥"))
        {
            jniDoPose(PoseConfusing);
        }
    }

    public void processWakeupEvent()
    {

    }

    public void playMusicTest()
    {
        try
        {
            player = new MediaPlayer();

            player.setVolume(0.8f, 0.8f);
            player.setDataSource(Environment.getExternalStorageDirectory().getPath() + "/eva.mp3");
            player.prepare();
            player.setLooping(true);
            player.start();
        }
        catch (Exception err)
        {
            Log.e("DbgServer", "Cannot load mp3: " + err.getMessage());
        }
    }

    public static final int BG_Normal = 0;
    public static final int BG_Bright = 1;
    public static final int BG_Error = 2;

    public void setRenderBgColor(int colorType)
    {
        if(colorType == BG_Bright)
            jniSetBgColor(15f/255f, 55f/255f, 60f/255f);
        else if(colorType == BG_Error)
            jniSetBgColor(90f/255f, 15f/255f, 15f/255f);
        else
            jniSetBgColor(0, 0, 0);
    }

    // functions

    // JNIEXPORT void JNICALL Java_com_deskgirl_demo_DebugServer_xxx(JNIEnv *, jobject);
    private native void jniNextPose();
    private native void jniDoPose(int posid);
    private native String jniGetCurPoseName();
    private native void jniSetupTranslate(float x, float y, float z);
    private native void jniPlayVmd(String path);
    private native void jniPlayLip(float d);
    private native void jniSetBgColor(float r, float g, float b);

    final int PoseHello = 19;
    final int PoseConfusing = 18;

    public interface Callback
    {
        void onReceiveBeginIat();
        String onRequireAssetText(String filename);
    }
}
