/*
 * Copyright (c) 2008 - 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and proprietary
 * rights in and to this software, related documentation and any modifications thereto.
 * Any use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation is strictly
 * prohibited.
 *
 * TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED *AS IS*
 * AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS BE LIABLE FOR ANY
 * SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT
 * LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
 * BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR
 * INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES
 */


/******************************************************************************\
 *
 * Contains declarations used internally.  This file is never to be released.
 *
\******************************************************************************/

#ifndef __optix_optix_declarations_private_h__
#define __optix_optix_declarations_private_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  RT_BUFFER_PARTITIONED_INTERNAL      = 1u << 16,
  RT_BUFFER_PINNED_INTERNAL           = 1u << 17,
  RT_BUFFER_WRITECOMBINED_INTERNAL    = 1u << 18,
  RT_BUFFER_DEVICE_ONLY_INTERNAL      = 1u << 19,
  RT_BUFFER_FORCE_ZERO_COPY           = 1u << 20,
  RT_BUFFER_LAYERED_RESERVED          = 1u << 21, // reserved here, declared in optix_declarations.h
  RT_BUFFER_CUBEMAP_RESERVED          = 1u << 22, // reserved here, declared in optix_declarations.h 
  RT_BUFFER_INTERNAL_PREFER_TEX_HEAP  = 1u << 23, /* For buffers wanting to use texture heap */
} RTbufferflag_internal;

namespace optix {
  enum ObjectStorageType {
    OBJECT_STORAGE_CONSTANT,
    OBJECT_STORAGE_SHARED,
    OBJECT_STORAGE_GLOBAL,
    OBJECT_STORAGE_LINEAR_TEXTURE,
    OBJECT_STORAGE_BLOCKED_TEXTURE
  };
}

#define RT_CONTEXT_INTERNAL_MAX_CUBIN_CACHE 128                                  // Precompiled CUBINS
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_ABI_STATUS                       0x2000000 /* sizeof(int) */
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_CALLID                           0x2000001 /* sizeof(long long) */
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_ACCELERATION_BAKE_CHILD_POINTERS 0x2000002 /* sizeof(int) */
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_COMPARE_CUBIN_HASH               0x2000003 /* sizeof(int) */
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_ALLOW_32BITBUFFER_OPTIMIZATION   0x2000004 /* sizeof(int) */
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_LIMIT_RESIDENT_DEVICE_MEMORY     0x2000005 /* sizeof(size_t) */
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_SUBFRAME_INDEX                   0x2000006 /* sizeof(int) */
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_CUBIN_HASH                       0x3000000 /* 2*sizeof(long long) */
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_CUBIN_SMVERSION  (0x3000000 + RT_CONTEXT_INTERNAL_MAX_CUBIN_CACHE)   /* sizeof(int) */
#define RT_CONTEXT_INTERNAL_ATTRIBUTE_CUBIN_DATA       (0x3000000 + 2*RT_CONTEXT_INTERNAL_MAX_CUBIN_CACHE) /* chars */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __optix_optix_declarations_private_h__ */
