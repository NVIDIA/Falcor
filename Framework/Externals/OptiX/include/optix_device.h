
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
 
 /**
 * @file   optix_device.h
 * @author NVIDIA Corporation
 * @brief  OptiX public API
 *
 * OptiX public API Reference - Host/Device side
 */

/******************************************************************************\
 * optix_cuda.h
 *
 * This file provides the nvcc interface for generating PTX that the OptiX is
 * capable of parsing and weaving into the final kernel.  This is included by
 * optix.h automatically if compiling device code.  It can be included explicitly
 * in host code if desired.
 *
\******************************************************************************/

#ifndef __optix_optix_cuda__internal_h__
#define __optix_optix_cuda__internal_h__

#include "internal/optix_datatypes.h"
#include "internal/optix_declarations.h"
#include "internal/optix_internal.h"

/*
  Augment vector types
*/

namespace optix {

  template<typename T, int Dim> struct VectorTypes {};
  template<> struct VectorTypes<int, 1> {
    typedef int Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_int(s); }
  };
  template<> struct VectorTypes<int, 2> {
    typedef int2 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_int2(s); }
  };
  template<> struct VectorTypes<int, 3> {
    typedef int3 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_int3(s); }
  };
  template<> struct VectorTypes<int, 4> {
    typedef int4 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_int4(s); }
  };
  template<> struct VectorTypes<unsigned int, 1> {
    typedef unsigned int Type;
    static __device__ __forceinline__
      Type make(unsigned int s) { return s; }
    template<class S> static __device__ __forceinline__
      Type make(S s) { return (unsigned int)s.x; }
  };
  template<> struct VectorTypes<unsigned int, 2> {
    typedef uint2 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_uint2(s); }
  };
  template<> struct VectorTypes<unsigned int, 3> {
    typedef uint3 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_uint3(s); }
  };
  template<> struct VectorTypes<unsigned int, 4> {
    typedef uint4 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_uint4(s); }
  };
  template<> struct VectorTypes<float, 1> {
    typedef float Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_float(s); }
  };
  template<> struct VectorTypes<float, 2> {
    typedef float2 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_float2(s); }
  };
  template<> struct VectorTypes<float, 3> {
    typedef float3 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_float3(s); }
  };
  template<> struct VectorTypes<float, 4> {
    typedef float4 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_float4(s); }
  };

#if defined(__APPLE__) || defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
  template<> struct VectorTypes<size_t, 1> {
    typedef size_t Type;
    static __device__ __forceinline__
      Type make(unsigned int s) { return s; }
    template<class S> static __device__ __forceinline__
      Type make(S s) { return (unsigned int)s.x; }
  };
  template<> struct VectorTypes<size_t, 2> {
    typedef size_t2 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_size_t2(s); }
  };
  template<> struct VectorTypes<size_t, 3> {
    typedef size_t3 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_size_t3(s); }
  };
  template<> struct VectorTypes<size_t, 4> {
    typedef size_t4 Type;
    template<class S> static __device__ __forceinline__
      Type make(S s) { return make_size_t4(s); }
  };
#endif
}

/*
   Variables
*/

/**
  * @brief Opaque handle to a OptiX object
  * 
  * @ingroup CUDACTypes
  * 
  * <B>Description</B>
  *
  * @ref rtObject is an opaque handle to an OptiX object of any type. To set or query
  * the variable value, use @ref rtVariableSetObject and @ref rtVariableGetObject.
  *
  * Depending on how exacly the variable is used, only
  * certain concrete types may make sense. For example, when used as an argument
  * to @ref rtTrace, the variable must be set to any OptiX type of @ref RTgroup,
  * @ref RTselector, @ref RTgeometrygroup, or @ref RTtransform.
  *
  * Note that for certain OptiX types, there are more specialized handles available
  * to access a variable. For example, to access an OptiX object of type @ref RTtexturesampler,
  * a handle of type @ref rtTextureSampler provides more functionality than
  * one of the generic type @ref rtObject.
  *
  * <B>History</B>
  * 
  * @ref rtObject was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtVariableSetObject,
  * @ref rtVariableGetObject,
  * @ref rtTrace,
  * @ref rtTextureSampler,
  * @ref rtBuffer
  * 
  */
struct rtObject {
protected:
  unsigned int handle;
  /* Bogus use of handle to quiet warnings from compilers that warn about unused private
   * data members. */
  void never_call() { handle = 0; }
};

/**
  * @brief Variable declaration
  * 
  * @ingroup CUDACDeclarations
  * 
  * <B>Description</B>
  * 
  * @ref rtDeclareVariable declares variable \a name of the specified
  * \a type.  By default, the variable name will be matched against a
  * variable declared on the API object using the lookup hierarchy for the
  * current program.  Using the semanticName, this variable can be bound
  * to internal state, to the payload associated with a ray, or to
  * attributes that are communicated between intersection and material
  * programs.  An additional optional annotation can be used to associate
  * application-specific metadata with the variable as well.
  *
  * \a type may be a primitive type or a user-defined struct (See
  * @ref rtVariableSetUserData).  Except for the ray
  * payload and attributes, the declared variable will be read-only.  The
  * variable will be visible to all of the cuda functions defined in the
  * current file.  The binding of variables to values on API objects is
  * allowed to vary from one instance to another.
  *
  *
  * <B>Valid semanticNames</B>
  *
  * - \b rtLaunchIndex - The launch invocation index. Type must be one of \a unsigned int, \a uint2, \a uint3, \a int, \a int2, \a int3 and is read-only.
  * - \b rtLaunchDim - The size of each dimension of the launch. The values range from 1 to the launch size in that dimension. Type must be one of \a unsigned int, \a uint2, \a uint3, \a int, \a int2, \a int3 and is read-only.
  * - \b rtCurrentRay - The currently active ray, valid only when a call to @ref rtTrace is active.  Type must be \a optix::Ray and is read-only.
  * - \b rtIntersectionDistance - The current closest hit distance, valid only when a call to @ref rtTrace is active. Type must be \a float and is read-only.
  * - \b rtRayPayload - The struct passed into the most recent @ref rtTrace call and is read-write.
  * - \b attribute \a name - A named attribute passed from the intersection program to a closest-hit or any-hit program.  The types must match in both sets of programs.  This variable is read-only in the closest-hit or any-hit program and is written in the intersection program.
  *
  * @param[in]  type        Type of the variable
  * @param[in]  name        Name of the variable
  * @param[in]  semantic    Semantic name
  * @param[in]  annotation  Annotation for this variable
  * 
  * <B>History</B>
  * 
  * - @ref rtDeclareVariable was introduced in OptiX 1.0.
  * - \a rtLaunchDim was introduced in OptiX 2.0.
  * 
  * <B>See also</B>
  * @ref rtDeclareAnnotation,
  * @ref rtVariableGetAnnotation,
  * @ref rtContextDeclareVariable,
  * @ref rtProgramDeclareVariable,
  * @ref rtSelectorDeclareVariable,
  * @ref rtGeometryInstanceDeclareVariable,
  * @ref rtGeometryDeclareVariable,
  * @ref rtMaterialDeclareVariable
  * 
  */
#define rtDeclareVariable(type, name, semantic, annotation)    \
  namespace rti_internal_typeinfo { \
    __device__ ::rti_internal_typeinfo::rti_typeinfo name = { ::rti_internal_typeinfo::_OPTIX_VARIABLE, sizeof(type)}; \
  } \
  namespace rti_internal_typename { \
    __device__ char name[] = #type; \
  } \
  namespace rti_internal_typeenum { \
    __device__ int name = ::rti_internal_typeinfo::rti_typeenum<type>::m_typeenum; \
  } \
  namespace rti_internal_semantic { \
    __device__ char name[] = #semantic; \
  } \
  namespace rti_internal_annotation { \
    __device__ char name[] = #annotation; \
  } \
  __device__ type name


/**
  * @brief Annotation declaration
  * 
  * @ingroup CUDACDeclarations
  * 
  * <B>Description</B>
  * 
  * @ref rtDeclareAnnotation sets the annotation \a annotation of the given
  * variable \a name.  Typically annotations are declared using an argument to 
  * @ref rtDeclareVariable, but variables of type @ref rtBuffer and @ref rtTextureSampler
  * are declared using templates, so separate annotation attachment is required.
  *
  * OptiX does not attempt to interpret the annotation in any way.  It is considered
  * metadata for the application to query and interpret in its own way.
  *
  * <B>Valid annotations</B>
  *
  * The macro @ref rtDeclareAnnotation uses the C pre-processor's "stringification"
  * feature to turn the literal text of the annotation argument into a string 
  * constant.  The pre-processor will backslash-escape quotes and backslashes 
  * within the text of the annotation.  Leading and trailing whitespace will be 
  * ignored, and sequences of whitespace in the middle of the text is converted to 
  * a single space character in the result.  The only restriction the C-PP places 
  * on the text is that it may not contain a comma character unless it is either 
  * quoted or contained within parens: "," or (,).
  *
  * Example(s):
  *
  * @code
  *  rtDeclareAnnotation( tex, this is a test );
  *  annotation = "this is a test"
  *
  *  rtDeclareAnnotation( tex, "this is a test" );
  *  annotation = "\"this is a test\""
  *
  *  rtDeclareAnnotation( tex, float3 a = {1, 2, 3} );
  *  --> Compile Error, no unquoted commas may be present in the annotation
  *
  *  rtDeclareAnnotation( tex, "float3 a = {1, 2, 3}" );
  *  annotation = "\"float3 a = {1, 2, 3}\""
  *
  *  rtDeclareAnnotation( tex, string UIWidget = "slider";
  *                            float UIMin = 0.0;
  *                            float UIMax = 1.0; );
  *  annotation = "string UIWidget = \"slider\"; float UIMin = 0.0; float UIMax = 1.0;"
  * @endcode
  *
  * @param[in]  variable    Variable to annotate
  * @param[in]  annotation  Annotation metadata
  * 
  * <B>History</B>
  * 
  * @ref rtDeclareAnnotation was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtDeclareVariable,
  * @ref rtVariableGetAnnotation
  * 
  */
#define rtDeclareAnnotation(variable, annotation) \
  namespace rti_internal_annotation { \
    __device__ char variable[] = #annotation; \
  }

/* Declares a function that can be set via rtVariableSetObject and called from CUDA C.
   Once declared the function variable can be used as if it were a regular function.  Note
   that the parameters argument to the macro need to have parentheses even if they will be
   empty.

   Example: rtCallableProgram(float, times2, (float));
   Example: rtCallableProgram(float, doStuff, ());
 */

template<typename T> struct rtCallableProgramSizeofWrapper { static const size_t value = sizeof(T); }; 
template<> struct rtCallableProgramSizeofWrapper<void> { static const size_t value = 0; };


/**
  * @brief Callable Program Declaration
  * 
  * @ingroup CUDACDeclarations
  * 
  * <B>Description</B>
  * 
  * @ref rtCallableProgram declares callable program \a name, which will appear
  * to be a callable function with the specified return type and list of arguments.
  * This callable program must be matched against a
  * variable declared on the API object using @ref rtVariableSetObject.
  *
  * Unless compatibility with SM_10 is needed, new code should \#define
  * RT_USE_TEMPLATED_RTCALLABLEPROGRAM and rely on the new templated version of
  * rtCallableProgram.
  *
  * Example(s):
  *
  *@code
  *  rtCallableProgram(float3, modColor, (float3, float));
  *@endcode
  * 
  * @param[in]  return_type    Return type of the callable program
  * @param[in]  function_name  Name of the callable program
  * @param[in]  parameter_list Parameter_List of the callable program
  * 
  * <B>History</B>
  * 
  * @ref rtCallableProgram was introduced in OptiX 3.0.
  * 
  * <B>See also</B>
  * @ref rtDeclareVariable
  * @ref rtCallableProgramId
  * @ref rtCallableProgramX
  * 
  */

#ifndef RT_USE_TEMPLATED_RTCALLABLEPROGRAM
#if (__CUDA_ARCH__ < 200) || (CUDART_VERSION < 4010)
#define rtCallableProgram(return_type, function_name, parameter_list)   \
  namespace rti_internal_typeinfo {                                     \
    __device__ ::rti_internal_typeinfo::rti_typeinfo function_name = { ::rti_internal_typeinfo::_OPTIX_VARIABLE, rtCallableProgramSizeofWrapper<return_type>::value }; \
  }                                                                     \
  namespace rti_internal_typename {                                     \
    __device__ char function_name[] = #return_type;                     \
  }                                                                     \
  namespace rti_internal_semantic {                                     \
    __device__ char function_name[] = ""; /* used to be rt_call, but not needed anymore */ \
  }                                                                     \
  namespace rti_internal_annotation {                                   \
    __device__ char function_name[] = #parameter_list;                  \
  }                                                                     \
  __noinline__ __device__ return_type function_name parameter_list { typedef return_type localtype; return localtype(); }
#else
#define rtCallableProgram(return_type, function_name, parameter_list)   \
  rtDeclareVariable(optix::boundCallableProgramId<return_type parameter_list>, function_name,,);
#endif
#endif


/*
  Buffer
*/

namespace optix {
  template<typename T, int Dim> struct bufferId;
  
  template<typename T, int Dim = 1> struct buffer {
    typedef VectorTypes<size_t, Dim> WrapperType;
    typedef typename VectorTypes<size_t, Dim>::Type IndexType;

    __device__ __forceinline__ IndexType size() const {
      return WrapperType::make(rt_buffer_get_size(this, Dim, sizeof(T)));
    }
    __device__ __forceinline__ T& operator[](IndexType i) {
      size_t4 c = make_index(i);
      return *(T*)create(type<T>(), rt_buffer_get(this, Dim, sizeof(T), c.x, c.y, c.z, c.w));
    }
  protected:
    __inline__ __device__ static size_t4 make_index(size_t v0) { return make_size_t4(v0, 0, 0, 0); }
    __inline__ __device__ static size_t4 make_index(size_t2 v0) { return make_size_t4(v0.x, v0.y, 0, 0); }
    __inline__ __device__ static size_t4 make_index(size_t3 v0) { return make_size_t4(v0.x, v0.y, v0.z, 0); }
    __inline__ __device__ static size_t4 make_index(size_t4 v0) { return make_size_t4(v0.x, v0.y, v0.z, v0.w); }

    // This struct is used to create overloaded methods based on the type of buffer
    // element.  Note that we use a different name for the typemplate typename to avoid
    // confusing it with the template type of buffer.
    template<typename T2> struct type { };

    // Regular type: just return the pointer
    template<typename T2> __device__ __forceinline__ static void* create(type<T2>, void* v) { return v; }
    // bufferId type.  Read the ID from the buffer than assign it to a new bufferId to be
    // used later.
    template<typename T2, int Dim2>
    __device__ __forceinline__ static void* create(type<bufferId<T2,Dim2> >, void* v) 
    {
      // Returning a pointer to a locally created thing is generally a bad idea,
      // however since this function and its caller are always inlined the
      // object is created on the same stack that the buffer::operator[] was
      // called from.
      bufferId<T,Dim> b(*reinterpret_cast<int*>(v));
      void* result = &b;
      return result;
    }
  };

  // Helper class for encapsulating a buffer ID with methods to allow it to behave as a buffer.
  template<typename T, int Dim = 1> struct bufferId : public buffer<T,Dim> {
    typedef typename buffer<T,Dim>::WrapperType WrapperType;
    typedef typename buffer<T,Dim>::IndexType IndexType;
    
    // Default constructor
    __device__ __forceinline__ bufferId() {}
    // Constructor that initializes the id with null.
    __device__ __forceinline__ bufferId(RTbufferidnull nullid) { m_id = (int)nullid; }
    // Constructor that initializes the id.
    __device__ __forceinline__ explicit bufferId(int id) : m_id(id) {}
      
    // assigment that initializes the id with null.
    __device__ __forceinline__ bufferId& operator= (RTbufferidnull nullid) { m_id = nullid; return *this; }

    // Buffer access methods that use m_id as the argument to identify which buffer is
    // being accessed.
    __device__ __forceinline__ IndexType size() const {
      return WrapperType::make(rt_buffer_get_size_id(m_id, Dim, sizeof(T)));
    }
    __device__ __forceinline__ T& operator[](IndexType i) const {
      size_t4 c = make_index(i);
      return *(T*)create(typename buffer<T,Dim>::template type<T>(),
                         rt_buffer_get_id(m_id, Dim, sizeof(T), c.x, c.y, c.z, c.w));
    }

    __device__ __forceinline__ int getId() const { return m_id; }

    __device__ __forceinline__ operator bool() const { return m_id; }
 
  private:
    // Member variable
    int m_id;
  };
}

/**
  * @brief Declare a reference to a buffer object
  *
  * @ingroup CUDACTypes
  *
  * <B>Description</B>
  *
  * @code
  *   rtBuffer<Type, Dim> name;
  * @endcode
  *
  * @ref rtBuffer declares a buffer of type \a Type and dimensionality \a Dim.
  * \a Dim must be between 1 and 4 inclusive and defaults to 1 if not specified.
  * The resulting object provides access to buffer data through the [] indexing
  * operator, where the index is either unsigned int, uint2, uint3, or uint4 for
  * 1, 2, 3 or 4-dimensional buffers (respectively).  This operator can be used
  * to read from or write to the resulting buffer at the specified index.
  *
  * The named buffer obeys the runtime name lookup semantics as described in
  * @ref rtDeclareVariable.  A compile error will result if the named buffer is
  * not bound to a buffer object, or is bound to a buffer object of the
  * incorrect type or dimension.  The behavior of writing to a read-only buffer
  * is undefined.  Reading from a write-only buffer is well defined only if a
  * value has been written previously by the same thread.
  *
  * This declaration must appear at the file scope (not within a function), and
  * will be visible to all @ref RT_PROGRAM instances within the same compilation
  * unit.
  *
  * An annotation may be associated with the buffer variable by using the
  * @ref rtDeclareAnnotation macro.
  *
  * <B>History</B>
  * 
  * @ref rtBuffer was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtDeclareAnnotation,
  * @ref rtDeclareVariable,
  * @ref rtBufferCreate,
  * @ref rtTextureSampler,
  * @ref rtVariableSetObject
  * @ref rtBufferId
  * 
  */
#define rtBuffer       __device__ optix::buffer
/**
  * @brief A class that wraps buffer access functionality when using a buffer id.
  *
  * @ingroup CUDACTypes
  *
  * <B>Description</B>
  *
  * The @ref rtBufferId provides an interface similar to @ref rtBuffer when
  * using a buffer id obtained through @ref rtBufferGetId.  Unlike rtBuffer,
  * this class can be passed to functions or stored in other data structures
  * such as the ray payload.  It should be noted, however, doing so can limit
  * the extent that OptiX can optimize the generated code.
  *
  * There is also a version of rtBufferId that can be used by the host code, so
  * that types can exist in both host and device code. See the documetation for
  * rtBufferId found in the optix C++ API header.
  *
  * <B>History</B>
  *
  * @ref rtBufferId was introduced in OptiX 3.5.
  *
  * <B>See also</B>
  *
  * @ref rtBuffer
  * @ref rtBufferGetId
  *
  */
#define rtBufferId                optix::bufferId

/*
   Texture - they are defined in CUDA
*/

/**
  * @brief Declares a reference to a texture sampler object
  *
  * @ingroup CUDACTypes
  *
  * <B>Description</B>
  *
  * @ref rtTextureSampler declares a texture of type \a Type and
  * dimensionality \a Dim.  \a Dim must be between 1 and 3 inclusive and
  * defaults to 1 if not specified.  The resulting object provides access
  * to texture data through the tex1D, tex2D and tex3D functions.  These
  * functions can be used only to read the data.
  *
  * Texture filtering and wrapping modes, specified in \a ReadMode will be
  * dependent on the state of the texture sampler object created with
  * @ref rtTextureSamplerCreate.
  *
  * An annotation may be associated with the texture sampler variable by
  * using the @ref rtDeclareAnnotation macro.  
  *
  * <B>History</B>
  * 
  * @ref rtTextureSampler was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtDeclareAnnotation,
  * @ref rtTextureSamplerCreate
  * 
  */
#define rtTextureSampler texture

namespace optix {

  typedef int rtTextureId;

  #define _OPTIX_TEX_FUNC_DECLARE_(FUNC, SIGNATURE, PARAMS )  \
  template<> inline __device__ unsigned char FUNC SIGNATURE   \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return (unsigned char)(tmp.x);                            \
  }                                                           \
  template<> inline __device__ char FUNC SIGNATURE            \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return (char)(tmp.x);                                     \
  }                                                           \
  template<> inline __device__ unsigned short FUNC SIGNATURE  \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return (unsigned short)(tmp.x);                           \
  }                                                           \
  template<> inline __device__ short FUNC SIGNATURE           \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return (short)(tmp.x);                                    \
  }                                                           \
  template<> inline __device__ int FUNC SIGNATURE             \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return tmp.x;                                             \
  }                                                           \
  template<> inline __device__ unsigned int FUNC SIGNATURE    \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return tmp.x;                                             \
  }                                                           \
  template<> inline __device__ uchar1 FUNC SIGNATURE          \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return make_uchar1(tmp.x);                                \
  }                                                           \
  template<> inline __device__ char1 FUNC SIGNATURE           \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return make_char1(tmp.x);                                 \
  }                                                           \
  template<> inline __device__ ushort1 FUNC SIGNATURE         \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return make_ushort1(tmp.x);                               \
  }                                                           \
  template<> inline __device__ short1 FUNC SIGNATURE          \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return make_short1(tmp.x);                                \
  }                                                           \
  template<> inline __device__ uint1 FUNC SIGNATURE           \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return make_uint1(tmp.x);                                 \
  }                                                           \
  template<> inline __device__ int1 FUNC SIGNATURE            \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return make_int1(tmp.x);                                  \
  }                                                           \
  template<> inline __device__ float FUNC SIGNATURE           \
  {                                                           \
    float4 tmp = FUNC <float4> PARAMS;                        \
    return tmp.x;                                             \
  }                                                           \
  template<> inline __device__ uchar2 FUNC SIGNATURE          \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return make_uchar2(tmp.x, tmp.y);                         \
  }                                                           \
  template<> inline __device__ char2 FUNC SIGNATURE           \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return make_char2(tmp.x, tmp.y);                          \
  }                                                           \
  template<> inline __device__ ushort2 FUNC SIGNATURE         \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return make_ushort2(tmp.x, tmp.y);                        \
  }                                                           \
  template<> inline __device__ short2 FUNC SIGNATURE          \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return make_short2(tmp.x, tmp.y);                         \
  }                                                           \
  template<> inline __device__ uint2 FUNC SIGNATURE           \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return make_uint2(tmp.x, tmp.y);                          \
  }                                                           \
  template<> inline __device__ int2 FUNC SIGNATURE            \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return make_int2(tmp.x, tmp.y);                           \
  }                                                           \
  template<> inline __device__ float2 FUNC SIGNATURE          \
  {                                                           \
    float4 tmp = FUNC <float4> PARAMS;                        \
    return ::make_float2(tmp.x, tmp.y);                       \
  }                                                           \
  template<> inline __device__ uchar4 FUNC SIGNATURE          \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return make_uchar4(tmp.x, tmp.y, tmp.z, tmp.w);           \
  }                                                           \
  template<> inline __device__ char4 FUNC SIGNATURE           \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return make_char4(tmp.x, tmp.y, tmp.z, tmp.w);            \
  }                                                           \
  template<> inline __device__ ushort4 FUNC SIGNATURE         \
  {                                                           \
    uint4 tmp = FUNC <uint4> PARAMS;                          \
    return make_ushort4(tmp.x, tmp.y, tmp.z, tmp.w);          \
  }                                                           \
  template<> inline __device__ short4 FUNC SIGNATURE          \
  {                                                           \
    int4 tmp = FUNC <int4> PARAMS;                            \
    return make_short4(tmp.x, tmp.y, tmp.z, tmp.w);           \
  }   

  inline __device__ int4 float4AsInt4( float4 f4 ) {
    return make_int4(__float_as_int(f4.x), __float_as_int(f4.y), __float_as_int(f4.z), __float_as_int(f4.w));
  }
    
  inline __device__ uint4 float4AsUInt4( float4 f4 ) {
    return make_uint4(__float_as_int(f4.x), __float_as_int(f4.y), __float_as_int(f4.z), __float_as_int(f4.w));
  }

  /** 
    * @brief Similar to CUDA C's texture functions, OptiX programs can access textures in a bindless way
    *
    * @ingroup rtTex
    *
    * <B>Description</B>
    *
    * \b rtTex1D, \b rtTex2D and \b rtTex3D fetch the texture referenced by the \a id with
    * texture coordinate \a x, \a y and \a z. The texture sampler \a id can be obtained on the host 
    * side using @ref rtTextureSamplerGetId function.
    * There are also C++ template and C-style additional declarations for other 
    * texture types (char1, uchar1, char2, uchar2 ...):
    * 
    * To get texture size dimensions \b rtTexSize can be used.
    *
    * Texture element may be fetched with integer coordinates using functions:
    * \b rtTex1DFetch, \b rtTex2DFetch and \b rtTex3DFetch
    *
    * Textures may also be sampled by providing a level of detail for mip mapping or
    * gradients for anisotropic filtering. An integer layer number is required for layered textures (arrays of textures)
    * using functions:  
    * \b rtTex2DGather, \b rtTex1DGrad, \b rtTex2DGrad, \b rtTex3DGrad, \b rtTex1DLayeredGrad, \b rtTex2DLayeredGrad, 
    * \b rtTex1DLod, \b rtTex2DLod, \b rtTex3DLod, \b rtTex1DLayeredLod, \b rtTex2DLayeredLod, \b rtTex1DLayered, \b rtTex2DLayered.
    *
    * And cubeamp textures with \b rtTexCubemap, \b rtTexCubemapLod, \b rtTexCubemapLayered and \b rtTexCubemapLayeredLod.
    *
    * @code
    *  template<> uchar2 rtTex1D(rtTextureId id, float x)
    *  void rtTex1D(ushort2 *retVal, rtTextureId id, float x)
    * @endcode
    *
    *
    * <B>History</B>
    * 
    * \b rtTex1D, \b rtTex2D and \b rtTex3D were introduced in OptiX 3.0.
    *
    * \b rtTexSize, \b rtTex1DFetch, \b rtTex2DFetch, \b rtTex3DFetch,
    * \b rtTex2DGather, \b rtTex1DGrad, \b rtTex2DGrad, \b rtTex3DGrad, \b rtTex1DLayeredGrad, \b rtTex2DLayeredGrad, 
    * \b rtTex1DLod, \b rtTex2DLod, \b rtTex3DLod, \b rtTex1DLayeredLod, \b rtTex2DLayeredLod, \b rtTex1DLayered, \b rtTex2DLayered,
    * \b rtTexCubemap, \b rtTexCubemapLod, \b rtTexCubemapLayered and \b rtTexCubemapLayeredLod
    * were introduced in OptiX 3.9.
    * 
    * <B>See also</B>
    * @ref rtTextureSamplerGetId
    * 
    */
  /** @{ */

  inline __device__ uint3 rtTexSize(rtTextureId id)
  {
    return optix::rt_texture_get_size_id(id);
  }

  template<typename T>
  inline __device__ T rtTex1D(rtTextureId id, float x);
  template<> inline __device__ float4 rtTex1D(rtTextureId id, float x)
  {
    return optix::rt_texture_get_f_id(id, 1, x, 0, 0, 0);
  }
  template<> inline __device__ int4 rtTex1D(rtTextureId id, float x)
  {
    return optix::rt_texture_get_i_id(id, 1, x, 0, 0, 0);
  }
  template<> inline __device__ uint4 rtTex1D(rtTextureId id, float x)
  {
    return optix::rt_texture_get_u_id(id, 1, x, 0, 0, 0);
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex1D, (rtTextureId id, float x), (id, x) )
  template<typename T> 
  inline __device__ void rtTex1D(T* retVal, rtTextureId id, float x)
  {
    T tmp = rtTex1D<T>(id, x);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex1DFetch(rtTextureId id, int x);
  template<> inline __device__ float4 rtTex1DFetch(rtTextureId id, int x)
  {
    return optix::rt_texture_get_fetch_id(id, 1, x, 0, 0, 0);
  }
  template<> inline __device__ int4 rtTex1DFetch(rtTextureId id, int x)
  {
    return float4AsInt4(optix::rt_texture_get_fetch_id(id, 1, x, 0, 0, 0));
  }
  template<> inline __device__ uint4 rtTex1DFetch(rtTextureId id, int x)
  {
    return float4AsUInt4(optix::rt_texture_get_fetch_id(id, 1, x, 0, 0, 0));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex1DFetch, (rtTextureId id, int x), (id, x) )
  template<typename T> 
  inline __device__ void rtTex1DFetch(T* retVal, rtTextureId id, int x)
  {
    T tmp = rtTex1DFetch<T>(id, x);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex2D(rtTextureId id, float x, float y);
  template<>
  inline __device__ float4 rtTex2D(rtTextureId id, float x, float y)
  {
    return optix::rt_texture_get_f_id(id, 2, x, y, 0, 0);
  }
  template<>
  inline __device__ int4 rtTex2D(rtTextureId id, float x, float y)
  {
    return optix::rt_texture_get_i_id(id, 2, x, y, 0, 0);
  }
  template<>
  inline __device__ uint4 rtTex2D(rtTextureId id, float x, float y)
  {
    return optix::rt_texture_get_u_id(id, 2, x, y, 0, 0);
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex2D, (rtTextureId id, float x, float y), (id, x, y) )
  template<typename T> 
  inline __device__ void rtTex2D(T* retVal, rtTextureId id, float x, float y)
  {
    T tmp = rtTex2D<T>(id, x, y);
    *retVal = tmp;
  }
  
  template<typename T>
  inline __device__ T rtTex2DFetch(rtTextureId id, int x, int y);
  template<> inline __device__ float4 rtTex2DFetch(rtTextureId id, int x, int y)
  {
    return optix::rt_texture_get_fetch_id(id, 2, x, y, 0, 0);
  }
  template<> inline __device__ int4 rtTex2DFetch(rtTextureId id, int x, int y)
  {
    return float4AsInt4(optix::rt_texture_get_fetch_id(id, 2, x, y, 0, 0));
  }
  template<> inline __device__ uint4 rtTex2DFetch(rtTextureId id, int x, int y)
  {
    return float4AsUInt4(optix::rt_texture_get_fetch_id(id, 2, x, y, 0, 0));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex2DFetch, (rtTextureId id, int x, int y), (id, x, y) )
  template<typename T> 
  inline __device__ void rtTex2DFetch(T* retVal, rtTextureId id, int x, int y)
  {
    T tmp = rtTex2DFetch<T>(id, x, y);
    *retVal = tmp;
  }
  
  template<typename T>
  inline __device__ T rtTex3D(rtTextureId id, float x, float y, float z);
  template<> inline __device__ float4 rtTex3D(rtTextureId id, float x, float y, float z)
  {
    return optix::rt_texture_get_f_id(id, 3, x, y, z, 0);
  }
  template<> inline __device__ int4 rtTex3D(rtTextureId id, float x, float y, float z)
  {
    return optix::rt_texture_get_i_id(id, 3, x, y, z, 0);
  }
  template<> inline __device__ uint4 rtTex3D(rtTextureId id, float x, float y, float z)
  {
    return optix::rt_texture_get_u_id(id, 3, x, y, z, 0);
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex3D, (rtTextureId id, float x, float y, float z), (id, x, y, z) )
  template<typename T> 
  inline __device__ void rtTex3D(T* retVal, rtTextureId id, float x, float y, float z)
  {
    T tmp = rtTex3D<T>(id, x, y, z);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex3DFetch(rtTextureId id, int x, int y, int z);
  template<> inline __device__ float4 rtTex3DFetch(rtTextureId id, int x, int y, int z)
  {
    return optix::rt_texture_get_fetch_id(id, 3, x, y, z, 0);
  }
  template<> inline __device__ int4 rtTex3DFetch(rtTextureId id, int x, int y, int z)
  {
    return float4AsInt4(optix::rt_texture_get_fetch_id(id, 3, x, y, z, 0));
  }
  template<> inline __device__ uint4 rtTex3DFetch(rtTextureId id, int x, int y, int z)
  {
    return float4AsUInt4(optix::rt_texture_get_fetch_id(id, 3, x, y, z, 0));
  }    
  _OPTIX_TEX_FUNC_DECLARE_(rtTex3DFetch, (rtTextureId id, int x, int y, int z), (id, x, y, z) )
  template<typename T> 
  inline __device__ void rtTex3DFetch(T* retVal, rtTextureId id, int x, int y, int z)
  {
    T tmp = rtTex3DFetch<T>(id, x, y, z);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex2DGather(rtTextureId id, float x, float y, int comp = 0);
  template<> inline __device__ float4 rtTex2DGather(rtTextureId id, float x, float y, int comp)
  {
    return optix::rt_texture_get_gather_id(id, x, y, comp);
  }
  template<> inline __device__ int4 rtTex2DGather(rtTextureId id, float x, float y, int comp)
  {
    return float4AsInt4(optix::rt_texture_get_gather_id(id, x, y, comp));
  }
  template<> inline __device__ uint4 rtTex2DGather(rtTextureId id, float x, float y, int comp)
  {
    return float4AsUInt4(optix::rt_texture_get_gather_id(id, x, y, comp));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex2DGather, (rtTextureId id, float x, float y, int comp), (id, x, y, comp) )
  template<typename T> 
  inline __device__ void rtTex2DGather(T* retVal, rtTextureId id, float x, float y, int comp = 0)
  {
    T tmp = rtTex2DGather<T>(id, x, y, comp);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex1DGrad(rtTextureId id, float x, float dPdx, float dPdy);
  template<> inline __device__ float4 rtTex1DGrad(rtTextureId id, float x, float dPdx, float dPdy)
  {
    return optix::rt_texture_get_grad_id(id, TEX_LOOKUP_1D, x, 0, 0, 0, dPdx, 0, 0, dPdy, 0, 0);
  }
  template<> inline __device__ int4 rtTex1DGrad(rtTextureId id, float x, float dPdx, float dPdy)
  {
    return float4AsInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_1D, x, 0, 0, 0, dPdx, 0, 0, dPdy, 0, 0));
  }
  template<> inline __device__ uint4 rtTex1DGrad(rtTextureId id, float x, float dPdx, float dPdy)
  {
    return float4AsUInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_1D, x, 0, 0, 0, dPdx, 0, 0, dPdy, 0, 0));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex1DGrad, (rtTextureId id, float x, float dPdx, float dPdy), (id, x, dPdx, dPdy) )
  template<typename T> 
  inline __device__ void rtTex1DGrad(T* retVal, rtTextureId id, float x, float dPdx, float dPdy)
  {
    T tmp = rtTex1DGrad<T>(id, x, dPdx, dPdy);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex2DGrad(rtTextureId id, float x, float y, float2 dPdx, float2 dPdy);
  template<> inline __device__ float4 rtTex2DGrad(rtTextureId id, float x, float y, float2 dPdx, float2 dPdy)
  {
    return optix::rt_texture_get_grad_id(id, TEX_LOOKUP_2D, x, y, 0, 0, dPdx.x, dPdx.y, 0, dPdy.x, dPdy.y, 0);
  }
  template<> inline __device__ int4 rtTex2DGrad(rtTextureId id, float x, float y, float2 dPdx, float2 dPdy)
  {
    return float4AsInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_2D, x, y, 0, 0, dPdx.x, dPdx.y, 0, dPdy.x, dPdy.y, 0));
  }
  template<> inline __device__ uint4 rtTex2DGrad(rtTextureId id, float x, float y, float2 dPdx, float2 dPdy)
  {
    return float4AsUInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_2D, x, y, 0, 0, dPdx.x, dPdx.y, 0, dPdy.x, dPdy.y, 0));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex2DGrad, (rtTextureId id, float x, float y, float2 dPdx, float2 dPdy), (id, x, y, dPdx, dPdy) )
  template<typename T> 
  inline __device__ void rtTex2DGrad(T* retVal, rtTextureId id, float x, float y, float2 dPdx, float2 dPdy)
  {
    T tmp = rtTex2DGrad<T>(id, x, y, dPdx, dPdy);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex3DGrad(rtTextureId id, float x, float y, float z, float4 dPdx, float4 dPdy);
  template<> inline __device__ float4 rtTex3DGrad(rtTextureId id, float x, float y, float z, float4 dPdx, float4 dPdy)
  {
    return optix::rt_texture_get_grad_id(id, TEX_LOOKUP_3D, x, y, z, 0, dPdx.x, dPdx.y, dPdx.z, dPdy.x, dPdy.y, dPdy.z);
  }
  template<> inline __device__ int4 rtTex3DGrad(rtTextureId id, float x, float y, float z, float4 dPdx, float4 dPdy)
  {
    return float4AsInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_3D, x, y, z, 0, dPdx.x, dPdx.y, dPdx.z, dPdy.x, dPdy.y, dPdy.z));
  }
  template<> inline __device__ uint4 rtTex3DGrad(rtTextureId id, float x, float y, float z, float4 dPdx, float4 dPdy)
  {
    return float4AsUInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_3D, x, y, z, 0, dPdx.x, dPdx.y, dPdx.z, dPdy.x, dPdy.y, dPdy.z));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex3DGrad, (rtTextureId id, float x, float y, float z, float4 dPdx, float4 dPdy), (id, x, y, z, dPdx, dPdy) )
  template<typename T> 
  inline __device__ void rtTex3DGrad(T* retVal, rtTextureId id, float x, float y, float z, float4 dPdx, float4 dPdy)
  {
    T tmp = rtTex3DGrad<T>(id, x, y, z, dPdx, dPdy);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex1DLayeredGrad(rtTextureId id, float x, int layer, float dPdx, float dPdy);
  template<> inline __device__ float4 rtTex1DLayeredGrad(rtTextureId id, float x, int layer, float dPdx, float dPdy)
  {
    return optix::rt_texture_get_grad_id(id, TEX_LOOKUP_A1, x, 0, 0, layer, dPdx, 0, 0, dPdy, 0, 0);
  }
  template<> inline __device__ int4 rtTex1DLayeredGrad(rtTextureId id, float x, int layer, float dPdx, float dPdy)
  {
    return float4AsInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_A1, x, 0, 0, layer, dPdx, 0, 0, dPdy, 0, 0));
  }
  template<> inline __device__ uint4 rtTex1DLayeredGrad(rtTextureId id, float x, int layer, float dPdx, float dPdy)
  {
    return float4AsUInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_A1, x, 0, 0, layer, dPdx, 0, 0, dPdy, 0, 0));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex1DLayeredGrad, (rtTextureId id, float x, int layer, float dPdx, float dPdy), (id, x, layer, dPdx, dPdy) )
  template<typename T> 
  inline __device__ void rtTex1DLayeredGrad(T* retVal, rtTextureId id, float x, int layer, float dPdx, float dPdy)
  {
    T tmp = rtTex1DLayeredGrad<T>(id, x, layer, dPdx, dPdy);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex2DLayeredGrad(rtTextureId id, float x, float y, int layer, float2 dPdx, float2 dPdy);
  template<> inline __device__ float4 rtTex2DLayeredGrad(rtTextureId id, float x, float y, int layer, float2 dPdx, float2 dPdy)
  {
    return optix::rt_texture_get_grad_id(id, TEX_LOOKUP_A2, x, y, 0, layer, dPdx.x, dPdx.y, 0, dPdy.x, dPdy.y, 0);
  }
  template<> inline __device__ int4 rtTex2DLayeredGrad(rtTextureId id, float x, float y, int layer, float2 dPdx, float2 dPdy)
  {
    return float4AsInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_A2, x, y, 0, layer, dPdx.x, dPdx.y, 0, dPdy.x, dPdy.y, 0));
  }
  template<> inline __device__ uint4 rtTex2DLayeredGrad(rtTextureId id, float x, float y, int layer, float2 dPdx, float2 dPdy)
  {
    return float4AsUInt4(optix::rt_texture_get_grad_id(id, TEX_LOOKUP_A2, x, y, 0, layer, dPdx.x, dPdx.y, 0, dPdy.x, dPdy.y, 0));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex2DLayeredGrad, (rtTextureId id, float x, float y, int layer, float2 dPdx, float2 dPdy), (id, x, y, layer, dPdx, dPdy) )
  template<typename T> 
  inline __device__ void rtTex2DLayeredGrad(T* retVal, rtTextureId id, float x, float y, int layer, float2 dPdx, float2 dPdy)
  {
    T tmp = rtTex2DLayeredGrad<T>(id, x, y, layer, dPdx, dPdy);
    *retVal = tmp;
  }
 
  template<typename T>
  inline __device__ T rtTex1DLod(rtTextureId id, float x, float level);
  template<> inline __device__ float4 rtTex1DLod(rtTextureId id, float x, float level)
  {
    return optix::rt_texture_get_level_id(id, TEX_LOOKUP_1D, x, 0, 0, 0, level );
  }
  template<> inline __device__ int4 rtTex1DLod(rtTextureId id, float x, float level)
  {
    return float4AsInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_1D, x, 0, 0, 0, level ));
  }
  template<> inline __device__ uint4 rtTex1DLod(rtTextureId id, float x, float level)
  {
    return float4AsUInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_1D, x, 0, 0, 0, level ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex1DLod, (rtTextureId id, float x, float level), (id, x, level) )
  template<typename T> 
  inline __device__ void rtTex1DLod(T* retVal, rtTextureId id, float x, float level)
  {
    T tmp = rtTex1DLod<T>(id, x, level);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex2DLod(rtTextureId id, float x, float y, float level);
  template<> inline __device__ float4 rtTex2DLod(rtTextureId id, float x, float y, float level)
  {
    return optix::rt_texture_get_level_id(id, TEX_LOOKUP_2D, x, y, 0, 0, level );
  }
  template<> inline __device__ int4 rtTex2DLod(rtTextureId id, float x, float y, float level)
  {
    return float4AsInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_2D, x, y, 0, 0, level ));
  }
  template<> inline __device__ uint4 rtTex2DLod(rtTextureId id, float x, float y, float level)
  {
    return float4AsUInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_2D, x, y, 0, 0, level ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex2DLod, (rtTextureId id, float x, float y, float level), (id, x, y, level) )
  template<typename T> 
  inline __device__ void rtTex2DLod(T* retVal, rtTextureId id, float x, float y, float level)
  {
    T tmp = rtTex2DLod<T>(id, x, y, level);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex3DLod(rtTextureId id, float x, float y, float z, float level);
  template<> inline __device__ float4 rtTex3DLod(rtTextureId id, float x, float y, float z, float level)
  {
    return optix::rt_texture_get_level_id(id, TEX_LOOKUP_3D, x, y, z, 0, level );
  }
  template<> inline __device__ int4 rtTex3DLod(rtTextureId id, float x, float y, float z, float level)
  {
    return float4AsInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_3D, x, y, z, 0, level ));
  }
  template<> inline __device__ uint4 rtTex3DLod(rtTextureId id, float x, float y, float z, float level)
  {
    return float4AsUInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_3D, x, y, z, 0, level ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex3DLod, (rtTextureId id, float x, float y, float z, float level), (id, x, y, z, level) )
  template<typename T> 
  inline __device__ void rtTex3DLod(T* retVal, rtTextureId id, float x, float y, float z, float level)
  {
    T tmp = rtTex3DLod<T>(id, x, y, z, level);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex1DLayeredLod(rtTextureId id, float x, int layer, float level);
  template<> inline __device__ float4 rtTex1DLayeredLod(rtTextureId id, float x, int layer, float level)
  {
    return optix::rt_texture_get_level_id(id, TEX_LOOKUP_A1, x, 0, 0, layer, level );
  }
  template<> inline __device__ int4 rtTex1DLayeredLod(rtTextureId id, float x, int layer, float level)
  {
    return float4AsInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_A1, x, 0, 0, layer, level ));
  }
  template<> inline __device__ uint4 rtTex1DLayeredLod(rtTextureId id, float x, int layer, float level)
  {
    return float4AsUInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_A1, x, 0, 0, layer, level ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex1DLayeredLod, (rtTextureId id, float x, int layer, float level), (id, x, layer, level) )
  template<typename T> 
  inline __device__ void rtTex1DLayeredLod(T* retVal, rtTextureId id, float x, int layer, float level)
  {
    T tmp = rtTex1DLayeredLod<T>(id, x, layer, level);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex2DLayeredLod(rtTextureId id, float x, float y, int layer, float level);
  template<> inline __device__ float4 rtTex2DLayeredLod(rtTextureId id, float x, float y, int layer, float level)
  {
    return optix::rt_texture_get_level_id(id, TEX_LOOKUP_A2, x, y, 0, layer, level );
  }
  template<> inline __device__ int4 rtTex2DLayeredLod(rtTextureId id, float x, float y, int layer, float level)
  {
    return float4AsInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_A2, x, y, 0, layer, level ));
  }
  template<> inline __device__ uint4 rtTex2DLayeredLod(rtTextureId id, float x, float y, int layer, float level)
  {
    return float4AsUInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_A2, x, y, 0, layer, level ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex2DLayeredLod, (rtTextureId id, float x, float y, int layer, float level), (id, x, y, layer, level) )
  template<typename T> 
  inline __device__ void rtTex2DLayeredLod(T* retVal, rtTextureId id, float x, float y, int layer, float level)
  {
    T tmp = rtTex2DLayeredLod<T>(id, x, y, layer, level);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex1DLayered(rtTextureId id, float x, int layer);
  template<> inline __device__ float4 rtTex1DLayered(rtTextureId id, float x, int layer)
  {
    return optix::rt_texture_get_base_id(id, TEX_LOOKUP_A1, x, 0, 0, layer );
  }
  template<> inline __device__ int4 rtTex1DLayered(rtTextureId id, float x, int layer)
  {
    return float4AsInt4(optix::rt_texture_get_base_id(id, TEX_LOOKUP_A1, x, 0, 0, layer ));
  }
  template<> inline __device__ uint4 rtTex1DLayered(rtTextureId id, float x, int layer)
  {
    return float4AsUInt4(optix::rt_texture_get_base_id(id, TEX_LOOKUP_A1, x, 0, 0, layer ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex1DLayered, (rtTextureId id, float x, int layer), (id, x, layer) )
  template<typename T> 
  inline __device__ void rtTex1DLayered(T* retVal, rtTextureId id, float x, int layer)
  {
    T tmp = rtTex1DLayered<T>(id, x, layer);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTex2DLayered(rtTextureId id, float x, float y, int layer);
  template<> inline __device__ float4 rtTex2DLayered(rtTextureId id, float x, float y, int layer)
  {
    return optix::rt_texture_get_base_id(id, TEX_LOOKUP_A2, x, y, 0, layer );
  }
  template<> inline __device__ int4 rtTex2DLayered(rtTextureId id, float x, float y, int layer)
  {
    return float4AsInt4(optix::rt_texture_get_base_id(id, TEX_LOOKUP_A2, x, y, 0, layer ));
  }
  template<> inline __device__ uint4 rtTex2DLayered(rtTextureId id, float x, float y, int layer)
  {
    return float4AsUInt4(optix::rt_texture_get_base_id(id, TEX_LOOKUP_A2, x, y, 0, layer ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTex2DLayered, (rtTextureId id, float x, float y, int layer), (id, x, y, layer) )
  template<typename T> 
  inline __device__ void rtTex2DLayered(T* retVal, rtTextureId id, float x, float y, int layer)
  {
    T tmp = rtTex2DLayered<T>(id, x, y, layer);
    *retVal = tmp;
  }
   
  template<typename T>
  inline __device__ T rtTexCubemap(rtTextureId id, float x, float y, float z);
  template<> inline __device__ float4 rtTexCubemap(rtTextureId id, float x, float y, float z)
  {
    return optix::rt_texture_get_base_id(id, TEX_LOOKUP_CUBE, x, y, z, 0 );
  }
  template<> inline __device__ int4 rtTexCubemap(rtTextureId id, float x, float y, float z)
  {
    return float4AsInt4(optix::rt_texture_get_base_id(id, TEX_LOOKUP_CUBE, x, y, z, 0 ));
  }
  template<> inline __device__ uint4 rtTexCubemap(rtTextureId id, float x, float y, float z)
  {
    return float4AsUInt4(optix::rt_texture_get_base_id(id, TEX_LOOKUP_CUBE, x, y, z, 0 ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTexCubemap, (rtTextureId id, float x, float y, float z), (id, x, y, z) )
  template<typename T> 
  inline __device__ void rtTexCubemap(T* retVal, rtTextureId id, float x, float y, float z)
  {
    T tmp = rtTexCubemap<T>(id, x, y, z);
    *retVal = tmp;
  }
    
  template<typename T>
  inline __device__ T rtTexCubemapLayered(rtTextureId id, float x, float y, float z, int layer);
  template<> inline __device__ float4 rtTexCubemapLayered(rtTextureId id, float x, float y, float z, int layer)
  {
    return optix::rt_texture_get_base_id(id, TEX_LOOKUP_ACUBE, x, y, z, layer );
  }
  template<> inline __device__ int4 rtTexCubemapLayered(rtTextureId id, float x, float y, float z, int layer)
  {
    return float4AsInt4(optix::rt_texture_get_base_id(id, TEX_LOOKUP_ACUBE, x, y, z, layer ));
  }
  template<> inline __device__ uint4 rtTexCubemapLayered(rtTextureId id, float x, float y, float z, int layer)
  {
    return float4AsUInt4(optix::rt_texture_get_base_id(id, TEX_LOOKUP_ACUBE, x, y, z, layer ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTexCubemapLayered, (rtTextureId id, float x, float y, float z, int layer), (id, x, y, z, layer) )
  template<typename T> 
  inline __device__ void rtTexCubemapLayered(T* retVal, rtTextureId id, float x, float y, float z, int layer)
  {
    T tmp = rtTexCubemapLayered<T>(id, x, y, z, layer);
    *retVal = tmp;
  }
    
  template<typename T>
  inline __device__ T rtTexCubemapLod(rtTextureId id, float x, float y, float z, float level);
  template<> inline __device__ float4 rtTexCubemapLod(rtTextureId id, float x, float y, float z, float level)
  {
    return optix::rt_texture_get_level_id(id, TEX_LOOKUP_CUBE, x, y, z, 0, level );
  }
  template<> inline __device__ int4 rtTexCubemapLod(rtTextureId id, float x, float y, float z, float level)
  {
    return float4AsInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_CUBE, x, y, z, 0, level ));
  }
  template<> inline __device__ uint4 rtTexCubemapLod(rtTextureId id, float x, float y, float z, float level)
  {
    return float4AsUInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_CUBE, x, y, z, 0, level ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTexCubemapLod, (rtTextureId id, float x, float y, float z, float level), (id, x, y, z, level) )
  template<typename T> 
  inline __device__ void rtTexCubemapLod(T* retVal, rtTextureId id, float x, float y, float z, float level)
  {
    T tmp = rtTexCubemapLod<T>(id, x, y, z, level);
    *retVal = tmp;
  }

  template<typename T>
  inline __device__ T rtTexCubemapLayeredLod(rtTextureId id, float x, float y, float z, int layer, float level);
  template<> inline __device__ float4 rtTexCubemapLayeredLod(rtTextureId id, float x, float y, float z, int layer, float level)
  {
    return optix::rt_texture_get_level_id(id, TEX_LOOKUP_ACUBE, x, y, z, layer, level );
  }
  template<> inline __device__ int4 rtTexCubemapLayeredLod(rtTextureId id, float x, float y, float z, int layer, float level)
  {
    return float4AsInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_ACUBE, x, y, z, layer, level ));
  }
  template<> inline __device__ uint4 rtTexCubemapLayeredLod(rtTextureId id, float x, float y, float z, int layer, float level)
  {
    return float4AsUInt4(optix::rt_texture_get_level_id(id, TEX_LOOKUP_ACUBE, x, y, z, layer, level ));
  }
  _OPTIX_TEX_FUNC_DECLARE_(rtTexCubemapLayeredLod, (rtTextureId id, float x, float y, float z, int layer, float level), (id, x, y, z, layer, level) )
  template<typename T> 
  inline __device__ void rtTexCubemapLayeredLod(T* retVal, rtTextureId id, float x, float y, float z, int layer, float level)
  {
    T tmp = rtTexCubemapLayeredLod<T>(id, x, y, z, layer, level);
    *retVal = tmp;
  }

  /** @} */

  #undef _OPTIX_TEX_FUNC_DECLARE_
};

/*
   Program
*/

/**
  * @brief Define an OptiX program
  * 
  * @ingroup CUDACDeclarations
  * 
  * <B>Description</B>
  * 
  * @ref RT_PROGRAM defines a program \b program_name with the specified
  * arguments and return value. This function can be bound to a specific
  * program object using @ref rtProgramCreateFromPTXString or
  * @ref rtProgramCreateFromPTXFile, which will subsequently get bound to
  * different programmable binding points.
  *
  * All programs should have a "void" return type. Bounding box programs
  * will have an argument for the primitive index and the bounding box
  * reference return value (type \b nvrt::AAbb&). Intersection programs will
  * have a single int primitiveIndex argument. All other programs take
  * zero arguments.
  * 
  * <B>History</B>
  * 
  * @ref RT_PROGRAM was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref RT_PROGRAM
  * @ref rtProgramCreateFromPTXFile
  * @ref rtProgramCreateFromPTXString
  * 
  */
#define RT_PROGRAM __global__

/* This is used to declare programs that can be attached to variables and called from
 * within other RT_PROGRAMS.
 *
 * There are some limitations with PTX that is targetted at sm_1x devices.
 *
 * 1. Functions declared with RT_CALLABLE_PROGRAM will not be emitted in the PTX unless
 *    another function calls it.  This can be fixed by declaring a __global__ helper
 *    function that calls the desired function.
 *
 *    RT_CALLABLE_PROGRAM
 *    float3 simple_shade(float multiplier,  float3 input_color)
 *    {
 *      return multiplier * input_color;
 *    }
 *
 *    #if __CUDA_ARCH__ < 200
 *    __global__ void stub() {
 *       (void) simple_shade( 0, make_float3(0,0,0) );
 *    }
 *    #endif
 *
 * 2. You can't pass pointers to functions or use integers for pointers.  In the first
 *    case CUDA will force the inline of the proxy function removing the call altogether,
 *    and in the case of passing pointers as integers, CUDA will assume that any pointer
 *    that was cast from an integer will point to global memory and could cause errors
 *    when loading from that pointer.  If you need to pass pointers, you should target
 *    sm_20.
 */
   
#define RT_CALLABLE_PROGRAM __device__ __noinline__


namespace rti_internal_callableprogram {
  /* Any classes or types in the rti_internal_callableprogram namespace are used to help
   * implement callable program features and should not be used directly.
   */


  /* CPArgVoid a special class to act as an unspecified argument type that we can
   * statically query to determine if we have called our function with the wrong number of
   * arguments.
   */
     
  class CPArgVoid {};
  template< typename T1>
  struct is_CPArgVoid            { static const bool result = false; };

  template<> 
  struct is_CPArgVoid<CPArgVoid> { static const bool result = true; };

  template< bool Condition, typename Dummy = void >
  struct check_is_CPArgVoid {
    typedef bool result;
  };

  template<typename IntentionalError>
  struct check_is_CPArgVoid<false,IntentionalError> {
    typedef typename IntentionalError::does_not_exist result;
  };

  /* callableProgramIdBase is the underlying class for handling both bound and bindless
   * callable program calls.  It should not be used directly, but instead the derived
   * classes of rtCallableProgramId and rtCallableProgramX should be used.
   */
  template <typename ReturnT
            ,typename Arg0T=rti_internal_callableprogram::CPArgVoid
            ,typename Arg1T=rti_internal_callableprogram::CPArgVoid
            ,typename Arg2T=rti_internal_callableprogram::CPArgVoid
            ,typename Arg3T=rti_internal_callableprogram::CPArgVoid
            ,typename Arg4T=rti_internal_callableprogram::CPArgVoid
            ,typename Arg5T=rti_internal_callableprogram::CPArgVoid
            ,typename Arg6T=rti_internal_callableprogram::CPArgVoid
            ,typename Arg7T=rti_internal_callableprogram::CPArgVoid
            ,typename Arg8T=rti_internal_callableprogram::CPArgVoid
            ,typename Arg9T=rti_internal_callableprogram::CPArgVoid
            >
  class callableProgramIdBase {
  public:
#if (CUDART_VERSION >= 4010)
      // Default constructor
      __device__ __forceinline__ callableProgramIdBase() {}
    // Constructor that initializes the id with null.
    __device__ __forceinline__ callableProgramIdBase(RTprogramidnull nullid) { m_id = (int)nullid; }
    // Constructor that initializes the id.
    __device__ __forceinline__ explicit callableProgramIdBase(int id) : m_id(id) {}
#endif

    ///////////////////////////////////////////////////
    // Call operators
    //
    // If you call the function with the wrong number of argument, you will get a
    // compilation error.  If you have too many, then you will warned that an argument
    // doesn't match the CPArgVoid type.  If you have too few, then the check_is_CPArgVoid
    // typedef will error out complaining that check_is_CPArgVoid::result isn't a type.
    __device__ __forceinline__ ReturnT operator()()
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg0T>::result>::result Arg0_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg1T>::result>::result Arg1_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg2T>::result>::result Arg2_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg3T>::result>::result Arg3_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg4T>::result>::result Arg4_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg5T>::result>::result Arg5_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg6T>::result>::result Arg6_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg7T>::result>::result Arg7_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg8T>::result>::result Arg8_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)();
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call();
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0)
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg1T>::result>::result Arg1_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg2T>::result>::result Arg2_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg3T>::result>::result Arg3_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg4T>::result>::result Arg4_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg5T>::result>::result Arg5_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg6T>::result>::result Arg6_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg7T>::result>::result Arg7_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg8T>::result>::result Arg8_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)(Arg0T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0);
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0, Arg1T arg1)
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg2T>::result>::result Arg2_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg3T>::result>::result Arg3_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg4T>::result>::result Arg4_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg5T>::result>::result Arg5_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg6T>::result>::result Arg6_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg7T>::result>::result Arg7_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg8T>::result>::result Arg8_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)(Arg0T,Arg1T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0,arg1);
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0, Arg1T arg1, Arg2T arg2)
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg3T>::result>::result Arg3_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg4T>::result>::result Arg4_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg5T>::result>::result Arg5_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg6T>::result>::result Arg6_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg7T>::result>::result Arg7_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg8T>::result>::result Arg8_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)(Arg0T,Arg1T,Arg2T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0,arg1,arg2);
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0, Arg1T arg1, Arg2T arg2, Arg3T arg3)
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg4T>::result>::result Arg4_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg5T>::result>::result Arg5_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg6T>::result>::result Arg6_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg7T>::result>::result Arg7_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg8T>::result>::result Arg8_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)(Arg0T,Arg1T,Arg2T,Arg3T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0,arg1,arg2,arg3);
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0, Arg1T arg1, Arg2T arg2, Arg3T arg3,
                                                  Arg4T arg4)
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg5T>::result>::result Arg5_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg6T>::result>::result Arg6_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg7T>::result>::result Arg7_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg8T>::result>::result Arg8_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0,arg1,arg2,arg3,arg4);
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0, Arg1T arg1, Arg2T arg2, Arg3T arg3,
                                                  Arg4T arg4, Arg5T arg5)
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg6T>::result>::result Arg6_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg7T>::result>::result Arg7_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg8T>::result>::result Arg8_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0,arg1,arg2,arg3,arg4,arg5);
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0, Arg1T arg1, Arg2T arg2, Arg3T arg3,
                                                  Arg4T arg4, Arg5T arg5, Arg6T arg6)
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg7T>::result>::result Arg7_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg8T>::result>::result Arg8_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0, Arg1T arg1, Arg2T arg2, Arg3T arg3,
                                                  Arg4T arg4, Arg5T arg5, Arg6T arg6, Arg7T arg7)
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg8T>::result>::result Arg8_test;
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7);
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0, Arg1T arg1, Arg2T arg2, Arg3T arg3,
                                                  Arg4T arg4, Arg5T arg5, Arg6T arg6, Arg7T arg7,
                                                  Arg8T arg8)
    {
      typedef typename check_is_CPArgVoid<is_CPArgVoid<Arg9T>::result>::result Arg9_test;
      typedef ReturnT (*funcT)(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
    }
    __device__ __forceinline__ ReturnT operator()(Arg0T arg0, Arg1T arg1, Arg2T arg2, Arg3T arg3,
                                                  Arg4T arg4, Arg5T arg5, Arg6T arg6, Arg7T arg7,
                                                  Arg8T arg8, Arg9T arg9)
    {
      typedef ReturnT (*funcT)(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T,Arg9T);
      funcT call = (funcT)optix::rt_callable_program_from_id(m_id);
      return call(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9);
    }
  protected:
    int m_id;
  };
} // end namespace rti_internal_callableprogram

namespace optix {

  /* RT_INTERNAL_CALLABLE_PROGRAM_DEFS is a helper macro to define the body of each
   * version of the callableProgramId class. Variadic macro arguments are our friend here,
   * so we can use arguments such as (ReturnT,Arg0T) and (ReturnT,Arg0T,Arg1T) and get the
   * correct template types defined.
   */
#define RT_INTERNAL_CALLABLE_PROGRAM_DEFS(...) public rti_internal_callableprogram::callableProgramIdBase<__VA_ARGS__> \
  {                                                                     \
  public:                                                               \
    /* Default constructor */                                           \
    __device__ __forceinline__ callableProgramId() {}                   \
    /* Constructor that initializes the id with null.*/                 \
    __device__ __forceinline__ callableProgramId(RTprogramidnull nullid) \
      : rti_internal_callableprogram::callableProgramIdBase<__VA_ARGS__>(nullid) {} \
    /* Constructor that initializes the id.*/                           \
    __device__ __forceinline__ explicit callableProgramId(int id)       \
      : rti_internal_callableprogram::callableProgramIdBase<__VA_ARGS__>(id) {} \
    /* assigment that initializes the id with null. */                  \
    __device__ __forceinline__ callableProgramId& operator= (RTprogramidnull nullid) \
      { this->m_id = nullid; return *this; } \
    /* Return the id */                                                 \
    __device__ __forceinline__ int getId() const { return this->m_id; } \
    /* Return whether the id is valid */                                \
    __device__ __forceinline__ operator bool() const \
    { return this->m_id != RT_PROGRAM_ID_NULL; } \
  }

  /* callableProgramId should not be used directly.  Use rtCallableProgramId instead to
   * make sure compatibility with future versions of OptiX is maintained.
   */

  /* The default template version is left undefined on purpose.  Only the specialized
   * versions should be used. */
  template<typename T>
  class callableProgramId;

  /* These are specializations designed to be used like: <ReturnT(argument types)> */
  template<typename ReturnT>
  class callableProgramId<ReturnT()>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT);
  template<typename ReturnT, typename Arg0T>
  class callableProgramId<ReturnT(Arg0T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T);
  template<typename ReturnT, typename Arg0T, typename Arg1T>
  class callableProgramId<ReturnT(Arg0T,Arg1T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T>
  class callableProgramId<ReturnT(Arg0T,Arg1T,Arg2T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T>
  class callableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T>
  class callableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T>
  class callableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T, typename Arg6T>
  class callableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T, typename Arg6T, typename Arg7T>
  class callableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T, typename Arg6T, typename Arg7T, typename Arg8T>
  class callableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T, typename Arg6T, typename Arg7T, typename Arg8T, typename Arg9T>
  class callableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T,Arg9T)>: RT_INTERNAL_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T,Arg9T);
  
  /* RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS is a helper macro to define the body of each
   * version of the boundCallableProgramId class. Variadic macro arguments are our friend
   * here, so we can use arguments such as (ReturnT,Arg0T) and (ReturnT,Arg0T,Arg1T) and
   * get the correct template types defined.
   *
   * Also, the constructors (except the default) and operators are made private, because
   * the objects should not be set, copied or otherwise changed from the value set by
   * OptiX from the host.
   *
   * The getId and bool operator (from the parent) are redefined and made private, because
   * the internal ID (m_id) should never be accessible.  Using this ID is likely to cause
   * problems, because OptiX is free to compile the called program in a method that would
   * be incompatible with bindless callable programs.
   */
#if (CUDART_VERSION >= 4010)
#define RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(...) public rti_internal_callableprogram::callableProgramIdBase<__VA_ARGS__> \
  {                                                                     \
  public:                                                               \
    /* Default constructor */                                           \
    __device__ __forceinline__ boundCallableProgramId() {}              \
  private:                                                              \
    /* No copying of this class*/                                       \
    __device__ __forceinline__ boundCallableProgramId(const boundCallableProgramId& ); \
    __device__ __forceinline__ boundCallableProgramId& operator= (const boundCallableProgramId& ); \
  }
#else
#define RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(...) public rti_internal_callableprogram::callableProgramIdBase<__VA_ARGS__> \
  {                                                                     \
  }
#endif
  
  /* boundCallableProgramId should not be used directly.  Use rtCallableProgramX
   * instead to make sure compatibility with future versions of OptiX is maintained.
   */

  /* The default template version is left undefined on purpose.  Only the specialized
   * versions should be used. */
  template<typename T>
  class boundCallableProgramId;

  /* These are specializations designed to be used like: <ReturnT(argument types)> */
  template<typename ReturnT>
  class boundCallableProgramId<ReturnT()>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT);
  template<typename ReturnT, typename Arg0T>
  class boundCallableProgramId<ReturnT(Arg0T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T);
  template<typename ReturnT, typename Arg0T, typename Arg1T>
  class boundCallableProgramId<ReturnT(Arg0T,Arg1T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T>
  class boundCallableProgramId<ReturnT(Arg0T,Arg1T,Arg2T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T>
  class boundCallableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T>
  class boundCallableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T>
  class boundCallableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T, typename Arg6T>
  class boundCallableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T, typename Arg6T, typename Arg7T>
  class boundCallableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T, typename Arg6T, typename Arg7T, typename Arg8T>
  class boundCallableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T);
  template<typename ReturnT, typename Arg0T, typename Arg1T, typename Arg2T, typename Arg3T,
           typename Arg4T, typename Arg5T, typename Arg6T, typename Arg7T, typename Arg8T, typename Arg9T>
  class boundCallableProgramId<ReturnT(Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T,Arg9T)>: RT_INTERNAL_BOUND_CALLABLE_PROGRAM_DEFS(ReturnT,Arg0T,Arg1T,Arg2T,Arg3T,Arg4T,Arg5T,Arg6T,Arg7T,Arg8T,Arg9T);
  
} // end namespace optix

namespace rti_internal_typeinfo {
  // Specialization for callableProgramId types
  template <typename T>
  struct rti_typeenum<optix::callableProgramId<T> >
  {
    static const int m_typeenum = _OPTIX_TYPE_ENUM_PROGRAM_ID;
  };

  // Specialization for boundCallableProgramId types
  template <typename T>
  struct rti_typeenum<optix::boundCallableProgramId<T> >
  {
    static const int m_typeenum = _OPTIX_TYPE_ENUM_PROGRAM_AS_ID;
  };
  
}

/**
  * @brief Callable Program ID Declaration
  * 
  * @ingroup CUDACDeclarations
  * 
  * <B>Description</B>
  * 
  * @ref rtCallableProgramId declares callable program \a name, which will appear
  * to be a callable function with the specified return type and list of arguments.
  * This callable program must be matched against a
  * variable declared on the API object of type int.
  *
  * Example(s):
  *
  *@code
  *  rtDeclareVariable(rtCallableProgramId<float3(float3, float)>, modColor);
  *  rtBuffer<rtCallableProgramId<float3(float3, float)>, 1> modColors;
  *@endcode
  * 
  * <B>History</B>
  * 
  * @ref rtCallableProgramId was introduced in OptiX 3.6.
  * 
  * <B>See also</B>
  * @ref rtCallableProgram
  * @ref rtCallableProgramX
  * @ref rtDeclareVariable
  * 
  */
#define rtCallableProgramId  optix::callableProgramId

/**
  * @brief Callable Program X Declaration
  * 
  * @ingroup CUDACDeclarations
  * 
  * <B>Description</B>
  * 
  * @ref rtCallableProgramX declares callable program \a name, which will appear
  * to be a callable function with the specified return type and list of arguments.
  * This callable program must be matched against a
  * variable declared on the API object using @ref rtVariableSetObject.
  *
  * Unless compatibility with SM_10 is needed, new code should \#define
  * RT_USE_TEMPLATED_RTCALLABLEPROGRAM and rely on the new templated version of
  * rtCallableProgram instead of directly using rtCallableProgramX.
  *
  * Example(s):
  *
  *@code
  *  rtDeclareVariable(rtCallableProgramX<float3(float3, float)>, modColor);
  *  // With RT_USE_TEMPLATED_RTCALLABLEPROGRAM defined
  *  rtDeclareVariable(rtCallableProgram<float3(float3, float)>, modColor);
  *@endcode
  * 
  * <B>History</B>
  * 
  * @ref rtCallableProgramX was introduced in OptiX 3.6.
  * 
  * <B>See also</B>
  * @ref rtCallableProgram
  * @ref rtCallableProgramId
  * @ref rtDeclareVariable
  * 
  */
#define rtCallableProgramX  optix::boundCallableProgramId
#ifdef RT_USE_TEMPLATED_RTCALLABLEPROGRAM
#  undef  rtCallableProgram
#  define rtCallableProgram optix::boundCallableProgramId
#endif

/*
   Functions
*/

 /**
  * @brief Traces a ray
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtTrace traces \a ray against object \a topNode.  A reference to
  * \a prd, the per-ray data, will be passed to all of the closest-hit and any-hit programs
  * that are executed during this invocation of trace. \a topNode must refer
  * to an OptiX object of type @ref RTgroup, @ref RTselector, @ref RTgeometrygroup or @ref RTtransform.
  *
  * @param[in] topNode  Top node object where to start the traversal
  * @param[in] ray      Ray to be traced
  * @param[in] prd      Per-ray custom data
  *
  * @retval void    void return value
  * 
  * <B>History</B>
  * 
  * @ref rtTrace was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtObject
  * @ref Ray
  * 
  */
template<class T>
static inline __device__ void rtTrace( rtObject topNode, optix::Ray ray, T& prd )
{
  optix::rt_trace(*(unsigned int*)&topNode, ray.origin, ray.direction, ray.ray_type, ray.tmin, ray.tmax, &prd, sizeof(T));
}

/**
  * @brief Determine whether a computed intersection is potentially valid
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * Reporting an intersection from a geometry program is a two-stage
  * process.  If the geometry program computes that the ray intersects the
  * geometry, it will first call @ref rtPotentialIntersection.
  * @ref rtPotentialIntersection will determine whether the reported hit distance
  * is within the valid interval associated with the ray, and return true
  * if the intersection is valid. Subsequently, the geometry program will
  * compute the attributes (normal, texture coordinates, etc.) associated
  * with the intersection before calling @ref rtReportIntersection.  When
  * @ref rtReportIntersection is called, the any-hit program associated with
  * the material is called.  If the any-hit program does not ignore the
  * intersection then the \b t value will stand as the new closest
  * intersection.
  *
  * If @ref rtPotentialIntersection returns true, then
  * @ref rtReportIntersection should \b always be called after computing the
  * attributes.  Furthermore, attributes variables should only be written
  * after a successful return from @ref rtPotentialIntersection.
  *
  * @ref rtPotentialIntersection is passed the material index associated
  * with the reported intersection.  Objects with a single material should
  * pass an index of zero.
  *
  * @ref rtReportIntersection and @ref rtPotentialIntersection are valid only
  * within a geometry intersection program.
  * 
  * @param[in] tmin  t value of the ray to be checked
  *
  * @retval  bool   Returns whether the intersection is valid or not
  * 
  * <B>History</B>
  * 
  * @ref rtPotentialIntersection was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtGeometrySetIntersectionProgram,
  * @ref rtReportIntersection,
  * @ref rtIgnoreIntersection
  */
static inline __device__ bool rtPotentialIntersection( float tmin )
{
  return optix::rt_potential_intersection( tmin );
}

/**
  * @brief Report an intersection with the current object and the specified material
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtReportIntersection reports an intersection of the current ray
  * with the current object, and specifies the material associated with
  * the intersection.  @ref rtReportIntersection should only be used in
  * conjunction with @ref rtPotentialIntersection as described in
  * @ref rtPotentialIntersection.
  *
  * @param[in] material   Material associated with the intersection
  *
  * @retval bool  return value, this is set to \a false if the intersection is, for some reason, ignored
  * <B>History</B>
  * 
  * @ref rtReportIntersection was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtPotentialIntersection,
  * @ref rtIgnoreIntersection
  */
static inline __device__ bool rtReportIntersection( unsigned int material )
{
  return optix::rt_report_intersection( material );
}

/**
  * @brief Cancels the potential intersection with current ray
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtIgnoreIntersection causes the current potential intersection to
  * be ignored.  This intersection will not become the new closest hit
  * associated with the ray. This function does not return, so values
  * affecting the per-ray data should be applied before calling
  * @ref rtIgnoreIntersection.  @ref rtIgnoreIntersection is valid only within an
  * any-hit program.
  *
  * @ref rtIgnoreIntersection can be used to implement alpha-mapped
  * transparency by ignoring intersections that hit the geometry but are
  * labeled as transparent in a texture.  Since any-hit programs are called
  * frequently during intersection, care should be taken to make them as
  * efficient as possible.
  * 
  * 
  * @retval  void   void return value
  * 
  * <B>History</B>
  * 
  * @ref rtIgnoreIntersection was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtTerminateRay,
  * @ref rtPotentialIntersection
  */
static inline __device__ void rtIgnoreIntersection()
{
  optix::rt_ignore_intersection();
}

/**
  * @brief Terminate traversal associated with the current ray
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtTerminateRay causes the traversal associated with the current ray
  * to immediately terminate.  After termination, the closest-hit program
  * associated with the ray will be called.  This function does not
  * return, so values affecting the per-ray data should be applied before
  * calling @ref rtTerminateRay.  @ref rtTerminateRay is valid only within an any-hit
  * program. The value of rtIntersectionDistance is undefined when @ref rtTerminateRay is used.
  * 
  * @retval  void   void return value
  * 
  * <B>History</B>
  * 
  * @ref rtTerminateRay was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtIgnoreIntersection,
  * @ref rtPotentialIntersection
  */
static inline __device__ void rtTerminateRay()
{
  optix::rt_terminate_ray();
}

/**
  * @brief Visit child of selector
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtIntersectChild will perform intersection on the specified child
  * for the current active ray.  This is used in a selector visit program
  * to traverse one of the selector's children.  The \a index specifies
  * which of the children to be visited.  As the child is traversed,
  * intersection programs will be called and any-hit programs will be
  * called for positive intersections.  When this process is complete,
  * @ref rtIntersectChild will return unless one of the any-hit programs calls
  * @ref rtTerminateRay, in which case this function will never return.
  * Multiple children can be visited during a single selector visit call
  * by calling this function multiple times.
  *
  * \a index matches the index used in @ref rtSelectorSetChild on the
  * host. @ref rtIntersectChild is valid only within a selector visit
  * program.
  * 
  * @param[in] index  Specifies the child to perform intersection on
  *
  * @retval  void   void return value
  * 
  * <B>History</B>
  * 
  * @ref rtIntersectChild was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtSelectorSetVisitProgram,
  * @ref rtSelectorCreate,
  * @ref rtTerminateRay
  */
static inline __device__ void rtIntersectChild( unsigned int index )
{
  optix::rt_intersect_child( index );
}

/**
  * @brief Apply the current transformation to a point
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtTransformPoint transforms \a p as a point using the current
  * active transformation stack.  During traversal, intersection and
  * any-hit programs, the current ray will be located in object space.
  * During ray generation, closest-hit and miss programs, the current ray
  * will be located in world space.  This function can be used to
  * transform the ray origin and other points between object and world space.
  *
  * \a kind is an enumerated value that can be either
  * @ref RT_OBJECT_TO_WORLD or @ref RT_WORLD_TO_OBJECT and must be a constant
  * literal.  For ray generation and miss programs, the transform will
  * always be the identity transform.  For traversal, intersection,
  * any-hit and closest-hit programs, the transform will be dependent on
  * the set of active transform nodes for the current state.
  * 
  * @param[in] kind  Type of the transform
  * @param[in] p     Point to transform
  *
  * @retval  float3   Transformed point
  * 
  * <B>History</B>
  * 
  * @ref rtTransformPoint was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtTransformCreate,
  * @ref rtTransformVector,
  * @ref rtTransformNormal
  */
static inline __device__ float3 rtTransformPoint( RTtransformkind kind, const float3& p )
{
  return optix::rt_transform_point( kind, p );
}

/**
  * @brief Apply the current transformation to a vector
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtTransformVector transforms \a v as a vector using the current
  * active transformation stack.  During traversal, intersection and
  * any-hit programs, the current ray will be located in object space.
  * During ray generation, closest-hit and miss programs, the current ray
  * will be located in world space.  This function can be used to
  * transform the ray direction and other vectors between object and world
  * space.
  * 
  * \a kind is an enumerated value that can be either
  * @ref RT_OBJECT_TO_WORLD or @ref RT_WORLD_TO_OBJECT and must be a constant
  * literal.  For ray generation and miss programs, the transform will
  * always be the identity transform.  For traversal, intersection,
  * any-hit and closest-hit programs, the transform will be dependent on
  * the set of active transform nodes for the current state.
  * 
  * @param[in] kind  Type of the transform
  * @param[in] v     Vector to transform
  *
  * @retval  float3   Transformed vector
  * 
  * <B>History</B>
  * 
  * @ref rtTransformVector was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtTransformCreate,
  * @ref rtTransformPoint,
  * @ref rtTransformNormal
  */
static inline __device__ float3 rtTransformVector( RTtransformkind kind, const float3& v )
{
  return optix::rt_transform_vector( kind, v );
}

/**
  * @brief Apply the current transformation to a normal
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtTransformNormal transforms \a n as a normal using the current
  * active transformation stack (the inverse transpose).  During
  * traversal, intersection and any-hit programs, the current ray will be
  * located in object space.  During ray generation, closest-hit and miss
  * programs, the current ray will be located in world space.  This
  * function can be used to transform values between object and world
  * space.
  *
  * \a kind is an enumerated value that can be either
  * @ref RT_OBJECT_TO_WORLD or @ref RT_WORLD_TO_OBJECT and must be a constant
  * literal.  For ray generation and miss programs, the transform will
  * always be the identity transform.  For traversal, intersection,
  * any-hit and closest-hit programs, the transform will be dependent on
  * the set of active transform nodes for the current state.
  * 
  * @param[in] kind  Type of the transform
  * @param[in] n     Normal to transform
  *
  * @retval  float3   Transformed normal
  * 
  * <B>History</B>
  * 
  * @ref rtTransformNormal was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtTransformCreate,
  * @ref rtTransformPoint,
  * @ref rtTransformVector
  */
static inline __device__ float3 rtTransformNormal( RTtransformkind kind, const float3& n )
{
  return optix::rt_transform_normal( kind, n );
}

/**
  * @brief Get requested transform
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtGetTransform returns the requested transform in the return parameter
  * \a matrix.  The type of transform to be retrieved is specified with the
  * \a kind parameter.  \a kind is an enumerated value that can be either
  * @ref RT_OBJECT_TO_WORLD or @ref RT_WORLD_TO_OBJECT and must be a constant literal.
  * During traversal, intersection and any-hit programs, the current ray will be
  * located in object space.  During ray generation, closest-hit and miss programs,
  * the current ray will be located in world space.
  *
  * There may be significant performance overhead associated with a call to 
  * @ref rtGetTransform compared to a call to @ref rtTransformPoint, @ref rtTransformVector,
  * or @ref rtTransformNormal.
  * 
  * @param[in]    kind    The type of transform to retrieve
  * @param[out]   matrix  Return parameter for the requested transform
  * 
  * @retval  void   void return value
  * 
  * <B>History</B>
  * 
  * @ref rtGetTransform was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtTransformCreate,
  * @ref rtTransformPoint,
  * @ref rtTransformVector,
  * @ref rtTransformNormal
  * 
  */
static inline __device__ void rtGetTransform( RTtransformkind kind, float matrix[16] )
{
  return optix::rt_get_transform( kind, matrix );
}


/*
   Printing
*/

/**
  * @brief Prints text to the standard output
  * 
  * @ingroup rtPrintf
  * 
  * <B>Description</B>
  * 
  * @ref rtPrintf is used to output text from within user programs. Arguments are passed
  * as for the standard C \a printf function, and the same format strings are employed. The
  * only exception is the "%s" format specifier, which will generate an error if used.
  * Text printed using @ref rtPrintf is accumulated in a buffer and printed to the standard
  * output when @ref rtContextLaunch "rtContextLaunch" finishes. The buffer size can be configured using
  * @ref rtContextSetPrintBufferSize. Output can optionally be restricted to certain
  * launch indices using @ref rtContextSetPrintLaunchIndex.
  * Printing must be enabled using @ref rtContextSetPrintEnabled, otherwise @ref rtPrintf
  * invocations will be silently ignored.
  *
  * <B>History</B>
  * 
  * @ref rtPrintf was introduced in OptiX 1.0.
  * 
  * <B>See also</B>
  * @ref rtContextSetPrintEnabled,
  * @ref rtContextGetPrintEnabled,
  * @ref rtContextSetPrintBufferSize,
  * @ref rtContextGetPrintBufferSize,
  * @ref rtContextSetPrintLaunchIndex,
  * @ref rtContextSetPrintLaunchIndex
  * 
  */
  /** @{ */

static inline __device__ void rtPrintf( const char* fmt )
{
  _RT_PRINT_ACTIVE()
  printf(fmt);
}
template<typename T1>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1);
}
template<typename T1, typename T2>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2);
}
template<typename T1, typename T2, typename T3>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3);
}
template<typename T1, typename T2, typename T3, typename T4>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3, T4 arg4 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3, arg4);
}
template<typename T1, typename T2, typename T3, typename T4, typename T5>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3, arg4, arg5);
}
template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3, arg4, arg5, arg6);
}
template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}
template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}
template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}
template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
}
template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
}
template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
static inline __device__ void rtPrintf( const char* fmt, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12 )
{
  _RT_PRINT_ACTIVE()
  printf(fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);
}
/** @} */

/** @cond */
namespace rti_internal_register {
  extern __device__ void* reg_bitness_detector;
  extern __device__ volatile unsigned long long reg_exception_64_detail0;
  extern __device__ volatile unsigned long long reg_exception_64_detail1;
  extern __device__ volatile unsigned long long reg_exception_64_detail2;
  extern __device__ volatile unsigned long long reg_exception_64_detail3;
  extern __device__ volatile unsigned long long reg_exception_64_detail4;
  extern __device__ volatile unsigned long long reg_exception_64_detail5;
  extern __device__ volatile unsigned long long reg_exception_64_detail6;
  extern __device__ volatile unsigned long long reg_exception_64_detail7;
  extern __device__ volatile unsigned long long reg_exception_64_detail8;
  extern __device__ volatile unsigned long long reg_exception_64_detail9;
  extern __device__ volatile unsigned int reg_exception_detail0;
  extern __device__ volatile unsigned int reg_exception_detail1;
  extern __device__ volatile unsigned int reg_exception_detail2;
  extern __device__ volatile unsigned int reg_exception_detail3;
  extern __device__ volatile unsigned int reg_exception_detail4;
  extern __device__ volatile unsigned int reg_exception_detail5;
  extern __device__ volatile unsigned int reg_exception_detail6;
  extern __device__ volatile unsigned int reg_exception_detail7;
  extern __device__ volatile unsigned int reg_exception_detail8;
  extern __device__ volatile unsigned int reg_exception_detail9;
  extern __device__ volatile unsigned int reg_rayIndex_x;
  extern __device__ volatile unsigned int reg_rayIndex_y;
  extern __device__ volatile unsigned int reg_rayIndex_z;
}
/** @endcond */


/**
  * @brief Throw a user exception
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtThrow is used to trigger user defined exceptions which behave like built-in
  * exceptions. That is, upon invocation, ray processing for the current launch index
  * is immediately aborted and the corresponding exception program is executed. @ref rtThrow
  * does not return.
  *
  * The \a code passed as argument must be within the range reserved for user exceptions,
  * which starts at @ref RT_EXCEPTION_USER (\a 0x400) and ends at \a 0xFFFF. The code can
  * be queried within the exception program using @ref rtGetExceptionCode.
  *
  * @ref rtThrow may be called from within any program type except exception programs. Calls
  * to @ref rtThrow will be silently ignored unless user exceptions are enabled using
  * @ref rtContextSetExceptionEnabled.
  * 
  * <B>History</B>
  * 
  * @ref rtThrow was introduced in OptiX 1.1.
  * 
  * <B>See also</B>
  * @ref rtContextSetExceptionEnabled,
  * @ref rtContextGetExceptionEnabled,
  * @ref rtContextSetExceptionProgram,
  * @ref rtContextGetExceptionProgram,
  * @ref rtGetExceptionCode,
  * @ref rtPrintExceptionDetails
  * 
  */
static inline __device__ void rtThrow( unsigned int code )
{
  optix::rt_throw( code );
}

/**
  * @brief Retrieves the type of a caught exception
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtGetExceptionCode can be called from an exception program to query which type
  * of exception was caught. The returned code is equivalent to one of the @ref RTexception
  * constants passed to @ref rtContextSetExceptionEnabled, @ref RT_EXCEPTION_ALL excluded.
  * For user-defined exceptions, the code is equivalent to the argument passed to @ref rtThrow.
  * 
  * @retval unsigned int  Returned exception code
  * 
  * <B>History</B>
  * 
  * @ref rtGetExceptionCode was introduced in OptiX 1.1.
  * 
  * <B>See also</B>
  * @ref rtContextSetExceptionEnabled,
  * @ref rtContextGetExceptionEnabled,
  * @ref rtContextSetExceptionProgram,
  * @ref rtContextGetExceptionProgram,
  * @ref rtThrow,
  * @ref rtPrintExceptionDetails
  * 
  */
static inline __device__ unsigned int rtGetExceptionCode()
{
  return optix::rt_get_exception_code();
}

/**
  * @brief Print information on a caught exception
  * 
  * @ingroup CUDACFunctions
  * 
  * <B>Description</B>
  * 
  * @ref rtGetExceptionCode can be called from an exception program to provide information
  * on the caught exception to the user. The function uses @ref rtPrintf to output details
  * depending on the type of the exception. It is necessary to have printing enabled
  * using @ref rtContextSetPrintEnabled for this function to have any effect.
  * 
  * @retval void  void return type
  * 
  * <B>History</B>
  * 
  * @ref rtPrintExceptionDetails was introduced in OptiX 1.1.
  * 
  * <B>See also</B>
  * @ref rtContextSetExceptionEnabled,
  * @ref rtContextGetExceptionEnabled,
  * @ref rtContextSetExceptionProgram,
  * @ref rtContextGetExceptionProgram,
  * @ref rtContextSetPrintEnabled,
  * @ref rtGetExceptionCode,
  * @ref rtThrow,
  * @ref rtPrintf
  * 
  */
static inline __device__ void rtPrintExceptionDetails()
{
  const unsigned int code = rtGetExceptionCode();

  if( code == RT_EXCEPTION_STACK_OVERFLOW )
  {
    rtPrintf( "Caught RT_EXCEPTION_STACK_OVERFLOW\n"
              "  launch index : %d, %d, %d\n",
              rti_internal_register::reg_rayIndex_x,
              rti_internal_register::reg_rayIndex_y,
              rti_internal_register::reg_rayIndex_z
              );
  }
  else if( code == RT_EXCEPTION_BUFFER_INDEX_OUT_OF_BOUNDS )
  {
#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
    const unsigned int dim = rti_internal_register::reg_exception_detail0;

    rtPrintf( "Caught RT_EXCEPTION_BUFFER_INDEX_OUT_OF_BOUNDS\n"
              "  launch index   : %d, %d, %d\n"
              "  buffer address : 0x%llX\n"
              "  dimensionality : %d\n"
              "  size           : %lldx%lldx%lld\n"
              "  element size   : %d\n"
              "  accessed index : %lld, %lld, %lld\n",
              rti_internal_register::reg_rayIndex_x,
              rti_internal_register::reg_rayIndex_y,
              rti_internal_register::reg_rayIndex_z,
              rti_internal_register::reg_exception_64_detail0,
              rti_internal_register::reg_exception_detail0,
              rti_internal_register::reg_exception_64_detail1,
              dim > 1 ? rti_internal_register::reg_exception_64_detail2 : 1,
              dim > 2 ? rti_internal_register::reg_exception_64_detail3 : 1,
              rti_internal_register::reg_exception_detail1,
              rti_internal_register::reg_exception_64_detail4,
              rti_internal_register::reg_exception_64_detail5,
              rti_internal_register::reg_exception_64_detail6
              );
#else
    const unsigned int dim = rti_internal_register::reg_exception_detail1;

    rtPrintf( "Caught RT_EXCEPTION_BUFFER_INDEX_OUT_OF_BOUNDS\n"
              "  launch index   : %d, %d, %d\n"
              "  buffer address : 0x%X\n"
              "  dimensionality : %d\n"
              "  size           : %dx%dx%d\n"
              "  element size   : %d\n"
              "  accessed index : %d, %d, %d\n",
              rti_internal_register::reg_rayIndex_x,
              rti_internal_register::reg_rayIndex_y,
              rti_internal_register::reg_rayIndex_z,
              rti_internal_register::reg_exception_detail0,
              rti_internal_register::reg_exception_detail1,
              rti_internal_register::reg_exception_detail2,
              dim > 1 ? rti_internal_register::reg_exception_detail3 : 1,
              dim > 2 ? rti_internal_register::reg_exception_detail4 : 1,
              rti_internal_register::reg_exception_detail5,
              rti_internal_register::reg_exception_detail6,
              rti_internal_register::reg_exception_detail7,
              rti_internal_register::reg_exception_detail8
              );
#endif
  }
  else if( code == RT_EXCEPTION_PROGRAM_ID_INVALID )
  {
    rtPrintf( "Caught RT_EXCEPTION_PROGRAM_ID_INVALID\n");
    switch(rti_internal_register::reg_exception_detail1)
    {
    case 0:
      rtPrintf( "\tprogram ID equal to RT_PROGRAM_ID_NULL used\n");
      break;
    case 1:
      rtPrintf( "\tprogram ID (%d) is not in the valid range of [1,size)\n", rti_internal_register::reg_exception_detail0);
      break;
    case 2:
      rtPrintf( "\tprogram ID of a deleted program used\n");
      break;
    }
  }
  else if( code == RT_EXCEPTION_TEXTURE_ID_INVALID )
  {
    rtPrintf( "Caught RT_EXCEPTION_TEXTURE_ID_INVALID\n");
    switch(rti_internal_register::reg_exception_detail1)
    {
    case 0:
      rtPrintf( "\ttexture ID (%d) is invalid (0)\n", rti_internal_register::reg_exception_detail0);
      break;
    case 1:
      rtPrintf( "\ttexture ID (%d) is not in the valid range of [1,size)\n", rti_internal_register::reg_exception_detail0);
      break;
    case 2:
      rtPrintf( "\ttexture ID (%d) is invalid (-1)\n", rti_internal_register::reg_exception_detail0);
      break;
    }
  }
  else if( code == RT_EXCEPTION_BUFFER_ID_INVALID )
  {
    rtPrintf( "Caught RT_EXCEPTION_BUFFER_ID_INVALID\n");
    switch(rti_internal_register::reg_exception_detail1)
    {
    case 0:
      rtPrintf( "\tbuffer ID equal to RT_BUFFER_ID_NULL used\n");
      break;
    case 1:
      rtPrintf( "\tbuffer ID (%d) is not in the valid range of [1,size)\n", rti_internal_register::reg_exception_detail0);
      break;
    case 2:
      rtPrintf( "\tBuffer ID of a deleted buffer used\n");
      break;
    }
  }
  else if( code == RT_EXCEPTION_INDEX_OUT_OF_BOUNDS )
  {
#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
    const unsigned int dim = rti_internal_register::reg_exception_detail0;

    rtPrintf( "Caught RT_EXCEPTION_INDEX_OUT_OF_BOUNDS\n"
              "  launch index   : %d, %d, %d\n"
              "  buffer address : 0x%llX\n"
              "  size           : %lld\n"
              "  accessed index : %lld\n",
              rti_internal_register::reg_rayIndex_x,
              rti_internal_register::reg_rayIndex_y,
              rti_internal_register::reg_rayIndex_z,
              rti_internal_register::reg_exception_64_detail0,
              rti_internal_register::reg_exception_64_detail1,
              rti_internal_register::reg_exception_64_detail2
              );
#else
    const unsigned int dim = rti_internal_register::reg_exception_detail1;

    rtPrintf( "Caught RT_EXCEPTION_INDEX_OUT_OF_BOUNDS\n"
              "  launch index   : %d, %d, %d\n"
              "  buffer address : 0x%X\n"
              "  size           : %d\n"
              "  accessed index : %d\n",
              rti_internal_register::reg_rayIndex_x,
              rti_internal_register::reg_rayIndex_y,
              rti_internal_register::reg_rayIndex_z,
              rti_internal_register::reg_exception_detail0,
              rti_internal_register::reg_exception_detail1,
              rti_internal_register::reg_exception_detail2
              );
#endif
  }
  else if( code == RT_EXCEPTION_INVALID_RAY )
  {
    rtPrintf( "Caught RT_EXCEPTION_INVALID_RAY\n"
              "  launch index  : %d, %d, %d\n"
              "  ray origin    : %f %f %f\n"
              "  ray direction : %f %f %f\n"
              "  ray type      : %d\n"
              "  ray tmin      : %f\n"
              "  ray tmax      : %f\n",
              rti_internal_register::reg_rayIndex_x,
              rti_internal_register::reg_rayIndex_y,
              rti_internal_register::reg_rayIndex_z,
              __int_as_float(rti_internal_register::reg_exception_detail0),
              __int_as_float(rti_internal_register::reg_exception_detail1),
              __int_as_float(rti_internal_register::reg_exception_detail2),
              __int_as_float(rti_internal_register::reg_exception_detail3),
              __int_as_float(rti_internal_register::reg_exception_detail4),
              __int_as_float(rti_internal_register::reg_exception_detail5),
              rti_internal_register::reg_exception_detail6,
              __int_as_float(rti_internal_register::reg_exception_detail7),
              __int_as_float(rti_internal_register::reg_exception_detail8)
              );
  }
  else if( code == RT_EXCEPTION_INTERNAL_ERROR )
  {
    // Should never happen.
    rtPrintf( "Caught RT_EXCEPTION_INTERNAL_ERROR\n"
              "  launch index : %d, %d, %d\n"
              "  error id     : %d\n",
              rti_internal_register::reg_rayIndex_x,
              rti_internal_register::reg_rayIndex_y,
              rti_internal_register::reg_rayIndex_z,
              rti_internal_register::reg_exception_detail0
              );
  }
  else if( code >= RT_EXCEPTION_USER && code <= 0xFFFF )
  {
    rtPrintf( "Caught RT_EXCEPTION_USER+%d\n"
              "  launch index : %d, %d, %d\n",
              code-RT_EXCEPTION_USER,
              rti_internal_register::reg_rayIndex_x,
              rti_internal_register::reg_rayIndex_y,
              rti_internal_register::reg_rayIndex_z
              );
  }
  else
  {
    // Should never happen.
    rtPrintf( "Caught unknown exception\n"
              "  launch index : %d, %d, %d\n",
              rti_internal_register::reg_rayIndex_x,
              rti_internal_register::reg_rayIndex_y,
              rti_internal_register::reg_rayIndex_z
              );
  }
}

#endif /* __optix_optix_cuda__internal_h__ */
