package com.miooc.deskgirl;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.webkit.JavascriptInterface;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import com.deskgirl.demo.DebugServer;
import com.example.myapp.R;
import com.iflytek.cloud.*;
import com.iflytek.speech.setting.IatSettings;
import com.iflytek.speech.util.JsonParser;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.LinkedHashMap;

/**
 * Created by h4x on 15/4/23.
 */
public class IatImplementation_iFlyTec
{
    Context context;
    Callback handler;

    Toast toast;

    public IatImplementation_iFlyTec(Context ctx, Callback h)
    {
        this.context = ctx;
        this.handler = h;
    }

    public void init()
    {
        toast = Toast.makeText(this.context, "", Toast.LENGTH_SHORT);

        // 应用程序入口处调用,避免手机内存过小,杀死后台进程后通过历史intent进入Activity造成SpeechUtility对象为null
        // 注意：此接口在非主进程调用会返回null对象，如需在非主进程使用语音功能，请增加参数：SpeechConstant.FORCE_LOGIN+"=true"
        // 参数间使用“,”分隔。
        // 设置你申请的应用appid
        SpeechUtility.createUtility(this.context, "appid=" + this.context.getString(R.string.app_id));

        mIat = SpeechRecognizer.createRecognizer(this.context, mInitListener);
        mSharedPreferences = this.context.getSharedPreferences(IatSettings.PREFER_NAME, Activity.MODE_PRIVATE);
    }

    public boolean beginListen()
    {
        mIatResults.clear();

        setParam();

        int ret = mIat.startListening(recognizerListener);
        if (ret != ErrorCode.SUCCESS) {
            showTip("听写失败,错误码：" + ret);
            return false;
        } else {
            showTip("请开始说话");
            return true;
        }
    }

    public void stopListen()
    {
        mIat.stopListening();
    }

    @Override
    protected void finalize() throws Throwable
    {
        mIat.cancel();
        mIat.destroy();
        super.finalize();
    }

    final String TAG = "DG_IAT";

    void showTip(String tip)
    {
        // uncomment to show tips:
        // toast.setText(tip);
        // toast.show();
        Log.i("iFlyIAT", tip);
    }

    String buildResult(RecognizerResult results)
    {
        String text = JsonParser.parseIatResult(results.getResultString());

        String sn = null;
        // 读取json结果中的sn字段
        try
        {
            JSONObject resultJson = new JSONObject(results.getResultString());
            sn = resultJson.optString("sn");
        }
        catch (JSONException e)
        {
            e.printStackTrace();
        }

        mIatResults.put(sn, text);

        StringBuilder resultBuffer = new StringBuilder();
        for (String key : mIatResults.keySet()) {
            resultBuffer.append(mIatResults.get(key));
        }

        return resultBuffer.toString();
    }

    // ---- IAT ----

    private SpeechRecognizer mIat;
    private HashMap<String, String> mIatResults = new LinkedHashMap<String, String>();
    private SharedPreferences mSharedPreferences;

    InitListener mInitListener = new InitListener() {

        @Override
        public void onInit(int code) {
            Log.d(TAG, "SpeechRecognizer init() code = " + code);
            if (code != ErrorCode.SUCCESS) {
                showTip("初始化失败，错误码：" + code);
            }
        }
    };

    RecognizerListener recognizerListener = new RecognizerListener() {

        @Override
        public void onBeginOfSpeech() {
            showTip("开始说话");
        }

        @Override
        public void onError(SpeechError error)
        {
            // Tips：
            // 错误码：10118(您没有说话)，可能是录音机权限被禁，需要提示用户打开应用的录音权限。
            // 如果使用本地功能（语音+）需要提示用户开启语音+的录音权限。
            showTip(error.getPlainDescription(true));

            if (error.getErrorCode() == 10118)
            {
                if(handler != null)
                    handler.endOfIatNoResult();
            }
        }

        @Override
        public void onEndOfSpeech()
        {
            // end of speech, but not end of IAT
            showTip("结束说话");
        }

        @Override
        public void onResult(RecognizerResult results, boolean isLast)
        {
            Log.d(TAG, results.getResultString());
            String textResult = buildResult(results);

            if (isLast)
            {
                // do callback
                // TODO: which thread are we running in??
                if(IatImplementation_iFlyTec.this.handler != null)
                {
                    IatImplementation_iFlyTec.this.handler.didReceiveIatResult(textResult);
                }
            }
        }

        @Override
        public void onVolumeChanged(int volume) {
            showTip("当前正在说话，音量大小：" + volume);
        }

        @Override
        public void onEvent(int eventType, int arg1, int arg2, Bundle obj)
        {
        }
    };

    public void setParam() {
        // 清空参数
        mIat.setParameter(SpeechConstant.PARAMS, null);

        // 设置听写引擎
        mIat.setParameter(SpeechConstant.ENGINE_TYPE, SpeechConstant.TYPE_CLOUD);
        // 设置返回结果格式
        mIat.setParameter(SpeechConstant.RESULT_TYPE, "json");

        String lag = mSharedPreferences.getString("iat_language_preference",
                "mandarin");
        if (lag.equals("en_us"))
        {
            // 设置语言
            mIat.setParameter(SpeechConstant.LANGUAGE, "en_us");
        }
        else
        {
            // 设置语言
            mIat.setParameter(SpeechConstant.LANGUAGE, "zh_cn");
            // 设置语言区域
            mIat.setParameter(SpeechConstant.ACCENT, lag);
        }
        // 设置语音前端点
        mIat.setParameter(SpeechConstant.VAD_BOS, mSharedPreferences.getString("iat_vadbos_preference", "4000"));
        // 设置语音后端点
        mIat.setParameter(SpeechConstant.VAD_EOS, mSharedPreferences.getString("iat_vadeos_preference", "1000"));
        // 设置标点符号
        mIat.setParameter(SpeechConstant.ASR_PTT, mSharedPreferences.getString("iat_punc_preference", "1"));
        // 设置音频保存路径
        mIat.setParameter(SpeechConstant.ASR_AUDIO_PATH, Environment.getExternalStorageDirectory()
                + "/com/iflytek/wavaudio.pcm");
        // 设置听写结果是否结果动态修正，为“1”则在听写过程中动态递增地返回结果，否则只在听写结束之后返回最终结果
        // 注：该参数暂时只对在线听写有效
        mIat.setParameter(SpeechConstant.ASR_DWA, mSharedPreferences.getString("iat_dwa_preference", "0"));
    }

    public interface Callback
    {
        void didReceiveIatResult(String textResult);
        void endOfIatNoResult();
    }
}
