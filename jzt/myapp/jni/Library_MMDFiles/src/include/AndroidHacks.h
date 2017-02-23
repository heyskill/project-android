//
//  AndroidHacks.h
//  Library_MMDFiles
//
//  Created by h4x on 14/12/16.
//  Copyright (c) 2014年 h4x. All rights reserved.
//

#ifndef Library_MMDFiles_AndroidHacks_h
#define Library_MMDFiles_AndroidHacks_h

#if defined(__ANDROID__)
#   define glBufferData_safe(target, size, data, usage) \
        do { \
            if(data) glBufferData(target, size, data, usage); \
            else { \
                void* tmpbuf = malloc((size_t)size); \
                glBufferData(target, size, tmpbuf, usage); \
                free(tmpbuf); \
            } \
        } while(0)
#else
#   define glBufferData_safe(target, size, data, usage) \
           glBufferData(target, size, data, usage)
#endif

#endif
