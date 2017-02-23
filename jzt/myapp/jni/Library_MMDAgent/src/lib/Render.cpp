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

/* headers */

#include "MMDAgent.h"
#include "AndroidLogUtils.h"

/* compareDepth: qsort function for reordering */
static int compareDepth(const void *a, const void *b)
{
    RenderDepthData *x = (RenderDepthData *) a;
    RenderDepthData *y = (RenderDepthData *) b;
    
    if (x->dist == y->dist)
        return 0;
    return ( (x->dist > y->dist) ? 1 : -1 );
}

/* Render::updateProjectionMatrix: update view information */
void Render::updateProjectionMatrix()
{
    // original:
    // glViewport(0, 0, m_width, m_height);
    
    // to switch to quartered view port, we need set the viewport at
    //      MMDAgent::renderScene()
    
    /* camera setting */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    applyProjectionMatrix();
    glMatrixMode(GL_MODELVIEW);
}

/* Render::applyProjectionMatirx: update projection matrix */
void Render::applyProjectionMatrix()
{
//    static int times = 0;
//    
//    times++;
//    if(times % (4*5*60) == 0)
//    {
//        ALog("==== Projection settings ==== %d", times);
//        ALog(".   near = %f", RENDER_VIEWPOINTFRUSTUMNEAR);
//        ALog(".    far = %f", RENDER_VIEWPOINTFRUSTUMFAR);
//        ALog(".   fovy = %f", m_currentFovy);
//        ALog(".  width = %d", m_width);
//        ALog(". height = %d", m_height);
//    }
    
    // viewport rotation
    glRotatef(m_viewportRotAngle, 0, 0, 1);
    
    double y = RENDER_VIEWPOINTFRUSTUMNEAR * tan(MMDFILES_RAD(m_currentFovy) * 0.5);
    
    // original:
    //   double x = y * m_width / m_height;
    
    // switch to square viewport
    double x = y;
    
    glFrustum(-x, x, -y, y, RENDER_VIEWPOINTFRUSTUMNEAR, RENDER_VIEWPOINTFRUSTUMFAR);
}

/* Render::updateModelViewMatrix: update model view matrix */
void Render::updateModelViewMatrix()
{
    m_transMatrix.setIdentity();
    m_transMatrix.setRotation(m_currentRot);
    m_transMatrix.setOrigin(m_transMatrix * ( - m_currentTrans) - btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(m_currentDistance)));
    m_transMatrixInv = m_transMatrix.inverse();
    m_transMatrix.getOpenGLMatrix(m_rotMatrix);
    m_transMatrixInv.getOpenGLMatrix(m_rotMatrixInv);
}

/* Render::updateTransRotMatrix:  update trans and rotation matrix */
bool Render::updateTransRotMatrix(double ellapsedTimeForMove)
{
    float diff1, diff2;
    btVector3 trans;
    btQuaternion rot;
    
    /* if no difference, return */
    if (m_currentRot == m_rot && m_currentTrans == m_trans)
        return false;
    
    if (m_viewMoveTime == 0.0 || m_viewControlledByMotion == true) {
        /* immediately apply the target */
        m_currentRot = m_rot;
        m_currentTrans = m_trans;
    } else if (m_viewMoveTime > 0.0) {
        /* constant move */
        if (ellapsedTimeForMove >= m_viewMoveTime) {
            m_currentRot = m_rot;
            m_currentTrans = m_trans;
        } else {
            m_currentTrans = m_viewMoveStartTrans.lerp(m_trans, btScalar(ellapsedTimeForMove / m_viewMoveTime));
            m_currentRot = m_viewMoveStartRot.slerp(m_rot, btScalar(ellapsedTimeForMove / m_viewMoveTime));
        }
    } else {
        /* calculate difference */
        trans = m_trans;
        trans -= m_currentTrans;
        diff1 = trans.length2();
        rot = m_rot;
        rot -= m_currentRot;
        diff2 = rot.length2();
        
        if (diff1 > RENDER_MINMOVEDIFF)
            m_currentTrans = m_currentTrans.lerp(m_trans, btScalar(1.0f - RENDER_MOVESPEEDRATE)); /* current * 0.9 + target * 0.1 */
        else
            m_currentTrans = m_trans;
        if (diff2 > RENDER_MINSPINDIFF)
            m_currentRot = m_currentRot.slerp(m_rot, btScalar(1.0f - RENDER_SPINSPEEDRATE)); /* current * 0.9 + target * 0.1 */
        else
            m_currentRot = m_rot;
    }
    
    return true;
}

/* Render::updateRotationFromAngle: update rotation quaternion from angle */
void Render::updateRotationFromAngle()
{
    m_rot = btQuaternion(btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(1.0f)), btScalar(MMDFILES_RAD(m_angle.z())))
    * btQuaternion(btVector3(btScalar(1.0f), btScalar(0.0f), btScalar(0.0f)), btScalar(MMDFILES_RAD(m_angle.x())))
    * btQuaternion(btVector3(btScalar(0.0f), btScalar(1.0f), btScalar(0.0f)), btScalar(MMDFILES_RAD(m_angle.y())));
}

/* Render::updateDistance: update distance */
bool Render::updateDistance(double ellapsedTimeForMove)
{
    float diff;
    
    /* if no difference, return */
    if (m_currentDistance == m_distance)
        return false;
    
    if (m_viewMoveTime == 0.0 || m_viewControlledByMotion == true) {
        /* immediately apply the target */
        m_currentDistance = m_distance;
    } else if (m_viewMoveTime > 0.0) {
        /* constant move */
        if (ellapsedTimeForMove >= m_viewMoveTime) {
            m_currentDistance = m_distance;
        } else {
            m_currentDistance = m_viewMoveStartDistance + (m_distance - m_viewMoveStartDistance) * (float)(ellapsedTimeForMove / m_viewMoveTime);
        }
    } else {
        diff = fabs(m_currentDistance - m_distance);
        if (diff < RENDER_MINDISTANCEDIFF) {
            m_currentDistance = m_distance;
        } else {
            m_currentDistance = m_currentDistance * (RENDER_DISTANCESPEEDRATE) + m_distance * (1.0f - RENDER_DISTANCESPEEDRATE);
        }
    }
    
    return true;
}

/* Render::updateFovy: update fovy */
bool Render::updateFovy(double ellapsedTimeForMove)
{
    float diff;
    
    /* if no difference, return */
    if (m_currentFovy == m_fovy)
        return false;
    
    if (m_viewMoveTime == 0.0 || m_viewControlledByMotion == true) {
        /* immediately apply the target */
        m_currentFovy = m_fovy;
    } else if (m_viewMoveTime > 0.0) {
        /* constant move */
        if (ellapsedTimeForMove >= m_viewMoveTime) {
            m_currentFovy = m_fovy;
        } else {
            m_currentFovy = m_viewMoveStartFovy + (m_fovy - m_viewMoveStartFovy) * (float)(ellapsedTimeForMove / m_viewMoveTime);
        }
    } else {
        diff = fabs(m_currentFovy - m_fovy);
        if (diff < RENDER_MINFOVYDIFF) {
            m_currentFovy = m_fovy;
        } else {
            m_currentFovy = m_currentFovy * (RENDER_FOVYSPEEDRATE) + m_fovy * (1.0f - RENDER_FOVYSPEEDRATE);
        }
    }
    
    return true;
}

/* Render::initializeShadowMap: initialize OpenGL for shadow mapping */
void Render::initializeShadowMap(int textureSize)
{
#ifndef MMDAGENT_DONTUSESHADOWMAP
    static const GLdouble genfunc[][4] = {
        { 1.0, 0.0, 0.0, 0.0 },
        { 0.0, 1.0, 0.0, 0.0 },
        { 0.0, 0.0, 1.0, 0.0 },
        { 0.0, 0.0, 0.0, 1.0 },
    };
    
    /* initialize model view matrix */
    glPushMatrix();
    glLoadIdentity();
    
    /* use 4th texture unit for depth texture, make it current */
    glActiveTextureARB(GL_TEXTURE3_ARB);
    
    /* prepare a texture object for depth texture rendering in frame buffer object */
    glGenTextures(1, &m_depthTextureID);
    glBindTexture(GL_TEXTURE_2D, m_depthTextureID);
    
    /* assign depth component to the texture */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, textureSize, textureSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
    
    /* set texture parameters for shadow mapping */
#ifdef RENDER_SHADOWPCF
    /* use hardware PCF */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif /* RENDER_SHADOWPCF */
    
    /* tell OpenGL to compare the R texture coordinates to the (depth) texture value */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    
    /* also tell OpenGL to get the compasiron result as alpha value */
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_ALPHA);
    
    /* set texture coordinates generation mode to use the raw texture coordinates (S, T, R, Q) in eye view */
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGendv(GL_S, GL_EYE_PLANE, genfunc[0]);
    glTexGendv(GL_T, GL_EYE_PLANE, genfunc[1]);
    glTexGendv(GL_R, GL_EYE_PLANE, genfunc[2]);
    glTexGendv(GL_Q, GL_EYE_PLANE, genfunc[3]);
    
    /* finished configuration of depth texture: unbind the texture */
    glBindTexture(GL_TEXTURE_2D, 0);
    
    /* allocate a frame buffer object (FBO) for depth buffer rendering */
    glGenFramebuffersEXT(1, &m_fboID);
    /* switch to the newly allocated FBO */
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fboID);
    /* bind the texture to the FBO, telling that it should render the depth information to the texture */
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_depthTextureID, 0);
    /* also tell OpenGL not to draw and read the color buffers */
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    /* check FBO status */
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
        /* cannot use FBO */
    }
    /* finished configuration of FBO, now switch to default frame buffer */
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    
    /* reset the current texture unit to default */
    glActiveTextureARB(GL_TEXTURE0_ARB);
    
    /* restore the model view matrix */
    glPopMatrix();
#endif /* !MMDAGENT_DONTUSESHADOWMAP */
}

/* Render::renderSceneShadowMap: shadow mapping */
void Render::renderSceneShadowMap(RenderInfo* info)
{
    ALog("renderSceneShadowMap");
    
#ifndef MMDAGENT_DONTUSESHADOWMAP
    short i;
    GLint viewport[4]; /* store viewport */
    GLdouble modelview[16]; /* store model view transform */
    GLdouble projection[16]; /* store projection transform */
    bool toonLight = true;
    
#ifdef RENDER_SHADOWAUTOVIEW
    float eyeDist;
    btVector3 v;
#endif /* RENDER_SHADOWAUTOVIEW */
    
    static GLfloat lightdim[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    static const GLfloat lightblk[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    /* render the depth texture */
    /* store the current viewport */
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    /* store the current projection matrix */
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    
    /* switch to FBO for depth buffer rendering */
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fboID);
    
    /* clear the buffer */
    /* clear only the depth buffer, since other buffers will not be used */
    glClear(GL_DEPTH_BUFFER_BIT);
    
    /* set the viewport to the required texture size */
    glViewport(0, 0, shadowMappingTextureSize, shadowMappingTextureSize);
    
    /* reset the projection matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    /* set the model view matrix to make the light position as eye point and capture the whole scene in the view */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
#ifdef RENDER_SHADOWAUTOVIEW
    /* set the distance to cover all the model range */
    eyeDist = m_shadowMapAutoViewRadius / sinf(RENDER_SHADOWAUTOVIEWANGLE * 0.5f * 3.1415926f / 180.0f);
    /* set the perspective */
    gluPerspective(RENDER_SHADOWAUTOVIEWANGLE, 1.0, 1.0, eyeDist + m_shadowMapAutoViewRadius + 50.0f); /* +50.0f is needed to cover the background */
    /* the viewpoint should be at eyeDist far toward light direction from the model center */
    v = m_lightVec * eyeDist + m_shadowMapAutoViewEyePoint;
    gluLookAt(v.x(), v.y(), v.z(), m_shadowMapAutoViewEyePoint.x(), m_shadowMapAutoViewEyePoint.y(), m_shadowMapAutoViewEyePoint.z(), 0.0, 1.0, 0.0);
#else
    /* fixed view */
    gluPerspective(25.0, 1.0, 1.0, 120.0);
    gluLookAt(30.0, 77.0, 30.0, 0.0, 17.0, 0.0, 0.0, 1.0, 0.0);
#endif /* RENDER_SHADOWAUTOVIEW */
    
    /* keep the current model view for later process */
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    
    /* do not write into frame buffer other than depth information */
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    
    /* also, lighting is not needed */
    glDisable(GL_LIGHTING);
    
    /* disable rendering the front surface to get the depth of back face */
    glCullFace(GL_FRONT);
    
    /* disable alpha test */
    glDisable(GL_ALPHA_TEST);
    
    /* we are now writing to depth texture using FBO, so disable the depth texture mapping here */
    glActiveTextureARB(GL_TEXTURE3_ARB);
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);
    
    /* set polygon offset to avoid "moire" */
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(4.0, 4.0);
    
    /* render objects for depth */
    /* only objects that wants to drop shadow should be rendered here */
    for (i = 0; i < num; i++) {
        if (objs[order[i]].isEnable() == true) {
            objs[order[i]].getPMDModel()->renderForShadow();
        }
    }
    
    /* reset the polygon offset */
    glDisable(GL_POLYGON_OFFSET_FILL);
    
    /* switch to default FBO */
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    
    /* revert configurations to normal rendering */
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projection);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_LIGHTING);
    glCullFace(GL_BACK);
    glEnable(GL_ALPHA_TEST);
    
    /* clear all the buffers */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    /* render the full scene */
    /* set model view matrix, as the same as normal rendering */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(m_rotMatrix);
    
    /* render the whole scene */
    if (shadowMappingLightFirst) {
        /* render light setting, later render only the shadow part with dark setting */
        stage->renderBackground();
        stage->renderFloor();
        for (i = 0; i < num; i++) {
            if (objs[order[i]].isEnable() == true) {
                if (objs[order[i]].getPMDModel()->getToonFlag() == false && toonLight == true) {
                    /* disable toon lighting */
                    updateLight(true, false, lightIntensity, lightDirection, lightColor);
                    toonLight = false;
                } else if (objs[order[i]].getPMDModel()->getToonFlag() == true && toonLight == false) {
                    /* enable toon lighting */
                    updateLight(useMMDLikeCartoon, useCartoonRendering, lightIntensity, lightDirection, lightColor);
                    toonLight = true;
                }
                objs[order[i]].getPMDModel()->renderModel();
            }
        }
        if (toonLight == false) {
            /* restore toon lighting */
            updateLight(useMMDLikeCartoon, useCartoonRendering, lightIntensity, lightDirection, lightColor);
        }
    } else {
        /* render in dark setting, later render only the non-shadow part with light setting */
        /* light setting for non-toon objects */
        lightdim[0] = lightdim[1] = lightdim[2] = 0.34f - 0.13f * shadowMappingSelfDensity;
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdim);
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightdim);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightblk);
        
        /* render the non-toon objects (back, floor, non-toon models) */
        stage->renderBackground();
        stage->renderFloor();
        for (i = 0; i < num; i++) {
            if (objs[order[i]].isEnable() == true && objs[order[i]].getPMDModel()->getToonFlag() == false)
                objs[order[i]].getPMDModel()->renderModel();
        }
        
        /* for toon objects, they should apply the model-defined toon texture color at texture coordinates (0, 0) for shadow rendering */
        /* so restore the light setting */
        if (useCartoonRendering == true)
            updateLight(useMMDLikeCartoon, useCartoonRendering, lightIntensity, lightDirection, lightColor);
        /* render the toon objects */
        for (i = 0; i < num; i++) {
            if (objs[order[i]].isEnable() == true && objs[order[i]].getPMDModel()->getToonFlag() == true) {
                /* set texture coordinates for shadow mapping */
                objs[order[i]].getPMDModel()->updateShadowColorTexCoord(shadowMappingSelfDensity);
                /* tell model to render with the shadow corrdinates */
                objs[order[i]].getPMDModel()->setSelfShadowDrawing(true);
                /* render model and edge */
                objs[order[i]].getPMDModel()->renderModel();
                /* disable shadow rendering */
                objs[order[i]].getPMDModel()->setSelfShadowDrawing(false);
            }
        }
        if (useCartoonRendering == false)
            updateLight(useMMDLikeCartoon, useCartoonRendering, lightIntensity, lightDirection, lightColor);
    }
    
    /* render the part clipped by the depth texture */
    /* activate the texture unit for shadow mapping and make it current */
    glActiveTextureARB(GL_TEXTURE3_ARB);
    
    /* set texture matrix (note: matrices should be set in reverse order) */
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    /* move the range from [-1,1] to [0,1] */
    glTranslated(0.5, 0.5, 0.5);
    glScaled(0.5, 0.5, 0.5);
    /* multiply the model view matrix when the depth texture was rendered */
    glMultMatrixd(modelview);
    /* multiply the inverse matrix of current model view matrix */
    glMultMatrixf(m_rotMatrixInv);
    
    /* revert to model view matrix mode */
    glMatrixMode(GL_MODELVIEW);
    
    /* enable texture mapping with texture coordinate generation */
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
    glEnable(GL_TEXTURE_GEN_Q);
    
    /* bind the depth texture rendered at the first step */
    glBindTexture(GL_TEXTURE_2D, m_depthTextureID);
    
    /* depth texture set up was done, now switch current texture unit to default */
    glActiveTextureARB(GL_TEXTURE0_ARB);
    
    /* set depth func to allow overwrite for the same surface in the following rendering */
    glDepthFunc(GL_LEQUAL);
    
    if (shadowMappingLightFirst) {
        /* the area clipped by depth texture by alpha test is dark part */
        glAlphaFunc(GL_GEQUAL, 0.1f);
        
        /* light setting for non-toon objects */
        lightdim[0] = lightdim[1] = lightdim[2] = 0.34f - 0.13f * shadowMappingSelfDensity;
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdim);
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightdim);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightblk);
        
        /* render the non-toon objects (back, floor, non-toon models) */
        stage->renderBackground();
        stage->renderFloor();
        for (i = 0; i < num; i++) {
            if (objs[order[i]].isEnable() == true && objs[order[i]].getPMDModel()->getToonFlag() == false)
                objs[order[i]].getPMDModel()->renderModel();
        }
        
        /* for toon objects, they should apply the model-defined toon texture color at texture coordinates (0, 0) for shadow rendering */
        /* so restore the light setting */
        if (useCartoonRendering == true)
            updateLight(useMMDLikeCartoon, useCartoonRendering, lightIntensity, lightDirection, lightColor);
        /* render the toon objects */
        for (i = 0; i < num; i++) {
            if (objs[order[i]].isEnable() == true && objs[order[i]].getPMDModel()->getToonFlag() == true) {
                /* set texture coordinates for shadow mapping */
                objs[order[i]].getPMDModel()->updateShadowColorTexCoord(shadowMappingSelfDensity);
                /* tell model to render with the shadow corrdinates */
                objs[order[i]].getPMDModel()->setSelfShadowDrawing(true);
                /* render model and edge */
                objs[order[i]].getPMDModel()->renderModel();
                /* disable shadow rendering */
                objs[order[i]].getPMDModel()->setSelfShadowDrawing(false);
            }
        }
        if (useCartoonRendering == false)
            updateLight(useMMDLikeCartoon, useCartoonRendering, lightIntensity, lightDirection, lightColor);
    } else {
        /* the area clipped by depth texture by alpha test is light part */
        glAlphaFunc(GL_GEQUAL, 0.001f);
        stage->renderBackground();
        stage->renderFloor();
        for (i = 0; i < num; i++) {
            if (objs[order[i]].isEnable() == true) {
                if (objs[order[i]].getPMDModel()->getToonFlag() == false && toonLight == true) {
                    /* disable toon lighting */
                    updateLight(true, false, lightIntensity, lightDirection, lightColor);
                    toonLight = false;
                } else if (objs[order[i]].getPMDModel()->getToonFlag() == true && toonLight == false) {
                    /* enable toon lighting */
                    updateLight(useMMDLikeCartoon, useCartoonRendering, lightIntensity, lightDirection, lightColor);
                    toonLight = true;
                }
                objs[order[i]].getPMDModel()->renderModel();
            }
        }
        if (toonLight == false) {
            /* restore toon lighting */
            updateLight(useMMDLikeCartoon, useCartoonRendering, lightIntensity, lightDirection, lightColor);
        }
    }
    
    /* reset settings */
    glDepthFunc(GL_LESS);
    glAlphaFunc(GL_GEQUAL, 0.0001f);
    
    glActiveTextureARB(GL_TEXTURE3_ARB);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);
#endif /* !MMDAGENT_DONTUSESHADOWMAP */
}

/* Render::renderScene: render scene */
void Render::renderScene(RenderInfo* info)
{
    short i;
    bool toonLight = true;
    PMDObject* objs = info->objs;
    const int* order = info->order;
    
    /* clear rendering buffer */
    // move to MMDAgent::renderScene glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    
    /* set model view matrix */
    glLoadIdentity();
    glMultMatrixf(m_rotMatrix);
    
#ifdef MMDAgent_RenderStageShadow
    
    /* stage and shadow */
    /* background */
    stage->renderBackground();
    /* enable stencil */
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, ~0);
    /* make stencil tag true */
    glStencilOp(GL_KEEP, GL_KEEP , GL_REPLACE);
    /* render floor */
    stage->renderFloor();
    /* render shadow stencil */
    glColorMask(0, 0, 0, 0) ;
    glDepthMask(0);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, ~0);
    /* increment 1 pixel stencil */
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    /* render moodel */
    glDisable(GL_DEPTH_TEST);
    glPushMatrix();
    glMultMatrixf(stage->getShadowMatrix());
    for (i = 0; i < num; i++) {
        if (objs[order[i]].isEnable() == true) {
            objs[order[i]].getPMDModel()->renderForShadow();
        }
    }
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glColorMask(1, 1, 1, 1);
    glDepthMask(1);
    /* if stencil is 2, render shadow with blend on */
    glStencilFunc(GL_EQUAL, 2, ~0);
    glDisable(GL_LIGHTING);
    glColor4f(0.1f, 0.1f, 0.1f, shadowMappingFloorDensity);
    glDisable(GL_DEPTH_TEST);
    stage->renderFloor();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_LIGHTING);
#endif
    
    if(m_quarteredForceUpdate)
    {
        // rotations
        glRotatef(-m_cameraRotAngle, 0, 1, 0);
    }
    
    float modelScale = info->modelScale;
    glScalef(modelScale, modelScale, modelScale);
    glTranslatef(
        demoModelTrans[0],
        demoModelTrans[1] + info->modelRaise,
        demoModelTrans[2]
    );
    
    /* render model */
    for (i = 0; i < info->num; i++) {
        if (objs[order[i]].isEnable() == true) {
            bool objToolFlag = objs[order[i]].getPMDModel()->getToonFlag();
            
            if (objToolFlag == false && toonLight == true) {
                /* disable toon lighting */
                updateLight(true, false, info->lightIntensity, info->lightDirection, info->lightColor);
                toonLight = false;
            } else if (objToolFlag == true && toonLight == false) {
                /* enable toon lighting */
                updateLight(info->useMMDLikeCartoon, info->useCartoonRendering, info->lightIntensity, info->lightDirection, info->lightColor);
                toonLight = true;
            }
            objs[order[i]].getPMDModel()->renderModel();
        }
    }
    
    if (toonLight == false) {
        /* restore toon lighting */
        updateLight(info->useMMDLikeCartoon, info->useCartoonRendering, info->lightIntensity, info->lightDirection, info->lightColor);
    }
    
    static bool firstPrint = true;
    static int pt = 0;
    
//    if(firstPrint || (pt++) % (60*4*3) == 0)
//    {
//        firstPrint = false;
//        ALog("==== Render settings ====");
//        ALog(".   fovy = %f", this->m_fovy);
//        ALog(".  width = %d", this->m_width);
//        ALog(". height = %d", this->m_height);
//        ALog(". transx = %f", (float)this->m_trans[0]);
//        ALog(". transy = %f", (float)this->m_trans[1]);
//        ALog(". transz = %f", (float)this->m_trans[2]);
//        
////        maxy = RENDER_VIEWPOINTFRUSTUMNEAR * tan(MMDFILES_RAD(m_currentFovy) * 0.5);
////        maxx = maxy * (float)this->m_width / (float)this->m_height;
//    }
    
    return;
    
    static float maxy = 0.0f, maxx;
    static GLfloat ttt[] = {
        -1, 0, 0,
        1, 0, 0,
        0, 1, 0
    };
    static GLubyte idx[] = {
        0, 1, 2
    };
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    
    glScalef(20, 20, 1);
    glColor4f(1, 0, 1, 1);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, ttt);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, idx);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // clear matrics and draw triangle
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //glMultMatrixf(m_rotMatrix);
    //glScalef(maxx, maxy, 1);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(-1, 1, -1, 1, -1, 1);
    
    glColor4f(0, 1, 1, .3);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, ttt);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, idx);
    glScalef(1, -1, 1);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, idx);
    
    if(glGetError() != GL_NO_ERROR) while(1);
    
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glColor4f(1, 1, 1, 1);
}

/* Render::initialize: initialzie Render */
void Render::initialize()
{
    m_width = 0;
    m_height = 0;
    m_viewportSide = 0;     // [length of side]
    m_quarteredForceUpdate = false;
    m_cameraRotAngle = 0;
    m_viewportRotAngle = 0;
    m_trans = btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f));
    m_angle = btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f));
    updateRotationFromAngle();
    m_distance = 100.0f;
    m_fovy = 16.0f;
    
    m_currentTrans = m_trans;
    m_currentRot = m_rot;
    m_currentDistance = m_distance;
    m_currentFovy = m_fovy;
    
    m_viewMoveTime = -1.0;
    m_viewControlledByMotion = false;
    
    m_transMatrix.setIdentity();
    updateModelViewMatrix();
    
    m_shadowMapInitialized = false;
    m_lightVec = btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f));
    m_shadowMapAutoViewEyePoint = btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f));
    m_shadowMapAutoViewRadius = 0.0f;
    
    m_depth = NULL;
    
    m_diagonalMethod = 0;
    m_diagonalScale = 1.15f;
    m_fbo = 0;
    m_fboColorBuffer = 0;
    m_fboDepthBuffer = 0;
}

/* Render::clear: free Render */
void Render::clear()
{
    if(m_depth)
        free(m_depth);
    initialize();
}

/* Render::Render: constructor */
Render::Render()
{
    initialize();
}

/* Render::~Render: destructor */
Render::~Render()
{
    clear();
}

/* Render::setup: initialize and setup Render */
bool Render::setup(const int *size, const float *color, const float *trans, const float *rot, float distance, float fovy, bool useShadowMapping, int shadowMappingTextureSize, bool shadowMappingLightFirst, int maxNumModel)
{
    if(size == NULL || color == NULL || rot == NULL || trans == NULL)
        return false;
    
    resetCameraView(trans, rot, distance, fovy);
    setViewMoveTimer(0.0);
    
    /* set clear color */
    glClearColor(color[0], color[1], color[2], 0.0f);
    glClearStencil(0);
    
    // save clear color
    m_clearColor[0] = color[0];
    m_clearColor[1] = color[1];
    m_clearColor[2] = color[2];
    
    /* enable depth test */
    glEnable(GL_DEPTH_TEST);
    
    /* enable texture */
    glEnable(GL_TEXTURE_2D);
    
    /* enable face culling */
    glEnable(GL_CULL_FACE);
    /* not render the back surface */
    glCullFace(GL_BACK);
    
    /* enable alpha blending */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    /* enable alpha test, to avoid zero-alpha surfaces to depend on the rendering order */
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 0.0001f);
    
    /* enable lighting */
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    
    /* initialization for shadow mapping */
    setShadowMapping(useShadowMapping, shadowMappingTextureSize, shadowMappingLightFirst);
    
    setWindowSize(size[0], size[1]);
    
    m_depth = (RenderDepthData *) malloc(sizeof(RenderDepthData) * maxNumModel);
    
    return true;
}

/* Render::setSize: set size */
void Render::setWindowSize(int w, int h)
{
    ALog("Render::setSize(%d, %d)", w, h);
    if (m_width != w || m_height != h) {
        ALog("Render::setSize - setting width & height");
        if (w > 0)
            m_width = w;
        if (h > 0)
            m_height = h;
        m_viewportSide = (w < h) ? w : h;   // [side]
        ALog("Render::setSize - updating projection matrix");
        updateProjectionMatrix();
    }
}

/* Render::getWidth: get width */
int Render::getWindowWidth()
{
    return m_width;
}

/* Render::getHeight: get height */
int Render::getWindowHeight()
{
    return m_height;
}

/* Render::getViewportSide: get viewport length of side */
int Render::getScreenViewportSide()
{
    return m_viewportSide;
}

/* Render::resetCameraView: reset camera view */
void Render::resetCameraView(const float *trans, const float *angle, float distance, float fovy)
{
    m_angle = btVector3(btScalar(angle[0]), btScalar(angle[1]), btScalar(angle[2]));
    m_trans = btVector3(btScalar(trans[0]), btScalar(trans[1]), btScalar(trans[2]));
    m_distance = distance;
    m_fovy = fovy;
    updateRotationFromAngle();
}

/* Render::setCameraParam: set camera view parameter from camera controller */
void Render::setCameraFromController(CameraController *c)
{
    if (c != NULL) {
        c->getCurrentViewParam(&m_distance, &m_trans, &m_angle, &m_fovy);
        updateRotationFromAngle();
        m_viewControlledByMotion = true;
    } else
        m_viewControlledByMotion = false;
}

/* Render::setViewMoveTimer: set timer in sec for rotation, transition, and scale of view */
void Render::setViewMoveTimer(double sec)
{
    m_viewMoveTime = sec;
    if (m_viewMoveTime > 0.0) {
        m_viewMoveStartRot = m_currentRot;
        m_viewMoveStartTrans = m_currentTrans;
        m_viewMoveStartDistance = m_currentDistance;
        m_viewMoveStartFovy = m_currentFovy;
    }
}

/* Render::isViewMoving: return if view is moving by timer */
bool Render::isViewMoving()
{
    if (m_viewMoveTime > 0.0 && (m_currentRot != m_rot || m_currentTrans != m_trans || m_currentDistance != m_distance || m_currentFovy != m_fovy))
        return true;
    return false;
}

/* Render::translate: translate */
void Render::translate(float x, float y, float z)
{
    m_trans += btVector3(btScalar(x), btScalar(y), btScalar(z));
}

/* Render::rotate: rotate scene */
void Render::rotate(float x, float y, float z)
{
    m_angle.setX(btScalar(m_angle.x() + x));
    m_angle.setY(btScalar(m_angle.y() + y));
    m_angle.setZ(btScalar(m_angle.z() + z));
    updateRotationFromAngle();
}

/* Render::setDistance: set distance */
void Render::setDistance(float distance)
{
    m_distance = distance;
}

/* Render::getDistance: get distance */
float Render::getDistance()
{
    return m_distance;
}

/* Render::setFovy: set fovy */
void Render::setFovy(float fovy)
{
    m_fovy = fovy;
}

/* Render::getFovy: get fovy */
float Render::getFovy()
{
    return m_fovy;
}

/* Render::setShadowMapping: switch shadow mapping */
void Render::setShadowMapping(bool useShadowMapping, int textureSize, bool shadowMappingLightFirst)
{
#ifndef MMDAGENT_DONTUSESHADOWMAP
    if(useShadowMapping) {
        /* enabled */
        if (!m_shadowMapInitialized) {
            /* initialize now */
            initializeShadowMap(textureSize);
            m_shadowMapInitialized = true;
        }
        /* set how to set the comparison result value of R coordinates and texture (depth) value */
        glActiveTextureARB(GL_TEXTURE3_ARB);
        glBindTexture(GL_TEXTURE_2D, m_depthTextureID);
        if (shadowMappingLightFirst) {
            /* when rendering order is light(full) - dark(shadow part), OpenGL should set the shadow part as true */
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
        } else {
            /* when rendering order is dark(full) - light(non-shadow part), OpenGL should set the shadow part as false */
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }
        glDisable(GL_TEXTURE_2D);
        glActiveTextureARB(GL_TEXTURE0_ARB);
    } else {
        /* disabled */
        if (m_shadowMapInitialized) {
            /* disable depth texture unit */
            glActiveTextureARB(GL_TEXTURE3_ARB);
            glDisable(GL_TEXTURE_2D);
            glActiveTextureARB(GL_TEXTURE0_ARB);
        }
    }
#endif /* !MMDAGENT_DONTUSESHADOWMAP */
}

/* Render::getRenderOrder: return rendering order */
void Render::getRenderOrder(int *order, PMDObject *objs, int num)
{
    int i, s;
    btVector3 pos, v;
    
    if (num == 0)
        return;
    
    s = 0;
    for (i = 0; i < num; i++) {
        if (objs[i].isEnable() == false || objs[i].allowMotionFileDrop() == false) continue;
        pos = objs[i].getPMDModel()->getCenterBone()->getTransform()->getOrigin();
        pos = m_transMatrix * pos;
        m_depth[s].dist = pos.z();
        m_depth[s].id = i;
        s++;
    }
    qsort(m_depth, s, sizeof(RenderDepthData), compareDepth);
    for (i = 0; i < s; i++)
        order[i] = m_depth[i].id;
    for (i = 0; i < num; i++)
        if (objs[i].isEnable() == false || objs[i].allowMotionFileDrop() == false)
            order[s++] = i;
    
    for (i = 0; i < num; i++)
        if (objs[i].isEnable() == true)
            objs[i].getPMDModel()->updateMaterialOrder(&m_transMatrix);
}

/* Render::render: render all */
void Render::render(RenderInfo* info)
{
    bool updated;
    
    /* update camera view matrices */
    updated = updateDistance(info->ellapsedTimeForMove);
    updated |= updateTransRotMatrix(info->ellapsedTimeForMove);
    if (m_quarteredForceUpdate || updated == true)
        updateModelViewMatrix();
    if (m_quarteredForceUpdate || updateFovy(info->ellapsedTimeForMove) == true)
        updateProjectionMatrix();
    
    if (isViewMoving() == false)
        m_viewMoveTime = -1.0;
    
    if (info->useShadowMapping)
        renderSceneShadowMap(info);
    else
        renderScene(info);
}

/* Render::pickModel: pick up a model at the screen position */
int Render::pickModel(PMDObject *objs, int num, int x, int y, int *allowDropPicked)
{
#ifdef MMDAGENT_DONTPICKMODEL
    return -1;
#else
    int i;
    
    GLuint selectionBuffer[512];
    GLint viewport[4];
    
    GLint hits;
    GLuint *data;
    GLuint minDepth = 0, minDepthAllowDrop = 0;
    int minID, minIDAllowDrop;
    GLuint depth;
    int id;
    
    /* get current viewport */
    glGetIntegerv(GL_VIEWPORT, viewport);
    /* set selection buffer */
    glSelectBuffer(512, selectionBuffer);
    /* begin selection mode */
    glRenderMode(GL_SELECT);
    /* save projection matrix */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    /* set projection matrix for picking */
    glLoadIdentity();
    /* apply picking matrix */
    gluPickMatrix(x, viewport[3] - y, 15.0, 15.0, viewport);
    /* apply normal projection matrix */
    applyProjectionMatrix();
    /* switch to model view mode */
    glMatrixMode(GL_MODELVIEW);
    /* initialize name buffer */
    glInitNames();
    glPushName(0);
    /* draw models with selection names */
    for (i = 0; i < num; i++) {
        if (objs[i].isEnable() == true) {
            glLoadName(i);
            objs[i].getPMDModel()->renderForPick();
        }
    }
    
    /* restore projection matrix */
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    /* switch to model view mode */
    glMatrixMode(GL_MODELVIEW);
    /* end selection mode and get number of hits */
    hits = glRenderMode(GL_RENDER);
    if (hits == 0) return -1;
    data = &(selectionBuffer[0]);
    minID = -1;
    minIDAllowDrop = -1;
    for (i = 0; i < hits; i++) {
        depth = *(data + 1);
        id = *(data + 3);
        if (minID == -1 || minDepth > depth) {
            minDepth = depth;
            minID = id;
        }
        if (allowDropPicked && objs[id].allowMotionFileDrop()) {
            if (minIDAllowDrop == -1 || minDepthAllowDrop > depth) {
                minDepthAllowDrop = depth;
                minIDAllowDrop = id;
            }
        }
        data += *data + 3;
    }
    if (allowDropPicked)
        *allowDropPicked = minIDAllowDrop;
    
    return minID;
#endif /* MMDAGENT_DONTPICKMODEL */
}

/* Render::updateLight: update light */
void Render::updateLight(bool useMMDLikeCartoon, bool useCartoonRendering, float lightIntensity, const float *lightDirection, const float *lightColor)
{
    float fLightDif[4];
    float fLightSpc[4];
    float fLightAmb[4];
    int i;
    float d, a, s;
    
    if (useMMDLikeCartoon == false) {
        /* MMDAgent original cartoon */
        d = 0.2f;
        a = lightIntensity * 1.5f;
        s = 0.4f;
    } else if (useCartoonRendering) {
        /* like MikuMikuDance */
        d = 0.0f;
        a = lightIntensity * 1.5f;
        s = lightIntensity;
    } else {
        /* no toon */
        d = lightIntensity;
        a = 1.0f;
        s = 1.0f; /* OpenGL default */
    }
    
    for (i = 0; i < 3; i++)
        fLightDif[i] = lightColor[i] * d;
    fLightDif[3] = 1.0f;
    for (i = 0; i < 3; i++)
        fLightAmb[i] = lightColor[i] * a;
    fLightAmb[3] = 1.0f;
    for (i = 0; i < 3; i++)
        fLightSpc[i] = lightColor[i] * s;
    fLightSpc[3] = 1.0f;
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, fLightDif);
    glLightfv(GL_LIGHT0, GL_AMBIENT, fLightAmb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, fLightSpc);
    
    /* update light direction vector */
    m_lightVec = btVector3(btScalar(lightDirection[0]), btScalar(lightDirection[1]), btScalar(lightDirection[2]));
    m_lightVec.normalize();
}

/* Render::updateDepthTextureViewParam: update center and radius information to get required range for shadow mapping */
void Render::updateDepthTextureViewParam(PMDObject *objList, int num)
{
    int i;
    float d, dmax;
    float *r = (float *) malloc(sizeof(float) * num);
    btVector3 *c = new btVector3[num];
    btVector3 cc = btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f));
    
    for (i = 0; i < num; i++) {
        if (objList[i].isEnable() == false)
            continue;
        r[i] = objList[i].getPMDModel()->calculateBoundingSphereRange(&(c[i]));
        cc += c[i];
    }
    cc /= (float) num;
    
    dmax = 0.0f;
    for (i = 0; i < num; i++) {
        if (objList[i].isEnable() == false)
            continue;
        d = cc.distance(c[i]) + r[i];
        if (dmax < d)
            dmax = d;
    }
    
    m_shadowMapAutoViewEyePoint = cc;
    m_shadowMapAutoViewRadius = dmax;
    
    free(r);
    delete [] c;
}

/* Render::getScreenPointPosition: convert screen position to object position */
void Render::getScreenPointPosition(btVector3 *dst, btVector3 *src)
{
    *dst = m_transMatrixInv * (*src);
}

/* Render::getCurrentViewCenterPos: get current view center position */
void Render::getCurrentViewCenterPos(btVector3 *pos)
{
    *pos = m_currentTrans;
}

/* Render::getCurrentViewTransform: get current view transform matrix */
void Render::getCurrentViewTransform(btTransform *tr)
{
    *tr = m_transMatrix;
}

/* Render::getInfoString: store current view parameters to buffer */
void Render::getInfoString(char *buf)
{
    sprintf(buf, "%.2f, %.2f, %.2f | %.2f, %.2f, %.2f | %.2f | %.2f", m_currentTrans.x(), m_currentTrans.y(), m_currentTrans.z(), m_angle.x(), m_angle.y(), m_angle.z(), m_currentDistance, m_currentFovy);
}

/* Render::setDiagonalRendering: set the rendering type and do lazy preparing. (must be called within a valid opengl context) */
void Render::setDiagonalRendering(int method)
{
    if(m_diagonalMethod == method)
        return;
    
    // clean up old rendering resources
    if(m_diagonalMethod == 1)
    {
        // nothing to do...
    }
    else if(m_diagonalMethod == 2)
    {
        // free the diagonal rendering resource
        glDeleteTextures(1, &m_fboColorBuffer);
        glDeleteRenderbuffersOES(1, &m_fboDepthBuffer);
        glDeleteFramebuffersOES(1, &m_fbo);
    }
    
    // prepare
    if(method == 1)
    {
        // nothing to do...?
    }
    else if(method == 2)
    {
        // post effect diagonal redering
        
#define CheckPoint() \
    do{ ALog("Check point (line: %d)", __LINE__); GLenum err; \
      while((err = glGetError()) != GL_NO_ERROR) \
            ALog("glGetError: %x", err); \
    }while(0)
        
        // calculate the target side and POW texture size
        int screenSide = this->getScreenViewportSide();
        float targetSide = (float)screenSide * this->m_diagonalScale;
        int targetTexSide = roundPowerof2(targetSide);
        
        // save side length of the FBO
        m_fboSide = targetTexSide;
        m_fboViewportSide = (GLuint)targetSide;
        
        // caluclate texture uv coord
        GLfloat s = (GLfloat)m_fboViewportSide / (GLfloat)targetTexSide;
        GLfloat uv[] = {
            0, 1-s,     // LB
            s, 1-s,     // RB
            s, 1,       // RT
            0, 1,       // LT
        };
        memcpy(m_fboViewportUV, uv, sizeof(uv));
    
        // allocate the diagonal rendering resource
        
        // fbo_width and fbo_height are the desired width and height of the FBO.
        // For Opengl <= 4.4 or if the GL_ARB_texture_non_power_of_two extension
        // is present, fbo_width and fbo_height can be values other than 2^n for
        // some integer n.
        
        GLsizei fbo_width = targetTexSide;
        GLsizei fbo_height = targetTexSide;
        
        // create color buffer
        GLuint texture_map;
        glGenTextures(1, &texture_map);                 CheckPoint();
        glBindTexture(GL_TEXTURE_2D, texture_map);      CheckPoint();
        
#define RESAMPLE_METHOD GL_LINEAR
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, RESAMPLE_METHOD);   CheckPoint();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, RESAMPLE_METHOD);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);        CheckPoint(); // GL_CLAMP_TO_BORDER
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    // GL_CLAMP_TO_BORDER
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbo_width, fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); CheckPoint();
        
        glBindTexture(GL_TEXTURE_2D, 0);    CheckPoint();
        
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // note: we can only use OES version of functions in ES1.1
        //       the macro provided by glee is broken
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        
        // create depth buffer
        GLuint depthRenderbuffer;
        glGenRenderbuffersOES(1, &depthRenderbuffer);                                              CheckPoint();
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);                                 CheckPoint();
        glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, fbo_width, fbo_height);    CheckPoint();
        
        // create framebuffer
        GLuint framebuffer = -1;
        glGenFramebuffersOES(1, &framebuffer);  CheckPoint();
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);  CheckPoint();
        
        glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_map, 0);            CheckPoint();
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);     CheckPoint();
        
        
        ALog("fbo: %lu, tex: %lu, dep: %lu", framebuffer, texture_map, depthRenderbuffer);
        GLenum status = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);    CheckPoint();
        if (status == GL_FRAMEBUFFER_COMPLETE && framebuffer > 0)
        {
            // good: save framebuffer
            m_fbo = framebuffer;
            m_fboColorBuffer = texture_map;
            m_fboDepthBuffer = depthRenderbuffer;
        }
        else
        {
            // bad
            ALog("Render::setDiagonalRendering(): create framebuffer failed (id=%d)", framebuffer);
            ALog("  -- glCheckFramebufferStatus(GL_FRAMEBUFFER) = %x", status);
            return;
        }
        
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);                    CheckPoint();
        
    }
    
    // save the value
    m_diagonalMethod = method;
}


/* Render::renderQuartered: render quartered (normally or diagonally) */
void Render::renderQuartered(RenderInfo* info)
{
    // clear color (for voice hint?)
    glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], 0.0f);
    
    float targetWidth;      // fbo width
    float targetHeight;     // fbo height
    float targetViewportSide;   // side length of viewport in fbo
    
    float windowWidth = this->getWindowWidth();
    float windowHeight = this->getWindowHeight();
    float windowViewportSide = this->getScreenViewportSide();
    
    // prepare viewport position & size
    if(m_diagonalMethod == 2)
    {
        // diagonal quarterted rendering
        targetViewportSide = m_fboViewportSide;
        targetWidth = m_fboSide;
        targetHeight = m_fboSide;
        
        // apply the framebuffer
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_fbo);
    }
    else if(m_diagonalMethod == 0 || m_diagonalMethod == 1)
    {
        // normal or directly diagonal quarterted rendering
        targetWidth = windowWidth;
        targetHeight = windowHeight;
        targetViewportSide = windowViewportSide;
    }
    
    // render scene
    // (scale factor 1.4 was used here for temporarily scale up the Miku model)
    if(m_diagonalMethod == 1)
    {
        info->modelScale = m_diagonalScale * 1.6f;
        info->modelRaise = 1.0f;
        this->renderQuartered_DirectDiagonal(info, targetWidth, targetHeight, targetViewportSide);
    }
    else
    {
        info->modelScale = 1.6f;
        this->renderQuarteredInternal(info, targetWidth, targetHeight, targetViewportSide);
    }
    
    // post effect for diagonal rendering
    if(m_diagonalMethod == 2)
    {
        // disable the framebuffer
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
        
        // draw post effect texture
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        static GLubyte idx[] = {
            1, 0, 2, 3
        };
        static GLfloat ver[] = {
            -1, -1,  0,  1, -1,  0,
            1,  1,  0, -1,  1,  0,
        };
        
        GLint vp_left = 0;
        GLint vp_bottom = (GLint)(windowHeight - windowViewportSide);
        glViewport(vp_left, vp_bottom, windowViewportSide, windowViewportSide);
        
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        
        // clear matrics
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrthof(-1, 1, -1, 1, -1, 1);
        
        // scale * 45deg-rot
        glScalef(m_diagonalScale, m_diagonalScale, 1.0f);
        glRotatef(-45.0f, 0.0f, 0.0f, 1.0f);
        
        // set color
        glColor4f(1, 1, 1, 1);
        
        // bind texture
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_fboColorBuffer);
        
        // bind verteces
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, ver);
        
        // bind uv coord
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, m_fboViewportUV);
        
        // draw
        glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);
        
        if(glGetError() != GL_NO_ERROR) while(1);
        
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
    }
}

int Render::roundPowerof2(float side)
{
    int r = 1;
    while(r < side)
        r <<= 1;
    return r;
}

void Render::setRenderBackgroundColor(float r, float g, float b)
{
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
}

void Render::renderQuarteredInternal(RenderInfo* info, float targetWidth, float targetHeight, float targetViewportSide)
{
    // clear buffers before render quartered screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // left top corner
    float vpLeft = 0.0f;
    float vpBottom = (targetHeight - targetViewportSide);
    float vpSubSide = targetViewportSide / 2.0f;
    float vpHalfSide = targetViewportSide / 4.0f;
    
    float viewportLB[4][2] =
    {
        { vpLeft + vpHalfSide, vpBottom},               // bottom
        { vpLeft, vpBottom + vpHalfSide},               // left
        { vpLeft + vpHalfSide, vpBottom + vpSubSide},   // top
        { vpLeft + vpSubSide, vpBottom + vpHalfSide},   // right
    };
    
    static float cameraRotation[4] = { 0, -90, -180, -270 };
    static float viewRotation[4] = { 0, -90, -180, -270 };
    
    this->setQuarteredForceUpdate(true);
    
    for(int vp = 0; vp<4; vp++)
    {
        float lbx = viewportLB[vp][0];
        float lby = viewportLB[vp][1];
        glViewport(lbx, lby, vpSubSide, vpSubSide);
        
        this->setQuarteredRotations(cameraRotation[vp], viewRotation[vp]);
        this->render(info);
    }
    
    this->setQuarteredForceUpdate(false);
}

void Render::renderQuartered_DirectDiagonal(RenderInfo* info, float targetWidth, float targetHeight, float targetViewportSide)
{
    // clear buffers before render quartered screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    float vpLeft = 0.0f;
    float vpBottom = (targetHeight - targetViewportSide);
    float vpSubSide = targetViewportSide / 2.0f;
    
    float viewportLB[4][2] =
    {
        { vpLeft, vpBottom },                           // LB
        { vpLeft, vpBottom + vpSubSide },               // LT
        { vpLeft + vpSubSide, vpBottom + vpSubSide },   // RT
        { vpLeft + vpSubSide, vpBottom },               // RB
    };
    
    // viewports:
    //  [L][B]
    //  [F][R]
    
    
    static float cameraRotation[4] = { 0, -90, -180, -270 };
    static float viewRotation[4] = { 0-45, -90-45, -180-45, -270-45 };
    
    this->setQuarteredForceUpdate(true);
    
    for(int vp = 0; vp<4; vp++)
    {
        float lbx = viewportLB[vp][0];
        float lby = viewportLB[vp][1];
        glViewport(lbx, lby, vpSubSide, vpSubSide);
        
        this->setQuarteredRotations(cameraRotation[vp], viewRotation[vp]);
        this->render(info);
    }
    
    this->setQuarteredForceUpdate(false);
}

float Render::demoModelTrans[3] = {0,0,0};
