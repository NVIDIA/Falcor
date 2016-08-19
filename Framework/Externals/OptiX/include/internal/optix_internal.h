
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

#ifndef __optix_optix_internal_h__
#define __optix_optix_internal_h__

#include "optix_datatypes.h"
#include "optix_defines.h"
#include "../optix_sizet.h"

#include <cstdio>

struct rtObject;

namespace optix {

#if (defined(__CUDACC__) && (__CUDA_ARCH__ > 0))
#if ((CUDA_VERSION >= 4010) || (CUDART_VERSION >= 4010)) && __CUDA_ARCH__ >= 200
  __forceinline__
#else
  __noinline__
#endif
  __device__ void
  rt_undefined_use(int)
  {
  }
#else
  void rt_undefined_use(int);
#endif

#if (defined(__CUDACC__) && (__CUDA_ARCH__ > 0))
#if ((CUDA_VERSION >= 4010) || (CUDART_VERSION >= 4010)) && __CUDA_ARCH__ >= 200
  __forceinline__
#else
  __noinline__
#endif
  __device__ void
  rt_undefined_use64(unsigned long long)
  {
  }
#else
  void rt_undefined_use64(int);
#endif

  static __forceinline__ __device__  uint3 rt_texture_get_size_id(int tex)
  {
    unsigned int u0, u1, u2;

    asm volatile("call (%0, %1, %2), _rt_texture_get_size_id, (%3);" :
                 "=r"(u0), "=r"(u1), "=r"(u2) : "r"(tex) : );

    rt_undefined_use((int)u0);
    rt_undefined_use((int)u1);
    rt_undefined_use((int)u2);

    return make_uint3(u0, u1, u2);
  }
   
   static __forceinline__ __device__ float4 rt_texture_get_gather_id(int tex, float x, float y, int comp)
  {
    float f0, f1, f2, f3;
    int dim = 2;

    asm volatile("call (%0, %1, %2, %3), _rt_texture_get_gather_id, (%4, %5, %6, %7, %8);" :
                 "=f"(f0), "=f"(f1), "=f"(f2), "=f"(f3) :
                 "r"(tex), "r"(dim), "f"(x), "f"(y), "r"(comp) :
                 );

    rt_undefined_use((int)f0);
    rt_undefined_use((int)f1);
    rt_undefined_use((int)f2);
    rt_undefined_use((int)f3);

    return make_float4(f0, f1, f2, f3);
  }
   
  static __forceinline__ __device__ float4 rt_texture_get_base_id(int tex, int dim, float x, float y, float z, int layer)
  {
    float f0, f1, f2, f3;

    asm volatile("call (%0, %1, %2, %3), _rt_texture_get_base_id, (%4, %5, %6, %7, %8, %9);" :
                 "=f"(f0), "=f"(f1), "=f"(f2), "=f"(f3) :
                 "r"(tex), "r"(dim), "f"(x), "f"(y), "f"(z), "r"(layer) :
                 );

    rt_undefined_use((int)f0);
    rt_undefined_use((int)f1);
    rt_undefined_use((int)f2);
    rt_undefined_use((int)f3);

    return make_float4(f0, f1, f2, f3);
  }

  static __forceinline__ __device__ float4
    rt_texture_get_level_id(int tex, int dim, float x, float y, float z, int layer, float level)
  {
    float f0, f1, f2, f3;

    asm volatile("call (%0, %1, %2, %3), _rt_texture_get_level_id, (%4, %5, %6, %7, %8, %9, %10);" :
                 "=f"(f0), "=f"(f1), "=f"(f2), "=f"(f3) :
                 "r"(tex), "r"(dim), "f"(x), "f"(y), "f"(z), "r"(layer), "f"(level) :
                 );

    rt_undefined_use((int)f0);
    rt_undefined_use((int)f1);
    rt_undefined_use((int)f2);
    rt_undefined_use((int)f3);

    return make_float4(f0, f1, f2, f3);
  }
   
  static __forceinline__ __device__ float4 rt_texture_get_grad_id(int tex, int dim, float x, float y, float z, int layer,
                                                  float dPdx_x, float dPdx_y, float dPdx_z, float dPdy_x, float dPdy_y, float dPdy_z)
  {
    float f0, f1, f2, f3;

    asm volatile("call (%0, %1, %2, %3), _rt_texture_get_grad_id, (%4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15);" :
                 "=f"(f0), "=f"(f1), "=f"(f2), "=f"(f3) :
                 "r"(tex), "r"(dim), "f"(x), "f"(y), "f"(z), "r"(layer), "f"(dPdx_x), "f"(dPdx_y), "f"(dPdx_z), "f"(dPdy_x), "f"(dPdy_y), "f"(dPdy_z) :
                 );

    rt_undefined_use((int)f0);
    rt_undefined_use((int)f1);
    rt_undefined_use((int)f2);
    rt_undefined_use((int)f3);

    return make_float4(f0, f1, f2, f3);
  }

  static __forceinline__ __device__ float4 rt_texture_get_f_id(int tex, int dim, float x, float y, float z, float w)
  {
    float f0, f1, f2, f3;

    asm volatile("call (%0, %1, %2, %3), _rt_texture_get_f_id, (%4, %5, %6, %7, %8, %9);" :
                 "=f"(f0), "=f"(f1), "=f"(f2), "=f"(f3) :
                 "r"(tex), "r"(dim), "f"(x), "f"(y), "f"(z), "f"(w) :
                 );

    rt_undefined_use((int)f0);
    rt_undefined_use((int)f1);
    rt_undefined_use((int)f2);
    rt_undefined_use((int)f3);

    return make_float4(f0, f1, f2, f3);
  }

  static __forceinline__ __device__ int4 rt_texture_get_i_id(int tex, int dim, float x, float y, float z, float w)
  {
    int i0, i1, i2, i3;

    asm volatile("call (%0, %1, %2, %3), _rt_texture_get_i_id, (%4, %5, %6, %7, %8, %9);" :
                 "=r"(i0), "=r"(i1), "=r"(i2), "=r"(i3) :
                 "r"(tex), "r"(dim), "f"(x), "f"(y), "f"(z), "f"(w) :
                 );

    rt_undefined_use((int)i0);
    rt_undefined_use((int)i1);
    rt_undefined_use((int)i2);
    rt_undefined_use((int)i3);

    return make_int4(i0, i1, i2, i3);
  }

  static __forceinline__ __device__ uint4 rt_texture_get_u_id(int tex, int dim, float x, float y, float z, float w)
  {
    unsigned int u0, u1, u2, u3;

    asm volatile("call (%0, %1, %2, %3), _rt_texture_get_u_id, (%4, %5, %6, %7, %8, %9);" :
                 "=r"(u0), "=r"(u1), "=r"(u2), "=r"(u3) :
                 "r"(tex), "r"(dim), "f"(x), "f"(y), "f"(z), "f"(w) :
                 );

    rt_undefined_use((int)u0);
    rt_undefined_use((int)u1);
    rt_undefined_use((int)u2);
    rt_undefined_use((int)u3);

    return make_uint4(u0, u1, u2, u3);
  }

  static __forceinline__ __device__ float4 rt_texture_get_fetch_id(int tex, int dim, int x, int y, int z, int w)
  {
    float f0, f1, f2, f3;

    asm volatile("call (%0, %1, %2, %3), _rt_texture_get_fetch_id, (%4, %5, %6, %7, %8, %9);" :
                 "=f"(f0), "=f"(f1), "=f"(f2), "=f"(f3) :
                 "r"(tex), "r"(dim), "r"(x), "r"(y), "r"(z), "r"(w) :
                 );

    rt_undefined_use((int)f0);
    rt_undefined_use((int)f1);
    rt_undefined_use((int)f2);
    rt_undefined_use((int)f3);

    return make_float4(f0, f1, f2, f3);
  }

  static __forceinline__ __device__ void* rt_buffer_get(void* buffer, unsigned int dim, unsigned int element_size,
                                               size_t i0_in, size_t i1_in, size_t i2_in, size_t i3_in)
  {
    optix::optix_size_t i0, i1, i2, i3;
    i0 = i0_in;
    i1 = i1_in;
    i2 = i2_in;
    i3 = i3_in;
    void* tmp;
    asm volatile("call (%0), _rt_buffer_get" OPTIX_BITNESS_SUFFIX ", (%1, %2, %3, %4, %5, %6, %7);" :
                 "=" OPTIX_ASM_PTR(tmp) :
                 OPTIX_ASM_PTR(buffer), "r"(dim), "r"(element_size),
                 OPTIX_ASM_SIZE_T(i0), OPTIX_ASM_SIZE_T(i1), OPTIX_ASM_SIZE_T(i2), OPTIX_ASM_SIZE_T(i3) :
                 );

#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
    rt_undefined_use64((unsigned long long)tmp);
#else
    rt_undefined_use((int)tmp);
#endif
    return tmp;
  }

  static __forceinline__ __device__ void* rt_buffer_get_id(int id, unsigned int dim, unsigned int element_size,
                                                  size_t i0_in, size_t i1_in, size_t i2_in, size_t i3_in)
  {
    optix::optix_size_t i0, i1, i2, i3;
    i0 = i0_in;
    i1 = i1_in;
    i2 = i2_in;
    i3 = i3_in;
    void* tmp;
    asm volatile("call (%0), _rt_buffer_get_id" OPTIX_BITNESS_SUFFIX ", (%1, %2, %3, %4, %5, %6, %7);" :
                 "=" OPTIX_ASM_PTR(tmp) :
                 "r"(id), "r"(dim), "r"(element_size),
                 OPTIX_ASM_SIZE_T(i0), OPTIX_ASM_SIZE_T(i1), OPTIX_ASM_SIZE_T(i2), OPTIX_ASM_SIZE_T(i3) :
                 );

#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
    rt_undefined_use64((unsigned long long)tmp);
#else
    rt_undefined_use((int)tmp);
#endif
    return tmp;
  }

  static __forceinline__ __device__ size_t4 rt_buffer_get_size(const void* buffer, unsigned int dim, unsigned int element_size)
  {
    optix::optix_size_t d0, d1, d2, d3;
    asm volatile("call (%0, %1, %2, %3), _rt_buffer_get_size" OPTIX_BITNESS_SUFFIX ", (%4, %5, %6);" :
                 "=" OPTIX_ASM_SIZE_T(d0), "=" OPTIX_ASM_SIZE_T(d1), "=" OPTIX_ASM_SIZE_T(d2), "=" OPTIX_ASM_SIZE_T(d3) :
                 OPTIX_ASM_PTR(buffer), "r"(dim), "r"(element_size) :
                 );

    return make_size_t4(d0, d1, d2, d3);
  }

  static __forceinline__ __device__ size_t4 rt_buffer_get_size_id(int id, unsigned int dim, unsigned int element_size)
  {
    optix::optix_size_t d0, d1, d2, d3;
    asm volatile("call (%0, %1, %2, %3), _rt_buffer_get_id_size" OPTIX_BITNESS_SUFFIX ", (%4, %5, %6);" :
                 "=" OPTIX_ASM_SIZE_T(d0), "=" OPTIX_ASM_SIZE_T(d1), "=" OPTIX_ASM_SIZE_T(d2), "=" OPTIX_ASM_SIZE_T(d3) :
                 "r"(id), "r"(dim), "r"(element_size) :
                 );

    return make_size_t4(d0, d1, d2, d3);
  }

  static __forceinline__ __device__ void* rt_callable_program_from_id(int id)
  {
    void* tmp;
    asm volatile("call (%0), _rt_callable_program_from_id" OPTIX_BITNESS_SUFFIX ", (%1);" :
                 "=" OPTIX_ASM_PTR(tmp) :
                 "r"(id):
                 );

#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
    rt_undefined_use64((unsigned long long)tmp);
#else
    rt_undefined_use((int)tmp);
#endif
    return tmp;
  }

  static __forceinline__ __device__ void rt_trace(unsigned int group, float3 origin, float3 direction, unsigned int ray_type,
                                                  float tmin, float tmax, void* prd, unsigned int prd_size)
  {
    float ox = origin.x, oy = origin.y, oz = origin.z;
    float dx = direction.x, dy = direction.y, dz = direction.z;
#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
    rt_undefined_use64((unsigned long long)prd);
#else
    rt_undefined_use((int)prd);
#endif
    asm volatile("call _rt_trace" OPTIX_BITNESS_SUFFIX ", (%0, %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11);" :
                 /* no return value */ :
                 "r"(group), "f"(ox), "f"(oy), "f"(oz), "f"(dx), "f"(dy), "f"(dz),
                 "r"(ray_type), "f"(tmin), "f"(tmax), OPTIX_ASM_PTR(prd), "r"(prd_size) :
                 );
  }

  static __forceinline__ __device__ bool rt_potential_intersection(float t)
  {
    int ret;
    asm volatile("call (%0), _rt_potential_intersection, (%1);" :
                 "=r"(ret) :
                 "f"(t):
                 );

    return ret;
  }

  static __forceinline__ __device__ bool rt_report_intersection(unsigned int matlIndex)
  {
    int ret;
    asm volatile("call (%0), _rt_report_intersection, (%1);" :
                 "=r"(ret) :
                 "r"(matlIndex) :
                 );

    return ret;
  }

  static __forceinline__ __device__ void rt_ignore_intersection()
  {
    asm volatile("call _rt_ignore_intersection, ();");
  }

  static __forceinline__ __device__ void rt_terminate_ray()
  {
    asm volatile("call _rt_terminate_ray, ();");
  }

  static __forceinline__ __device__ void rt_intersect_child(unsigned int index)
  {
    asm volatile("call _rt_intersect_child, (%0);" :
                 /* no return value */ :
                 "r"(index) :
                 );
  }

  static __forceinline__ __device__ float3 rt_transform_point( RTtransformkind kind, const float3& p )
  {

    float f0, f1, f2, f3;
    asm volatile("call (%0, %1, %2, %3), _rt_transform_tuple, (%4, %5, %6, %7, %8);" :
                 "=f"(f0), "=f"(f1), "=f"(f2), "=f"(f3) :
                 "r"(kind), "f"(p.x), "f"(p.y), "f"(p.z), "f"(1.0f) :
                 );

    rt_undefined_use((int)f0);
    rt_undefined_use((int)f1);
    rt_undefined_use((int)f2);
    rt_undefined_use((int)f3);
    return make_float3( f0/f3, f1/f3, f2/f3 );

  }

  static __forceinline__ __device__ float3 rt_transform_vector( RTtransformkind kind, const float3& v )
  {
    float f0, f1, f2, f3;
    asm volatile("call (%0, %1, %2, %3), _rt_transform_tuple, (%4, %5, %6, %7, %8);" :
                 "=f"(f0), "=f"(f1), "=f"(f2), "=f"(f3) :
                 "r"(kind), "f"(v.x), "f"(v.y), "f"(v.z), "f"(0.0f) :
                 );

    rt_undefined_use((int)f0);
    rt_undefined_use((int)f1);
    rt_undefined_use((int)f2);
    rt_undefined_use((int)f3);
    f3 = f3;
    return make_float3( f0, f1, f2 );
  }

  static __forceinline__ __device__ float3 rt_transform_normal( RTtransformkind kind, const float3& n )
  {
    float f0, f1, f2, f3;
    asm volatile("call (%0, %1, %2, %3), _rt_transform_tuple, (%4, %5, %6, %7, %8);" :
                 "=f"(f0), "=f"(f1), "=f"(f2), "=f"(f3) :
                 "r"(kind | RT_INTERNAL_INVERSE_TRANSPOSE ), "f"(n.x), "f"(n.y), "f"(n.z), "f"(0.0f) :
                 );
 
    rt_undefined_use((int)f0);
    rt_undefined_use((int)f1);
    rt_undefined_use((int)f2);
    rt_undefined_use((int)f3);
    f3 = f3;
    return make_float3( f0, f1, f2 );
  }

  static __forceinline__ __device__ void rt_get_transform( RTtransformkind kind, float matrix[16] )
  {
    asm volatile("call (%0, %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15), _rt_get_transform, (%16);" :
                 "=f"(matrix[ 0]), "=f"(matrix[ 1]), "=f"(matrix[ 2]), "=f"(matrix[ 3]),
                 "=f"(matrix[ 4]), "=f"(matrix[ 5]), "=f"(matrix[ 6]), "=f"(matrix[ 7]),
                 "=f"(matrix[ 8]), "=f"(matrix[ 9]), "=f"(matrix[10]), "=f"(matrix[11]),
                 "=f"(matrix[12]), "=f"(matrix[13]), "=f"(matrix[14]), "=f"(matrix[15]) :
                 "r"( kind ) :
                 );
  }

  static __forceinline__ __device__ void rt_throw( unsigned int code )
  {
    asm volatile("call _rt_throw, (%0);" :
                 /* no return value */ :
                 "r"(code) :
                 );
  }

  static __forceinline__ __device__ unsigned int rt_get_exception_code()
  {
    unsigned int result;
    asm volatile("call (%0), _rt_get_exception_code, ();" :
                 "=r"(result) :
                 );

    return result;
  }

  /*
     Printing
  */

  static __forceinline__ __device__ int rt_print_active()
  {
    int ret;
    asm volatile("call (%0), _rt_print_active, ();" :
                 "=r"(ret) :
                 :
                 );
    return ret;
  }
  
  #define _RT_PRINT_ACTIVE()                                              \
    if( !optix::rt_print_active() )                                       \
      return;                                                             \


} /* end namespace optix */

#endif /* __optix_optix_internal_h__ */
