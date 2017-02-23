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

#include "MMDFiles.h"

/* PMDMaterial::initialize: initialize material */
void PMDMaterial::initialize()
{
   int i;

   for (i = 0; i < 3; i++) {
      m_diffuse[i] = 0.0f;
      m_ambient[i] = 0.0f;
      m_avgcol[i] = 0.0f;
      m_specular[i] = 0.0f;
   }
   m_alpha = 0.0f;
   m_shiness = 0.0f;
   m_numSurface = 0;
   m_toonID = 0;
   m_edgeFlag = false;
   m_texture = NULL;
   m_additionalTexture = NULL;
   m_surfaceList = 0;
   m_centerVertexIndex = 0;
   m_centerVertexRadius = 0.0f;
}

/* PMDMaterial::clear: free material */
void PMDMaterial::clear()
{
   /* actual texture data will be released inside textureLoader, so just reset pointer here */
   initialize();
}

/* PMDMaterial:: constructor */
PMDMaterial::PMDMaterial()
{
   initialize();
}

/* ~PMDMaterial:: destructor */
PMDMaterial::~PMDMaterial()
{
   clear();
}

/* PMDMaterial::setup: initialize and setup material */
bool PMDMaterial::setup(PMDFile_Material *m, PMDTextureLoader *textureLoader, const char *dir, unsigned int indices, btVector3 *vertices, unsigned short *surfaces)
{
   int i, len;
   char *p;
   char buf[MMDFILES_MAXBUFLEN];
   bool ret = true;
   char name[21];
   unsigned int j;
   float f[3], d, tmp = 0.0f;
   unsigned short *surface;

   clear();

   /* colors */
   for (i = 0; i < 3; i++) {
      m_diffuse[i] = m->diffuse[i];
      m_ambient[i] = m->ambient[i];
      /* calculate color for toon rendering */
      m_avgcol[i] = m_diffuse[i] * 0.5f + m_ambient[i];
      if (m_avgcol[i] > 1.0f)
         m_avgcol[i] = 1.0f;
      m_specular[i] = m->specular[i];
   }
   m_alpha = m->alpha;
   m_shiness = m->shiness;

   /* number of surface indices whose material should be assigned by this */
   m_numSurface = m->numSurfaceIndex;

   /* toon texture ID */
   if (m->toonID == 0xff)
      m_toonID = 0;
   else
      m_toonID = m->toonID + 1;
   /* edge drawing flag */
   m_edgeFlag = m->edgeFlag ? true : false;

   /* load model texture */
   strncpy(name, m->textureFile, 20);
   name[20] = '\0';
   if (MMDFiles_strlen(name) > 0) {
      p = strchr(name, '*');
      if (p) {
         /* has extra sphere map */
         len = p - &(name[0]);
         sprintf(buf, "%s%c", dir, MMDFILES_DIRSEPARATOR);
         strncat(buf, name, len);
         m_texture = textureLoader->load(buf);
         if (!m_texture)
            ret = false;
         sprintf(buf, "%s%c%s", dir, MMDFILES_DIRSEPARATOR, p + 1);
         m_additionalTexture = textureLoader->load(buf);
         if (!m_additionalTexture)
            ret = false;
      } else {
         sprintf(buf, "%s%c%s", dir, MMDFILES_DIRSEPARATOR, name);
         m_texture = textureLoader->load(buf);
         if (!m_texture)
            ret = false;
      }
   }

   /* store pointer to surface */
   m_surfaceList = indices;

   /* calculate for center vertex */
   surface = &(surfaces[indices]);
   for (i = 0; i < 3; i++)
      f[i] = 0.0f;
   for (j = 0; j < m_numSurface; j++) {
      f[0] += vertices[surface[j]].getX();
      f[1] += vertices[surface[j]].getY();
      f[2] += vertices[surface[j]].getZ();
   }
   for (i = 0; i < 3; i++)
      f[i] /= (float)m_numSurface;
   for (j = 0; j < m_numSurface; j++) {
      d = (f[0] - vertices[surface[j]].getX()) * (f[0] - vertices[surface[j]].getX())
          + (f[1] - vertices[surface[j]].getY()) * (f[1] - vertices[surface[j]].getY())
          + (f[2] - vertices[surface[j]].getZ()) * (f[2] - vertices[surface[j]].getZ());
      if (j == 0 || tmp > d) {
         tmp = d;
         m_centerVertexIndex = surface[j];
      }
   }
   /* get maximum radius from the center vertex */
   for (j = 0; j < m_numSurface; j++) {
      d = vertices[m_centerVertexIndex].distance2(vertices[surface[j]]);
      if (j == 0 || m_centerVertexRadius < d) {
         m_centerVertexRadius = d;
      }
   }
   m_centerVertexRadius = sqrt(m_centerVertexRadius);

   return ret;
}

/* PMDMaterial::hasSingleSphereMap: return if it has single sphere maps */
bool PMDMaterial::hasSingleSphereMap()
{
   if (m_texture && m_texture->isSphereMap() && m_additionalTexture == NULL)
      return true;
   else
      return false;
}

/* PMDMaterial::hasMultipleSphereMap: return if it has multiple sphere map */
bool PMDMaterial::hasMultipleSphereMap()
{
   if (m_additionalTexture)
      return true;
   else
      return false;
}

/* PMDMaterial::copyDiffuse: get diffuse colors */
void PMDMaterial::copyDiffuse(float *c)
{
   int i;

   for (i = 0; i < 3; i++)
      c[i] = m_diffuse[i];
}

/* PMDMaterial::copyAvgcol: get average colors of diffuse and ambient */
void PMDMaterial::copyAvgcol(float *c)
{
   int i;

   for (i = 0; i < 3; i++)
      c[i] = m_avgcol[i];
}

/* PMDMaterial::copyAmbient: get ambient colors */
void PMDMaterial::copyAmbient(float *c)
{
   int i;

   for (i = 0; i < 3; i++)
      c[i] = m_ambient[i];
}

/* PMDMaterial::copySpecular: get specular colors */
void PMDMaterial::copySpecular(float *c)
{
   int i;

   for (i = 0; i < 3; i++)
      c[i] = m_specular[i];
}

/* PMDMaterial::getAlpha: get alpha */
float PMDMaterial::getAlpha()
{
   return m_alpha;
}

/* PMDMaterial::getShiness: get shiness */
float PMDMaterial::getShiness()
{
   return m_shiness;
}

/* PMDMaterial::getNumSurface: get number of surface */
unsigned int PMDMaterial::getNumSurface()
{
   return m_numSurface;
}

/* PMDMaterial::getToonID: get toon index */
unsigned char PMDMaterial::getToonID()
{
   return m_toonID;
}

/* PMDMaterial::getEdgeFlag: get edge flag */
bool PMDMaterial::getEdgeFlag()
{
   return m_edgeFlag;
}

/* PMDMaterial::getTexture: get texture */
PMDTexture *PMDMaterial::getTexture()
{
   return m_texture;
}

/* PMDMaterial::getAdditionalTexture: get additional sphere map */
PMDTexture *PMDMaterial::getAdditionalTexture()
{
   return m_additionalTexture;
}

/* PMDMaterial::getCenterPositionIndex: get center position index */
unsigned int PMDMaterial::getCenterPositionIndex()
{
   return m_centerVertexIndex;
}

/* PMDMaterial::getCenterVertexRadius: get maximum radius from center position index */
float PMDMaterial::getCenterVertexRadius()
{
   return m_centerVertexRadius;
}

/* PMDMaterial::getSurfaceListIndex: get surface list index */
unsigned int PMDMaterial::getSurfaceListIndex()
{
   return m_surfaceList;
}

