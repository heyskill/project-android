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

#define RENDER_SHADOWPCF                 /* use hardware PCF for shadow mapping */
#define RENDER_SHADOWAUTOVIEW            /* automatically define depth frustum */
#define RENDER_SHADOWAUTOVIEWANGLE 15.0f /* view angle for automatic depth frustum */

#define RENDER_MINMOVEDIFF       0.000001f
#define RENDER_MOVESPEEDRATE     0.9f
#define RENDER_MINSPINDIFF       0.000001f
#define RENDER_SPINSPEEDRATE     0.9f
#define RENDER_MINDISTANCEDIFF   0.1f
#define RENDER_DISTANCESPEEDRATE 0.9f
#define RENDER_MINFOVYDIFF       0.01f
#define RENDER_FOVYSPEEDRATE     0.9f

#define RENDER_VIEWPOINTFRUSTUMNEAR 5.0f      /* this value should be 0.5f. but, it has a problem in android. */
#define RENDER_VIEWPOINTFRUSTUMFAR  100000.0f

#define RENDER_VIEWPORT_SCALE    (0.5f*0.8f)

/* RenderDepthData: depth data for model ordering */
typedef struct {
    float dist;
    short   id;
} RenderDepthData;

typedef struct RenderInfo
{
    // args for Render::renderScene()
    PMDObject *objs;
    const int *order;
    int num;
    Stage *stage;
    bool useMMDLikeCartoon;
    bool useCartoonRendering;
    float lightIntensity;
    float *lightDirection;
    float *lightColor;
    bool useShadowMapping;
    int shadowMappingTextureSize;
    bool shadowMappingLightFirst;
    float shadowMappingSelfDensity;
    float shadowMappingFloorDensity;
    double ellapsedTimeForMove;
    
    // options for diagonal rendering
    float modelScale;
    float modelRaise;
    
    RenderInfo():
        objs(NULL),
        order(NULL),
        num(0),
        stage(NULL),
        useMMDLikeCartoon(false),
        useCartoonRendering(false),
        lightIntensity(0.0f),
        lightDirection(NULL),
        lightColor(NULL),
        useShadowMapping(false),
        shadowMappingTextureSize(0),
        shadowMappingLightFirst(false),
        shadowMappingSelfDensity(0.0f),
        shadowMappingFloorDensity(0.0f),
        ellapsedTimeForMove(0.0),
        modelScale(1.0f),
        modelRaise(0.0f) { }
    
} RenderInfo;

/* Render: render */
class Render
{
private:
    int m_viewportSide;      /* viewport length of side */
    int m_width;             /* window width */
    int m_height;            /* winodw height */
    
    /* rotations for quartered rendering */
    bool m_quarteredForceUpdate;    /* force update projecton & modelview matrices */
    float m_viewportRotAngle;       /* viewport rotate angle */
    float m_cameraRotAngle;         /* camera rotat angle */
    
    /* diagonal rendering configurations */
    int m_diagonalMethod;           /* indicates how to render diagonally (0-off, 1-directly, 2-posteffect) */
    float m_diagonalScale;          /* the scale factor for framebuffer (fb.side/vp.side)  */
    GLuint m_fbo;                   /* framebuffer object for diagonal rendering */
    GLuint m_fboSide;               /* side length of framebuffer */
    GLuint m_fboViewportSide;       /* side length of viewport in framebuffer */
    GLuint m_fboColorBuffer;        /* color buffer for fbo */
    GLuint m_fboDepthBuffer;        /* depth buffer for fbo */
    GLfloat m_fboViewportUV[8];     /* uv texture coord for viewport (order: lb,rb,rt,lt) */
    GLfloat m_clearColor[3];
    
    btVector3 m_trans;       /* view trans vector */
    btVector3 m_angle;       /* view angles */
    btQuaternion m_rot;      /* view rotation */
    float m_distance;        /* view distance */
    float m_fovy;            /* view fovy */
    
    btVector3 m_currentTrans;     /* current view trans vector */
    btQuaternion m_currentRot;    /* current view rotation */
    float m_currentDistance;      /* current view distance */
    float m_currentFovy;          /* current view fovy */
    btTransform m_transMatrix;    /* current trans vector + rotation matrix */
    btTransform m_transMatrixInv; /* current trans vector + inverse of rotation matrix */
    btScalar m_rotMatrix[16];     /* current rotation + OpenGL rotation matrix */
    btScalar m_rotMatrixInv[16];  /* current rotation + inverse of OpenGL rotation matrix */
    
    double m_viewMoveTime;           /* view length in sec */
    bool m_viewControlledByMotion;   /* true when view is controlled by motion */
    btVector3 m_viewMoveStartTrans;  /* transition at start of view move */
    btQuaternion m_viewMoveStartRot; /* rotation at start of view move */
    float m_viewMoveStartDistance;   /* distance at start of view move */
    float m_viewMoveStartFovy;       /* distance at start of view move */
    
    float m_backgroundColor[3]; /* background color */
    
    bool m_shadowMapInitialized;           /* true if initialized */
    GLuint m_depthTextureID;               /* depth texture for FBO */
    GLuint m_fboID;                        /* frame buffer object name */
    btVector3 m_lightVec;                  /* light vector for shadow maapping */
    btVector3 m_shadowMapAutoViewEyePoint; /* view point of shadow mapping */
    float m_shadowMapAutoViewRadius;       /* radius from view point */
    
    RenderDepthData *m_depth;              /* depth data of each model for reordering */
    
    /* updateProjectionMatrix: update view information */
    void updateProjectionMatrix();
    
    /* applyProjectionMatirx: update projection matrix */
    void applyProjectionMatrix();
    
    /* updateModelViewMatrix: update model view matrix */
    void updateModelViewMatrix();
    
    /* updateTransRotMatrix:  update trans and rotation matrix */
    bool updateTransRotMatrix(double ellapsedTimeForMove);
    
    /* updateRotationFromAngle: update rotation quaternion from angle */
    void updateRotationFromAngle();
    
    /* updateDistance: update distance */
    bool updateDistance(double ellapsedTimeForMove);
    
    /* updateFovy: update fovy */
    bool updateFovy(double ellapsedTimeForMove);
    
    /* initializeShadowMap: initialize OpenGL for shadow mapping */
    void initializeShadowMap(int textureSize);
    
    /* renderSceneShadowMap: shadow mapping */
    void renderSceneShadowMap(RenderInfo* info);
    
    /* renderScene: render scene */
    void renderScene(RenderInfo* info);
    
    /* initialize: initialzie Render */
    void initialize();
    
    /* clear: free Render */
    void clear();
    
public:
    
    /* Render: constructor */
    Render();
    
    /* ~Render: destructor */
    ~Render();
    
    /* setup: initialize and setup Render (size: window size, color: clear color)*/
    bool setup(const int *size, const float *color, const float *trans, const float *rot, float distance, float fovy, bool useShadowMapping, int shadowMappingTextureSize, bool shadowMappingLightFirst, int maxNumModel);
    
    /* setSize: set size */
    void setWindowSize(int w, int h);
    
    /* getWidth: get width */
    int getWindowWidth();
    
    /* getHeight: get height */
    int getWindowHeight();
    
    /* getScreenViewportSide: get viewport length of side */
    int getScreenViewportSide();
    
    /* resetCameraView: reset camera view */
    void resetCameraView(const float *trans, const float *angle, float distance, float fovy);
    
    /* setCameraParam: set camera view parameter from camera controller */
    void setCameraFromController(CameraController *c);
    
    /* setViewMoveTimer: reset timer in sec for rotation, transition, and scale of view */
    void setViewMoveTimer(double sec);
    
    /* isViewMoving: return if view is moving by timer */
    bool isViewMoving();
    
    /* translate: translate */
    void translate(float x, float y, float z);
    
    /* rotate: rotate scene */
    void rotate(float x, float y, float z);
    
    /* setDistance: set distance */
    void setDistance(float distance);
    
    /* getDistance: get distance */
    float getDistance();
    
    /* setFovy: set fovy */
    void setFovy(float distance);
    
    /* getFovy: get fovy */
    float getFovy();
    
    /* setShadowMapping: switch shadow mapping */
    void setShadowMapping(bool useShadowMapping, int textureSize, bool shadowMappingLightFirst);
    
    /* getRenderOrder: return rendering order */
    void getRenderOrder(int *order, PMDObject *objs, int num);
    
    /* render: render all */
    void render(RenderInfo* info);
    
    /* pickModel: pick up a model at the screen position */
    int pickModel(PMDObject *objs, int num, int x, int y, int *allowDropPicked);
    
    /* updateLigit: update light */
    void updateLight(bool useMMDLikeCartoon, bool useCartoonRendering, float lightIntensity, const float *lightDirection, const float *lightColor);
    
    /* updateDepthTextureViewParam: update center and radius information to get required range for shadow mapping */
    void updateDepthTextureViewParam(PMDObject *objList, int num);
    
    /* getScreenPointPosition: convert screen position to object position */
    void getScreenPointPosition(btVector3 *dst, btVector3 *src);
    
    /* getCurrentViewCenterPos: get current view center position */
    void getCurrentViewCenterPos(btVector3 *pos);
    
    /* getCurrentViewTransform: get current view transform matrix */
    void getCurrentViewTransform(btTransform *tr);
    
    /* getInfoString: store current view parameters to buffer */
    void getInfoString(char *buf);
    
    /* setQuarteredForceUpdate: set wether force update projection & modelview matrices */
    void setQuarteredForceUpdate(bool update) { m_quarteredForceUpdate = update; }
    
    /* setQuartationRotations: set rotations for quartered renering (in degree) */
    void setQuarteredRotations(float rotCamera, float rotViewport) { m_cameraRotAngle = rotCamera; m_viewportRotAngle = rotViewport; }
    
    /* setDiagonalRendering: set whether to rendering diagonally */
    void setDiagonalRendering(int method);
    
    /* setDiagonalScale: set the diagonal scale factor (fb.side/vp.side) */
    void setDiagonalScale(float f) { m_diagonalScale = f; }
 
    /* renderQuartered: render quartered (normally or diagonally) */
    void renderQuartered(RenderInfo* info);
    
    /* roundPowerof2: find the nearest power-of-two number which is not smaller than `side` */
    static int roundPowerof2(float side);
    
    void setRenderBackgroundColor(float r, float g, float b);
    
private:
    
    void renderQuarteredInternal(RenderInfo* info, float targetWidth, float targetHeight, float targetViewportSide);
    
    void renderQuartered_DirectDiagonal(RenderInfo* info, float targetWidth, float targetHeight, float targetViewportSide);
    
public:
    
    static float demoModelTrans[3];
    
};
