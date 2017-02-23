//
//  AndroidLogUtils.h
//  Library_MMDAgent
//
//  Created by h4x on 14/12/16.
//  Copyright (c) 2014年 h4x. All rights reserved.
//

#ifndef Library_MMDAgent_AndroidLogUtils_h
#define Library_MMDAgent_AndroidLogUtils_h


#if defined(__ANDROID__)

#include <android/log.h>
#   define ALOG_TAG "MMDAgentLog"
#   define ALog(...) _ALog(__VA_ARGS__)

static inline void vALog(const char* format, va_list ap)
{
    __android_log_vprint(ANDROID_LOG_INFO, ALOG_TAG, format, ap);
}

static inline void _ALog(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vALog(format, ap);
    va_end(ap);
}

#else
#   define ALog(...)
#endif


#endif
