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

#define LIPSYNC_CONFIGFILE          "lip.txt"
#define LIPSYNC_MAXBUFLEN           2048
#define LIPSYNC_MOTIONNAME          "LipSync" /* motion name of lip sync */
#define LIPSYNC_INTERPOLATIONMARGIN 2
#define LIPSYNC_INTERPOLATIONRATE   0.8f
#define LIPSYNC_SEPARATOR           ","

/* LipKeyFrame: key frame for lip motion */
typedef struct _LipKeyFrame {
   int phone;
   int duration;
   float rate;
   struct _LipKeyFrame *next;
} LipKeyFrame;

/* LipSync: lipsync */
class LipSync
{
private:

   int m_numMotion; /* number of expression */
   char **m_motion;

   int m_numPhone; /* number of phoneme */
   char **m_phone;
   float **m_blendRate;

   /* initialize: initialize lipsync */
   void initialize();

   /* clear: free lipsync */
   void clear();

public:

   /* LipSync: constructor */
   LipSync();

   /* ~LipSync: destructor */
   ~LipSync();

   /* load: initialize and load lip setting */
   bool load(const char *file);

   /* createMotion: create motion from phoneme sequence */
   bool createMotion(const char *str, unsigned char **rawData, unsigned int *rawSize);
};
