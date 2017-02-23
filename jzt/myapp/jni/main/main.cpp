/* ----------------------------------------------------------------- */
/*           The Toolkit for Building Voice Interaction Systems      */
/*           "MMDAgent" developed by MMDAgent Project Team           */
/*           http://www.mmdagent.jp/                                 */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2013  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAgent project team nor the names of  */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

/* definitions */

#define MAIN_TITLE          "MMDAgent - Toolkit for building voice interaction systems"
#define MAIN_DOUBLECLICKSEC 0.2

/* headers */

#ifdef _WIN32
#include <locale.h>
#include <windows.h>
#endif /* _WIN32 */
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif /* __APPLE__ */
#ifdef __ANDROID__
#include <jni.h>
#include <errno.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <unistd.h>
#include <sys/time.h>

#include "AndroidLogUtils.h"

#endif /* __ANDROID__ */
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__)
#include <limits.h>
#include <iconv.h>
#endif /* !_WIN32 && !__APPLE__ && !__ANDROID__ */

#include "MMDAgent.h"

/* MMDAgent */

MMDAgent *mmdagent;
bool enable;

/* Key */

bool shiftKeyL;
bool shiftKeyR;
bool ctrlKeyL;
bool ctrlKeyR;

/* Mouse */

double mouseLastClick;
int mousePosX;
int mousePosY;
int mouseLastWheel;

/* procWindowSizeMessage: process window resize message */
void GLFWCALL procWindowSizeMessage(int w, int h)
{
    ALog("main - procWindowSizeMessage(%d, %d)", w, h);
    
    if(enable == false)
        return;
    
    mmdagent->procWindowSizeMessage(w, h);
    mmdagent->updateAndRender();
}

/* procDropFileMessage: process drop files message */
void GLFWCALL procDropFileMessage(const char *file, int x, int y)
{
#ifdef __APPLE__
    CFStringRef cfs;
    size_t len;
    char *buff;
    
    if(enable == false)
        return;
    
    mousePosX = x;
    mousePosY = y;
    
    cfs = CFStringCreateWithCString(NULL, file, kCFStringEncodingUTF8);
    len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfs), kCFStringEncodingDOSJapanese) + 1;
    buff = (char *) malloc(len);
    CFStringGetCString(cfs, buff, len, kCFStringEncodingDOSJapanese);
    CFRelease(cfs);
    mmdagent->procDropFileMessage(buff,  mousePosX, mousePosY);
    free(buff);
#else
    if(enable == false)
        return;
    
    mmdagent->procDropFileMessage(file, mousePosX, mousePosY);
#endif /* __APPLE__ */
}

/* procKeyMessage: process key message */
void GLFWCALL procKeyMessage(int key, int action)
{
    if(enable == false)
        return;
    
    if(action == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_LSHIFT:
                shiftKeyL = true;
                break;
            case GLFW_KEY_RSHIFT:
                shiftKeyR = true;
                break;
            case GLFW_KEY_LCTRL:
                ctrlKeyL = true;
                break;
            case GLFW_KEY_RCTRL:
                ctrlKeyR = true;
                break;
            case GLFW_KEY_DEL:
                mmdagent->procDeleteModelMessage();
                break;
            case GLFW_KEY_ESC:
                enable = false;
                break;
            case GLFW_KEY_LEFT:
                if(ctrlKeyL == true || ctrlKeyR == true)
                    mmdagent->procTimeAdjustMessage(false);
                else if(shiftKeyL == true || shiftKeyR == true)
                    mmdagent->procHorizontalMoveMessage(false);
                else
                    mmdagent->procHorizontalRotateMessage(false);
                break;
            case GLFW_KEY_RIGHT:
                if(ctrlKeyL == true || ctrlKeyR == true)
                    mmdagent->procTimeAdjustMessage(true);
                else if(shiftKeyL == true || shiftKeyR == true)
                    mmdagent->procHorizontalMoveMessage(true);
                else
                    mmdagent->procHorizontalRotateMessage(true);
                break;
            case GLFW_KEY_UP:
                if(shiftKeyL == true || shiftKeyR == true)
                    mmdagent->procVerticalMoveMessage(true);
                else
                    mmdagent->procVerticalRotateMessage(true);
                break;
            case GLFW_KEY_DOWN:
                if(shiftKeyL == true || shiftKeyR == true)
                    mmdagent->procVerticalMoveMessage(false);
                else
                    mmdagent->procVerticalRotateMessage(false);
                break;
            case GLFW_KEY_PAGEUP:
                mmdagent->procScrollLogMessage(true);
                break;
            case GLFW_KEY_PAGEDOWN:
                mmdagent->procScrollLogMessage(false);
                break;
            default:
                break;
        }
    } else {
        switch(key) {
            case GLFW_KEY_LSHIFT:
                shiftKeyL = false;
                break;
            case GLFW_KEY_RSHIFT:
                shiftKeyR = false;
                break;
            case GLFW_KEY_LCTRL:
                ctrlKeyL = false;
                break;
            case GLFW_KEY_RCTRL:
                ctrlKeyR = false;
                break;
            default:
                break;
        }
    }
}

/* procCharMessage: process char message */
void GLFWCALL procCharMessage(int key, int action)
{
    if(enable == false)
        return;
    
    if(action == GLFW_RELEASE)
        return;
    
    ALog("key down char: %c", (char)key);
    
    switch(key) {
        case 'd':
        case 'D':
            mmdagent->procDisplayLogMessage();
            break;
        case 'f':
            mmdagent->procFullScreenMessage();
            break;
        case 's':
            mmdagent->procInfoStringMessage();
            break;
        case '+':
            mmdagent->procMouseWheelMessage(true, false, false);
            break;
        case '-':
            mmdagent->procMouseWheelMessage(false, false, false);
            break;
        case 'x':
            mmdagent->procShadowMappingMessage();
            break;
        case 'X':
            mmdagent->procShadowMappingOrderMessage();
            break;
        case 'W':
            mmdagent->procDisplayRigidBodyMessage();
            break;
        case 'w':
            mmdagent->procDisplayWireMessage();
            break;
        case 'b':
            mmdagent->procDisplayBoneMessage();
            break;
        case 'e':
            mmdagent->procCartoonEdgeMessage(true);
            break;
        case 'E':
            mmdagent->procCartoonEdgeMessage(false);
            break;
        case 'p':
            mmdagent->procPhysicsMessage();
            break;
        case 'h':
            mmdagent->procHoldMessage();
            break;
        case 'V':
            mmdagent->procVSyncMessage();
            break;
        default:
            break;
    }
    
    mmdagent->procKeyMessage((char) key);
}

/* procMouseButtonMessage: process mouse button message */
void GLFWCALL procMouseButtonMessage(int button, int action)
{
    if(enable == false)
        return;
    
    if(action == GLFW_PRESS) {
        switch(button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                if(MMDAgent_diffTime(MMDAgent_getTime(), mouseLastClick) <= MAIN_DOUBLECLICKSEC)
                    mmdagent->procMouseLeftButtonDoubleClickMessage(mousePosX, mousePosY);
                else
                    mmdagent->procMouseLeftButtonDownMessage(mousePosX, mousePosY, ctrlKeyL == true || ctrlKeyR == true ? true : false, shiftKeyL == true || shiftKeyR == true ? true : false);
                mouseLastClick = MMDAgent_getTime();
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                mmdagent->procMouseRightButtonDownMessage();
                break;
            default:
                break;
        }
    } else {
        switch(button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                mmdagent->procMouseLeftButtonUpMessage();
                break;
            default:
                break;
        }
    }
}

/* procMousePosMessage: process mouse position message */
void GLFWCALL procMousePosMessage(int x, int y, int shift, int ctrl)
{
    if(enable == false)
        return;
    
    mousePosX = x;
    mousePosY = y;
    shiftKeyL = shiftKeyR = shift == GLFW_PRESS ? true : false;
    ctrlKeyL = ctrlKeyR = ctrl == GLFW_PRESS ? true : false;
    
    mmdagent->procMousePosMessage(mousePosX, mousePosY, ctrlKeyL == true || ctrlKeyR == true ? true : false, shiftKeyL == true || shiftKeyR == true ? true : false);
}

/* procMouseWheelMessage: process mouse wheel message */
void GLFWCALL procMouseWheelMessage(int x)
{
    if(enable == false)
        return;
    
    mmdagent->procMouseWheelMessage(x > mouseLastWheel ? true : false, ctrlKeyL == true || ctrlKeyR == true ? true : false, shiftKeyL == true || shiftKeyR == true ? true : false);
    mouseLastWheel = x;
}

/* commonMain: common main function */
int commonMain(int argc, char **argv)
{
    enable = false;
    
    shiftKeyL = false;
    shiftKeyR = false;
    ctrlKeyL = false;
    ctrlKeyR = false;
    
    mouseLastClick = 0.0;
    mousePosX = 0;
    mousePosY = 0;
    mouseLastWheel = 0;
    
    ALog("MMDAgent creating...");
    
    /* create MMDAgent window */
    mmdagent = new MMDAgent();
    
    ALog("MMDAgent setting up...");
    if(mmdagent->setup(argc, argv, MAIN_TITLE) == false) {
        ALog("MMDAgent setup failed, disposing...");
        delete mmdagent;
        glfwTerminate();
        return -1;
    }
    
    ALog("glfw setting callbacks...");
    
    /* window */
    glfwSetWindowSizeCallback(procWindowSizeMessage);
    
    /* drag and drop */
    glfwSetDropFileCallback(procDropFileMessage);
    
    /* key */
    glfwSetKeyCallback(procKeyMessage);
    glfwEnable(GLFW_KEY_REPEAT);
    
    /* char */
    glfwSetCharCallback(procCharMessage);
    
    /* mouse */
    glfwSetMouseButtonCallback(procMouseButtonMessage);
    glfwSetMousePosCallback(procMousePosMessage);
    glfwSetMouseWheelCallback(procMouseWheelMessage);
    
    
    ALog("apply vmd (sd_mei_confusion.vmd)...");
    mmdagent->procDropFileMessage("/sdcard/MMDAgent_Example/Motion/sd_mei_confusion/sd_mei_confusion.vmd", 0, 0);
    
    // eye blink animation
    mmdagent->startEyeBlink();
    
    ALog("entering main loop...");
    
    ALog("---- OpenGL ES Driver Info ----");
    ALog("Vendor: %s", glGetString(GL_VENDOR));
    ALog("Version: %s", glGetString(GL_VERSION));
    ALog("Renderer: %s", glGetString(GL_RENDERER));
    ALog("Extensions: %s", glGetString(GL_EXTENSIONS));
    
    /* main loop */
    enable = true;
    
    // fps limit
    const double FPS = 60.0;
    const double FrameDuration = 1 / FPS;
    double accTime = 0.0;
    double lastUpdate = 0;
    
    while(enable == true && glfwGetWindowParam(GLFW_OPENED) == GL_TRUE)
    {
        double now = glfwGetTime();
        double dt = now - lastUpdate;
//        double sleepTime = FrameDuration - dt;
//        if(sleepTime > 0)
//            glfwSleep(sleepTime);
        // else: some frames are skipped
        
        double curFps = 1.0 / (glfwGetTime() - lastUpdate);
        
        accTime += dt;
        if(accTime >= 5.0)
        {
            ALog("fps = %lf (dt=%lf)", curFps, dt);
            accTime = 0.0;
        }
        
        bool r = mmdagent->updateAndRender();
        if(!r) ALog("updateAndRender() failed");
        
        lastUpdate = now;
    }
    
    ALog("exiting main loop...");
    
    /* free */
    mmdagent->procWindowDestroyMessage();
    delete mmdagent;
    return 0;
}

/* main: main function */
#if defined(_WIN32) && !defined(__MINGW32__)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int i;
    size_t len;
    int argc;
    wchar_t **wargv;
    char **argv;
    int result;
    
    setlocale(LC_CTYPE, "japanese");
    
    wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(argc < 1) return 0;
    argv = (char **) malloc(sizeof(char *) * argc);
    for(i = 0; i < argc; i++) {
        argv[i] = (char *) malloc(sizeof(char) * MMDAGENT_MAXBUFLEN);
        wcstombs_s(&len, argv[i], MMDAGENT_MAXBUFLEN, wargv[i], _TRUNCATE);
    }
    result = commonMain(argc, argv);
    for(i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
    
    return result;
}
#endif /* _WIN32 && !__MINGW32__ */
#if defined(_WIN32) && defined(__MINGW32__)
int main(int argc, char **argv)
{
    return commonMain(argc, argv);
}
#endif /* _WIN32 && __MIGW32__ */
#ifdef __APPLE__
int main(int argc, char **argv)
{
    int i;
    char inBuff[PATH_MAX + 1];
    CFStringRef cfs;
    size_t len;
    char **newArgv;
    int result;
    
    newArgv = (char **) malloc(sizeof(char *) * argc);
    for(i = 0; i < argc; i++) {
        /* prepare buffer */
        memset(inBuff, 0, PATH_MAX + 1);
        realpath(argv[i], inBuff);
        
        cfs = CFStringCreateWithCString(NULL, inBuff, kCFStringEncodingUTF8);
        len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfs), kCFStringEncodingDOSJapanese) + 1;
        newArgv[i] = (char *) malloc(len);
        CFStringGetCString(cfs, newArgv[i], len, kCFStringEncodingDOSJapanese);
        CFRelease(cfs);
    }
    result = commonMain(argc, newArgv);
    for(i = 0; i < argc; i++) {
        free(newArgv[i]);
    }
    free(newArgv);
    
    return result;
}
#endif /* __APPLE__ */
#ifdef __ANDROID__

void android_jnitest(JNIEnv* env)
{
    if(!env->FindClass("java/lang/String"))
    {
        ALog("FindClass(String) failed");
        return;
    }
    
                              //"com/deskgirl/demo/JNITest"
    jclass cls = env->FindClass("com/deskgirl/demo/JNITest");
    if(!cls)
    {
        ALog("FindClass(JNITest) failed");
        return;
    }
    
    jmethodID mGetStringFromJava = env->GetStaticMethodID(cls, "getStringFromJava", "()Ljava/lang/String;");
    if(!mGetStringFromJava)
    {
        ALog("GetMethodID(getStringFromJava():String) failed");
        return;
    }
    
    jobject oResult = env->CallStaticObjectMethod(cls, mGetStringFromJava);
    if(!oResult)
    {
        ALog("CallStaticObjectMethod(JNITest.getStringFromJava) returned a null pointer");
        return;
    }
    
    jboolean isCopy;
    const char* str = env->GetStringUTFChars((jstring)oResult, &isCopy);
    
    ALog("android_jnitest: getStringFromJava = %s", str);
}

void android_main(struct android_app *app)
{
    glfwInitForAndroid(app);
    app_dummy();
    
    // load main program
    
    char *argv[3];
    argv[0] = MMDAgent_strdup("dummy.exe");
    argv[1] = MMDAgent_strdup("dummy.mdf");
    //argv[2] = MMDAgent_strdup("/sdcard/MMDAgent_Example/Model/sd_mei/sd_mei.pmd");
    //argv[2] = MMDAgent_strdup("/sdcard/MMDAgent_Example/Model/rin/rin_normal2.pmd");
    //argv[2] = MMDAgent_strdup("/sdcard/MMDAgent_Example/Model/miku/miku.pmd");
    argv[2] = MMDAgent_strdup("/sdcard/MMDAgent_Example/Model/Nendoroid Miku POT/MMD_Miku_v1.1kai.pmd");
    commonMain((sizeof(argv)/sizeof(argv[0])), argv);
    free(argv[0]);
    free(argv[1]);
    free(argv[2]);

    app->activity->vm->DetachCurrentThread();
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    ALog("JNI_Onload(%p, %p)", vm, reserved);
    
    
    JNIEnv* env = NULL;
    if(vm->GetEnv((void**)&env, JNI_VERSION_1_6) < 0) ALog("GetEnv failed");
    if(vm->AttachCurrentThread(&env, NULL) < 0) ALog("Attach failed");
    ALog("android_main: JNIEnv* env = %p", env);
    
    android_jnitest(env);
    
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL Java_com_deskgirl_demo_DebugServer_jniNextPose(JNIEnv *, jobject)
{
    if(mmdagent)
    {
        // switch to next pose
        ALog("Switch to next pose!!!!!!!!!");
        mmdagent->demoSwitchMotion();
    }
}

extern "C"
JNIEXPORT void JNICALL Java_com_deskgirl_demo_DebugServer_jniDoPose(JNIEnv *, jobject, jint n)
{
    if(mmdagent)
    {
        ALog("JniDoPose(%d)", n);
        mmdagent->demoSwitchToPose(n);
    }
}

extern "C"
JNIEXPORT jstring JNICALL Java_com_deskgirl_demo_DebugServer_jniGetCurPoseName(JNIEnv* env, jobject)
{
    if(mmdagent)
    {
        ALog("JniGetCurPoseName()");
        const char* name = mmdagent->demoGetMotionName();
        jstring jstrName = env->NewStringUTF(name);
        
        return jstrName;
    }
    return NULL;
}

extern "C"
JNIEXPORT void JNICALL Java_com_deskgirl_demo_DebugServer_jniSetupTranslate(JNIEnv* env, jobject, float x, float y, float z)
{
    if(mmdagent)
    {
        ALog("JniSetupTranslate(%f, %f, %f)", x, y, z);
        mmdagent->demoSetModelTranslation(x, y, z);
    }
}

extern "C"
JNIEXPORT void JNICALL Java_com_deskgirl_demo_DebugServer_jniPlayVmd(JNIEnv* env, jobject, jstring path)
{
    if(mmdagent)
    {
        jboolean iscopy;
        const char* cpath = env->GetStringUTFChars(path, &iscopy);
        ALog("JniPlayVmd(%s)", cpath);
        mmdagent->demoPlayVmd(cpath);
    }
}

extern "C"
JNIEXPORT void JNICALL Java_com_deskgirl_demo_DebugServer_jniPlayLip(JNIEnv* env, jobject, float dur)
{
    if(mmdagent)
    {
        ALog("JniPlayLip(%f)", dur);
        mmdagent->demoPlayFakeLip(dur);
    }
}

extern "C"
JNIEXPORT void JNICALL Java_com_deskgirl_demo_DebugServer_jniSetBgColor(JNIEnv* env, jobject, float r, float g, float b)
{
    if(mmdagent)
    {
        mmdagent->demoSetBgColor(r, g, b);
    }
}

#endif /* __ANDROID__ */
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__)
int main(int argc, char **argv)
{
    int i;
    iconv_t ic;
    char **newArgv;
    char inBuff[PATH_MAX + 1], outBuff[PATH_MAX + 1];
    char *inStr, *outStr;
    size_t inLen, outLen;
    int result = 0;
    
    ic = iconv_open("SHIFT-JIS", MAIN_CHARSET);
    if(ic < 0)
        return -1;
    
    newArgv = (char **) malloc(sizeof(char *) * argc);
    for(i = 0; i < argc; i++) {
        /* prepare buffer */
        memset(inBuff, 0, PATH_MAX + 1);
        memset(outBuff, 0, PATH_MAX + 1);
        realpath(argv[i], inBuff);
        
        inStr = &inBuff[0];
        outStr = &outBuff[0];
        
        inLen = MMDAgent_strlen(inStr);
        outLen = MMDAGENT_MAXBUFLEN;
        
        if(iconv(ic, &inStr, &inLen, &outStr, &outLen) < 0) {
            result = -1;
            strcpy(outBuff, "");
        }
        
        newArgv[i] = MMDAgent_strdup(outBuff);
    }
    
    iconv_close(ic);
    
    if(result >= 0)
        result = commonMain(argc, newArgv);
    
    for(i = 0; i < argc; i++) {
        if(newArgv[i] != NULL)
            free(newArgv[i]);
    }
    free(newArgv);
    
    return result;
}
#endif /* !_WIN32 && !__APPLE__ && !__ANDROID__ */
