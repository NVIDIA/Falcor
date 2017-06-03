#ifndef RASTER_RENDERER_SYNTAX_H
#define RASTER_RENDERER_SYNTAX_H

#include "../CoreLib/Basic.h"
#include "Lexer.h"
#include "IL.h"

#include "../../Spire.h"

#include <assert.h>

namespace Spire
{
    namespace Compiler
    {
        using namespace CoreLib::Basic;
        class SyntaxVisitor;
        class FunctionSyntaxNode;

#if 0
        // We use a unified representation for modifiers on all declarations.
        // (Eventually this will also apply to statements that support attributes)
        //
        // The parser allows any set of modifiers on any declaration, and we leave
        // it to later phases of analysis to reject inappropriate uses.
        // TODO: implement rejection properly.
        //
        // Some common modifiers (thos represented by single keywords) are
        // specified via a simple set of flags.
        // TODO: consider using a real AST even for these, so we can give good
        // error messages on confliction modifiers.
        typedef unsigned int ModifierFlags;
        enum ModifierFlag : ModifierFlags
        {
            None = 0,
            Uniform = 1 << 0,
            Out = 1 << 1,
            In = 1 << 2,
            Const = 1 << 4,
            Instance = 1 << 5,
            Builtin = 1 << 6,

            Inline = 1 << 8,
            Public = 1 << 9,
            Require = 1 << 10,
            Param = (1 << 11) | ModifierFlag::Public,
            Extern = 1 << 12,
            Input = 1 << 13,
            Intrinsic = (1 << 14) | ModifierFlag::Extern,
            // TODO(tfoley): This should probably be its own flag
            InOut = ModifierFlag::In | ModifierFlag::Out,

            Transparent = 1 << 15,
            FromStdlib = 1 << 16,
        };
#endif
        //
        // Other modifiers may have more elaborate data, and so
        // are represented as heap-allocated objects, in a linked
        // list.
        //
        class Modifier : public RefObject
        {
        public:
            // Next modifier in linked list of modifiers on same piece of syntax
            RefPtr<Modifier> next;

            // Source code location where modifier was written
            //
            // TODO(tfoley): `Modifier` should probably extend `SyntaxNode` at some
            // point, but I'm avoiding it for now because `SyntaxNode` has additional
            // baggage I don't want to have to deal with.
            CodePosition Position;
        };

#define SIMPLE_MODIFIER(NAME) \
        class NAME##Modifier : public Modifier {}

        SIMPLE_MODIFIER(Uniform);
        SIMPLE_MODIFIER(In);
        SIMPLE_MODIFIER(Out);
        SIMPLE_MODIFIER(Const);
        SIMPLE_MODIFIER(Instance);
        SIMPLE_MODIFIER(Builtin);
        SIMPLE_MODIFIER(Inline);
        SIMPLE_MODIFIER(Public);
        SIMPLE_MODIFIER(Require);
        SIMPLE_MODIFIER(Param);
        SIMPLE_MODIFIER(Extern);
        SIMPLE_MODIFIER(Input);
        SIMPLE_MODIFIER(Intrinsic);
        SIMPLE_MODIFIER(Transparent);
        SIMPLE_MODIFIER(FromStdLib);

#undef SIMPLE_MODIFIER

        class InOutModifier : public OutModifier {};

        // This is a special sentinel modifier that gets added
        // to the list when we have multiple variable declarations
        // all sharing the same modifiers:
        //
        //     static uniform int a : FOO, *b : register(x0);
        //
        // In this case both `a` and `b` share the syntax
        // for part of their modifier list, but then have
        // their own modifiers as well:
        //
        //     a: SemanticModifier("FOO") --> SharedModifiers --> StaticModifier --> UniformModifier
        //                                 /
        //     b: RegisterModifier("x0")  /
        //
        class SharedModifiers : public Modifier {};

        // A `layout` modifier
        class LayoutModifier : public Modifier
        {
        public:
            String LayoutString;
        };

        // An HLSL semantic
        class HLSLSemantic : public Modifier
        {
        public:
            Token name;
        };


        // An HLSL semantic that affects layout
        class HLSLLayoutSemantic : public HLSLSemantic
        {
        public:
            Token registerName;
            Token componentMask;
        };

        // An HLSL `register` semantic
        class HLSLRegisterSemantic : public HLSLLayoutSemantic
        {
        };

        // TODO(tfoley): `packoffset`
        class HLSLPackOffsetSemantic : public HLSLLayoutSemantic
        {
        };

        // An HLSL semantic that just associated a declaration with a semantic name
        class HLSLSimpleSemantic : public HLSLSemantic
        {
        };

        // A set of modifiers attached to a syntax node
        struct Modifiers
        {
            // The first modifier in the linked list of heap-allocated modifiers
            RefPtr<Modifier> first;
        };

        // Helper class for iterating over a list of heap-allocated modifiers
        struct ModifierList
        {
            struct Iterator
            {
                Modifier* current;

                Modifier* operator*()
                {
                    return current;
                }

                void operator++()
                {
                    current = current->next.Ptr();
                }

                bool operator!=(Iterator other)
                {
                    return current != other.current;
                };

                Iterator()
                    : current(nullptr)
                {}

                Iterator(Modifier* modifier)
                    : current(modifier)
                {}
            };

            ModifierList()
                : modifiers(nullptr)
            {}

            ModifierList(Modifier* modifiers)
                : modifiers(modifiers)
            {}

            Iterator begin() { return Iterator(modifiers); }
            Iterator end() { return Iterator(nullptr); }

            Modifier* modifiers;
        };

        // Helper class for iterating over heap-allocated modifiers
        // of a specific type.
        template<typename T>
        struct FilteredModifierList
        {
            struct Iterator
            {
                Modifier* current;

                T* operator*()
                {
                    return (T*)current;
                }

                void operator++()
                {
                    current = Adjust(current->next.Ptr());
                }

                bool operator!=(Iterator other)
                {
                    return current != other.current;
                };

                Iterator()
                    : current(nullptr)
                {}

                Iterator(Modifier* modifier)
                    : current(modifier)
                {}
            };

            FilteredModifierList()
                : modifiers(nullptr)
            {}

            FilteredModifierList(Modifier* modifiers)
                : modifiers(Adjust(modifiers))
            {}

            Iterator begin() { return Iterator(modifiers); }
            Iterator end() { return Iterator(nullptr); }

            static Modifier* Adjust(Modifier* modifier)
            {
                Modifier* m = modifier;
                for (;;)
                {
                    if (!m) return m;
                    if (dynamic_cast<T*>(m)) return m;
                    m = m->next.Ptr();
                }
            }

            Modifier* modifiers;
        };

        enum class BaseType
        {
            // Note(tfoley): These are ordered in terms of promotion rank, so be vareful when messing with this

            Void = 0,
            Bool,
            Int,
            UInt,
            UInt64,
            Float,
#if 0
            Texture2D = 48,
            TextureCube = 49,
            Texture2DArray = 50,
            Texture2DShadow = 51,
            TextureCubeShadow = 52,
            Texture2DArrayShadow = 53,
            Texture3D = 54,
            SamplerState = 4096, SamplerComparisonState = 4097,
            Error = 16384,
#endif
        };

        class Decl;
        class StructSyntaxNode;
        class BasicExpressionType;
        class ArrayExpressionType;
        class TypeDefDecl;
        class DeclRefType;
        class NamedExpressionType;
        class TypeExpressionType;
        class VectorExpressionType;
        class MatrixExpressionType;
        class ArithmeticExpressionType;
        class GenericDecl;
        class Substitutions;
        class TextureType;
        class SamplerStateType;

        // A compile-time constant value (usually a type)
        class Val : public RefObject
        {
        public:
            // construct a new value by applying a set of parameter
            // substitutions to this one
            RefPtr<Val> Substitute(Substitutions* subst);

            // Lower-level interface for substition. Like the basic
            // `Substitute` above, but also takes a by-reference
            // integer parameter that should be incremented when
            // returning a modified value (this can help the caller
            // decide whether they need to do anything).
            virtual RefPtr<Val> SubstituteImpl(Substitutions* subst, int* ioDiff);

            virtual bool EqualsVal(Val* val) = 0;
            virtual String ToString() const = 0;
            virtual int GetHashCode() const = 0;
            bool operator == (const Val & v)
            {
                return EqualsVal(const_cast<Val*>(&v));
            }
        };

        // A compile-time integer (may not have a specific concrete value)
        class IntVal : public Val
        {
        };

        // Try to extract a simple integer value from an `IntVal`.
        // This fill assert-fail if the object doesn't represent a literal value.
        int GetIntVal(RefPtr<IntVal> val);

        // Trivial case of a value that is just a constant integer
        class ConstantIntVal : public IntVal
        {
        public:
            int value;

            ConstantIntVal(int value)
                : value(value)
            {}

            virtual bool EqualsVal(Val* val) override;
            virtual String ToString() const override;
            virtual int GetHashCode() const override;
        };

        // TODO(tfoley): classes for more general compile-time integers,
        // including references to template parameters

        // A type, representing a classifier for some term in the AST.
        //
        // Types can include "sugar" in that they may refer to a
        // `typedef` which gives them a good name when printed as
        // part of diagnostic messages.
        //
        // In order to operation on types, though, we often want
        // to look past any sugar, and operate on an underlying
        // "canonical" type. The reprsentation caches a pointer to
        // a canonical type on every type, so we can easily
        // operate on the raw representation when needed.
        class ExpressionType : public Val
        {
        public:
#if 0
            static RefPtr<ExpressionType> Bool;
            static RefPtr<ExpressionType> UInt;
            static RefPtr<ExpressionType> Int;
            static RefPtr<ExpressionType> Float;
            static RefPtr<ExpressionType> Float2;
            static RefPtr<ExpressionType> Void;
#endif
            static RefPtr<ExpressionType> Error;
            static RefPtr<ExpressionType> Overloaded;

            static Dictionary<int, RefPtr<ExpressionType>> sBuiltinTypes;
            static Dictionary<String, Decl*> sMagicDecls;

            // Note: just exists to make sure we can clean up
            // canonical types we create along the way
            static List<RefPtr<ExpressionType>> sCanonicalTypes;



            static ExpressionType* GetBool();
            static ExpressionType* GetFloat();
            static ExpressionType* GetInt();
            static ExpressionType* GetUInt();
            static ExpressionType* GetVoid();
            static ExpressionType* GetError();

        public:
            virtual String ToString() const = 0;

            bool IsIntegral() const;
            bool Equals(const ExpressionType * type) const;
            bool Equals(RefPtr<ExpressionType> type) const;

            bool IsVectorType() const { return As<VectorExpressionType>() != nullptr; }
            bool IsArray() const { return As<ArrayExpressionType>() != nullptr; }

            template<typename T>
            T* As() const
            {
                return dynamic_cast<T*>(GetCanonicalType());
            }

            // Convenience/legacy wrappers for `As<>`
            ArithmeticExpressionType * AsArithmeticType() const { return As<ArithmeticExpressionType>(); }
            BasicExpressionType * AsBasicType() const { return As<BasicExpressionType>(); }
            VectorExpressionType * AsVectorType() const { return As<VectorExpressionType>(); }
            MatrixExpressionType * AsMatrixType() const { return As<MatrixExpressionType>(); }
            ArrayExpressionType * AsArrayType() const { return As<ArrayExpressionType>(); }
            DeclRefType* AsDeclRefType() const { return As<DeclRefType>(); }
            TypeExpressionType* AsTypeType() const { return As<TypeExpressionType>(); }

            NamedExpressionType* AsNamedType() const;

            bool IsTextureOrSampler() const;
            bool IsTexture() const { return As<TextureType>() != nullptr; }
            bool IsSampler() const { return As<SamplerStateType>() != nullptr; }
            bool IsStruct() const;
            bool IsClass() const;
            static void Init();
            static void Finalize();
            ExpressionType* GetCanonicalType() const;
            BindableResourceType GetBindableResourceType() const;

            virtual RefPtr<Val> SubstituteImpl(Substitutions* subst, int* ioDiff) override;

        protected:
            virtual bool EqualsVal(Val* val) override;
            virtual bool IsIntegralImpl() const { return false; }
            virtual bool EqualsImpl(const ExpressionType * type) const = 0;

            virtual ExpressionType* CreateCanonicalType() = 0;
            ExpressionType* canonicalType = nullptr;
        };

        // A substitution represents a binding of certain
        // type-level variables to concrete argument values
        class Substitutions : public RefObject
        {
        public:
            // The generic declaration that defines the
            // parametesr we are binding to arguments
            GenericDecl*	genericDecl;

            // The actual values of the arguments
            List<RefPtr<Val>> args;

            // Any further substitutions, relating to outer generic declarations
            RefPtr<Substitutions> outer;

            // Apply a set of substitutions to the bindings in this substitution
            RefPtr<Substitutions> SubstituteImpl(Substitutions* subst, int* ioDiff);

            // Check if these are equivalent substitutiosn to another set
            bool Equals(Substitutions* subst);
            bool operator == (const Substitutions & subst)
            {
                return Equals(const_cast<Substitutions*>(&subst));
            }
            int GetHashCode() const
            {
                int rs = 0;
                for (auto && v : args)
                {
                    rs ^= v->GetHashCode();
                    rs *= 16777619;
                }
                return rs;
            }
        };

        class SyntaxNode : public RefObject
        {
        public:
            CodePosition Position;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) = 0;
        };

        class ContainerDecl;
        class SpecializeModifier;

        // Represents how much checking has been applied to a declaration.
        enum class DeclCheckState : uint8_t
        {
            // The declaration has been parsed, but not checked
            Unchecked,

            // We are in the process of checking the declaration "header"
            // (those parts of the declaration needed in order to
            // reference it)
            CheckingHeader,

            // We are done checking the declaration header.
            CheckedHeader,

            // We have checked the declaration fully.
            Checked,
        };

        // A syntax node which can have modifiers appled
        class ModifiableSyntaxNode : public SyntaxNode
        {
        public:
            Modifiers modifiers;

            template<typename T>
            FilteredModifierList<T> GetModifiersOfType() { return FilteredModifierList<T>(modifiers.first.Ptr()); }

            // Find the first modifier of a given type, or return `nullptr` if none is found.
            template<typename T>
            T* FindModifier()
            {
                return *GetModifiersOfType<T>().begin();
            }

            template<typename T>
            bool HasModifier() { return FindModifier<T>() != nullptr; }
        };


        // An intermediate type to represent either a single declaration, or a group of declarations
        class DeclBase : public ModifiableSyntaxNode
        {
        public:
        };

        class Decl : public DeclBase
        {
        public:
            ContainerDecl*  ParentDecl;

            Token Name;
            String const& getName() { return Name.Content; }
            Token const& getNameToken() { return Name; }


            DeclCheckState checkState = DeclCheckState::Unchecked;

            // The next declaration defined in the same container with the same name
            Decl* nextInContainerWithSameName = nullptr;

            bool IsChecked(DeclCheckState state) { return checkState >= state; }
            void SetCheckState(DeclCheckState state)
            {
                assert(state >= checkState);
                checkState = state;
            }
        };

        struct QualType
        {
            RefPtr<ExpressionType>	type;
            bool					IsLeftValue;

            QualType()
                : IsLeftValue(false)
            {}

            QualType(RefPtr<ExpressionType> type)
                : type(type)
                , IsLeftValue(false)
            {}

            QualType(ExpressionType* type)
                : type(type)
                , IsLeftValue(false)
            {}

            void operator=(RefPtr<ExpressionType> t)
            {
                *this = QualType(t);
            }

            void operator=(ExpressionType* t)
            {
                *this = QualType(t);
            }

            ExpressionType* Ptr() { return type.Ptr(); }

            operator RefPtr<ExpressionType>() { return type; }
            RefPtr<ExpressionType> operator->() { return type; }
        };

        class ExpressionSyntaxNode : public SyntaxNode
        {
        public:
            QualType Type;
            ExpressionSyntaxNode()
            {}
        };




        // A reference to a declaration, which may include
        // substitutions for generic parameters.
        struct DeclRef
        {
            typedef Decl DeclType;

            // The underlying declaration
            RefPtr<Decl> decl;
            Decl* GetDecl() const { return decl.Ptr(); }

            // Optionally, a chain of substititions to perform
            RefPtr<Substitutions> substitutions;

            DeclRef()
            {}

            DeclRef(Decl* decl, RefPtr<Substitutions> substitutions)
                : decl(decl)
                , substitutions(substitutions)
            {}

            // Apply substitutions to a type or ddeclaration
            RefPtr<ExpressionType> Substitute(RefPtr<ExpressionType> type) const;
            DeclRef Substitute(DeclRef declRef) const;

            // Apply substitutions to an expression
            RefPtr<ExpressionSyntaxNode> Substitute(RefPtr<ExpressionSyntaxNode> expr) const;

            // Apply substitutions to this declaration reference
            DeclRef SubstituteImpl(Substitutions* subst, int* ioDiff);

            // Check if this is an equivalent declaration reference to another
            bool Equals(DeclRef const& declRef) const;
            bool operator == (const DeclRef& other) const
            {
                return Equals(other);
            }

            // Convenience accessors for common properties of declarations
            String const& GetName() const;
            DeclRef GetParent() const;

            // "dynamic cast" to a more specific declaration reference type
            template<typename T>
            T As() const
            {
                T result;
                result.decl = dynamic_cast<T::DeclType*>(decl.Ptr());
                result.substitutions = substitutions;
                return result;
            }

            // Implicit conversion mostly so we can use a `DeclRef`
            // in a conditional context
            operator Decl*() const
            {
                return decl.Ptr();
          }

            int GetHashCode() const;
        };

        // Helper macro for defining `DeclRef` subtypes
        #define SPIRE_DECLARE_DECL_REF(D)				    \
            typedef D DeclType;							    \
            D* GetDecl() const { return (D*) decl.Ptr(); }	\
            /* */



        // The type of a reference to an overloaded name
        class OverloadGroupType : public ExpressionType
        {
        public:
            virtual String ToString() const override;

        protected:
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
            virtual int GetHashCode() const override;
        };

        // The type of an expression that was erroneous
        class ErrorType : public ExpressionType
        {
        public:
            virtual String ToString() const override;

        protected:
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
            virtual int GetHashCode() const override;
        };

        // A type that takes the form of a reference to some declaration
        class DeclRefType : public ExpressionType
        {
        public:
            DeclRef declRef;

            virtual String ToString() const override;
            virtual RefPtr<Val> SubstituteImpl(Substitutions* subst, int* ioDiff) override;

            static DeclRefType* Create(DeclRef declRef);

        protected:
            DeclRefType()
            {}
            DeclRefType(DeclRef declRef)
                : declRef(declRef)
            {}
            virtual int GetHashCode() const override;
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
        };

        // Base class for types that can be used in arithmetic expressions
        class ArithmeticExpressionType : public DeclRefType
        {
        public:
            virtual BasicExpressionType* GetScalarType() const = 0;
        };

        class FunctionDeclBase;

        // A reference to the generic type parameter of an import operator
        class ImportOperatorGenericParamType : public ExpressionType
        {
        public:
            String GenericTypeVar;

            ImportOperatorGenericParamType(String genericTypeVar)
                : GenericTypeVar(genericTypeVar)
            {}

            virtual String ToString() const override;
        protected:
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
        };

        class BasicExpressionType : public ArithmeticExpressionType
        {
        public:
            BaseType BaseType;

            BasicExpressionType()
            {
                BaseType = Compiler::BaseType::Int;
            }
            BasicExpressionType(Compiler::BaseType baseType)
            {
                BaseType = baseType;
            }
            virtual CoreLib::Basic::String ToString() const override;
        protected:
            virtual BasicExpressionType* GetScalarType() const override;
            virtual bool IsIntegralImpl() const override;
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
        };


        class TextureType : public DeclRefType
        {
        public:
            // The type that results from fetching an element from this texture
            RefPtr<ExpressionType> elementType;

            // Bits representing the kind of texture type we are looking at
            // (e.g., `Texture2DMS` vs. `TextureCubeArray`)
            typedef uint16_t Flavor;
            Flavor flavor;

            enum
            {
                // Mask for the overall "shape" of the texture
                ShapeMask		= SPIRE_RESOURCE_BASE_SHAPE_MASK,

                // Flag for whether the shape has "array-ness"
                ArrayFlag		= SPIRE_TEXTURE_ARRAY_FLAG,

                // Whether or not the texture stores multiple samples per pixel
                MultisampleFlag	= SPIRE_TEXTURE_MULTISAMPLE_FLAG,

                // Whether or not this is a shadow texture
                //
                // TODO(tfoley): is this even meaningful/used?
                // ShadowFlag		= 0x80, 
            };

            enum Shape : uint8_t
            {
                Shape1D			= SPIRE_TEXTURE_1D,
                Shape2D			= SPIRE_TEXTURE_2D,
                Shape3D			= SPIRE_TEXTURE_3D,
                ShapeCube		= SPIRE_TEXTURE_CUBE,

                Shape1DArray	= Shape1D | ArrayFlag,
                Shape2DArray	= Shape2D | ArrayFlag,
                // No Shape3DArray
                ShapeCubeArray	= ShapeCube | ArrayFlag,
            };
            

            Shape GetBaseShape() const { return Shape(flavor & ShapeMask); }
            bool isArray() const { return (flavor & ArrayFlag) != 0; }
            bool isMultisample() const { return (flavor & MultisampleFlag) != 0; }
//            bool isShadow() const { return (flavor & ShadowFlag) != 0; }

            SpireResourceShape getShape() const { return flavor & 0xFF; }
            SpireResourceAccess getAccess() const { return (flavor >> 8) & 0xFF; }

            TextureType(
                Flavor flavor,
                RefPtr<ExpressionType> elementType)
                : elementType(elementType)
                , flavor(flavor)
            {}

            virtual RefPtr<Val> SubstituteImpl(Substitutions* subst, int* ioDiff) override;
        };

        class SamplerStateType : public DeclRefType
        {
        public:
            // What flavor of sampler state is this
            enum class Flavor : uint8_t
            {
                SamplerState,
                SamplerComparisonState,
            };
            Flavor flavor;
        };

        // Other cases of generic types known to the compiler
        class BuiltinGenericType : public DeclRefType
        {
        public:
            RefPtr<ExpressionType> elementType;
        };

        // Types that behave like pointers, in that they can be
        // dereferenced (implicitly) to access members defined
        // in the element type.
        class PointerLikeType : public BuiltinGenericType
        {};

        // Types that behave like arrays, in that they can be
        // subscripted (explicitly) to access members defined
        // in the element type.
        class ArrayLikeType : public BuiltinGenericType
        {};

        // Generic types used in existing Spire code
        // TODO(tfoley): check that these are actually working right...
        class PatchType : public PointerLikeType {};
        class StorageBufferType : public ArrayLikeType {};
        class UniformBufferType : public PointerLikeType {};
        class PackedBufferType : public ArrayLikeType {};

        // HLSL buffer-type resources

        class HLSLBufferType : public ArrayLikeType {};
        class HLSLRWBufferType : public ArrayLikeType {};
        class HLSLStructuredBufferType : public ArrayLikeType {};
        class HLSLRWStructuredBufferType : public ArrayLikeType {};

        class UntypedBufferResourceType : public DeclRefType {};
        class HLSLByteAddressBufferType : public UntypedBufferResourceType {};
        class HLSLRWByteAddressBufferType : public UntypedBufferResourceType {};

        class HLSLAppendStructuredBufferType : public BuiltinGenericType {};
        class HLSLConsumeStructuredBufferType : public BuiltinGenericType {};

        class HLSLInputPatchType : public ArrayLikeType {};
        class HLSLOutputPatchType : public ArrayLikeType {};

        // HLSL geometry shader output stream types

        class HLSLStreamOutputType : public BuiltinGenericType {};
        class HLSLPointStreamType : public HLSLStreamOutputType {};
        class HLSLLineStreamType : public HLSLStreamOutputType {};
        class HLSLTriangleStreamType : public HLSLStreamOutputType {};

        // Type for HLSL `cbuffer` declarations, and `ConstantBuffer<T>`
        class ConstantBufferType : public PointerLikeType {};

        // Type for HLSL `tbuffer` declarations, and `TextureBuffer<T>`
        class TextureBufferType : public PointerLikeType {};

        class ArrayExpressionType : public ExpressionType
        {
        public:
            RefPtr<ExpressionType> BaseType;
            RefPtr<IntVal> ArrayLength;
            virtual CoreLib::Basic::String ToString() const override;
        protected:
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
            virtual int GetHashCode() const override;
        };

        // The "type" of an expression that resolves to a type.
        // For example, in the expression `float(2)` the sub-expression,
        // `float` would have the type `TypeType(float)`.
        class TypeExpressionType : public ExpressionType
        {
        public:
            TypeExpressionType(RefPtr<ExpressionType> type)
                : type(type)
            {}

            // The type that this is the type of...
            RefPtr<ExpressionType> type;


            virtual String ToString() const override;

        protected:
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
            virtual int GetHashCode() const override;
        };

        class GenericDecl;

        // A vector type, e.g., `vector<T,N>`
        class VectorExpressionType : public ArithmeticExpressionType
        {
        public:
            VectorExpressionType(
                RefPtr<ExpressionType>	elementType,
                RefPtr<IntVal>			elementCount)
                : elementType(elementType)
                , elementCount(elementCount)
            {}

            // The type of vector elements.
            // As an invariant, this should be a basic type or an alias.
            RefPtr<ExpressionType>	elementType;

            // The number of elements
            RefPtr<IntVal>			elementCount;

            virtual String ToString() const override;
            virtual RefPtr<Val> SubstituteImpl(Substitutions* subst, int* ioDiff) override;

        protected:
            virtual BasicExpressionType* GetScalarType() const override;
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
        };

        // A matrix type, e.g., `matrix<T,R,C>`
        class MatrixExpressionType : public ArithmeticExpressionType
        {
        public:
            MatrixExpressionType(
                RefPtr<ExpressionType>	elementType,
                RefPtr<IntVal>			rowCount,
                RefPtr<IntVal>			colCount)
                : elementType(elementType)
                , rowCount(rowCount)
                , colCount(colCount)
            {}

            // The type of vector elements.
            // As an invariant, this should be a basic type or an alias.
            RefPtr<ExpressionType>			elementType;

            // The type of the matrix rows
            RefPtr<VectorExpressionType>	rowType;

            // The number of rows and columns
            RefPtr<IntVal>					rowCount;
            RefPtr<IntVal>					colCount;

            virtual String ToString() const override;
            virtual RefPtr<Val> SubstituteImpl(Substitutions* subst, int* ioDiff) override;

        protected:
            virtual BasicExpressionType* GetScalarType() const override;
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;

        };

        inline BaseType GetVectorBaseType(VectorExpressionType* vecType) {
            return vecType->elementType->AsBasicType()->BaseType;
        }

        inline int GetVectorSize(VectorExpressionType* vecType)
        {
            auto constantVal = vecType->elementCount.As<ConstantIntVal>();
            if (constantVal)
                return constantVal->value;
            // TODO: what to do in this case?
            return 0;
        }

        class Type
        {
        public:
            RefPtr<ExpressionType> DataType;
            // ContrainedWorlds: Implementation must be defined at at least one of of these worlds in order to satisfy global dependency
            // FeasibleWorlds: The component can be computed at any of these worlds
            EnumerableHashSet<String> ConstrainedWorlds, FeasibleWorlds;
            EnumerableHashSet<String> PinnedWorlds;
        };

        class ContainerDecl;


        // A group of declarations that should be treated as a unit
        class DeclGroup : public DeclBase
        {
        public:
            List<RefPtr<Decl>> decls;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        template<typename T>
        struct FilteredMemberList
        {
            typedef RefPtr<Decl> Element;

            FilteredMemberList()
                : mBegin(NULL)
                , mEnd(NULL)
            {}

            explicit FilteredMemberList(
                List<Element> const& list)
                : mBegin(Adjust(list.begin(), list.end()))
                , mEnd(list.end())
            {}

            struct Iterator
            {
                Element* mCursor;
                Element* mEnd;

                bool operator!=(Iterator const& other)
                {
                    return mCursor != other.mCursor;
                }

                void operator++()
                {
                    mCursor = Adjust(mCursor + 1, mEnd);
                }

                RefPtr<T>& operator*()
                {
                    return *(RefPtr<T>*)mCursor;
                }
            };

            Iterator begin()
            {
                Iterator iter = { mBegin, mEnd };
                return iter;
            }

            Iterator end()
            {
                Iterator iter = { mEnd, mEnd };
                return iter;
            }

            static Element* Adjust(Element* cursor, Element* end)
            {
                while (cursor != end)
                {
                    if ((*cursor).As<T>())
                        return cursor;
                    cursor++;
                }
                return cursor;
            }

            // TODO(tfoley): It is ugly to have these.
            // We should probably fix the call sites instead.
            RefPtr<T>& First() { return *begin(); }
            int Count()
            {
                int count = 0;
                for (auto iter : (*this))
                {
                    (void)iter;
                    count++;
                }
                return count;
            }

            List<RefPtr<T>> ToArray()
            {
                List<RefPtr<T>> result;
                for (auto element : (*this))
                {
                    result.Add(element);
                }
                return result;
            }

            Element* mBegin;
            Element* mEnd;
        };

        struct TransparentMemberInfo
        {
            // The declaration of the transparent member
            Decl*	decl;
        };

        // A "container" decl is a parent to other declarations
        class ContainerDecl : public Decl
        {
        public:
            List<RefPtr<Decl>> Members;

            template<typename T>
            FilteredMemberList<T> GetMembersOfType()
            {
                return FilteredMemberList<T>(Members);
            }


            // Dictionary for looking up members by name.
            // This is built on demand before performing lookup.
            Dictionary<String, Decl*> memberDictionary;

            // Whether the `memberDictionary` is valid.
            // Should be set to `false` if any members get added/remoed.
            bool memberDictionaryIsValid = false;

            // A list of transparent members, to be used in lookup
            // Note: this is only valid if `memberDictionaryIsValid` is true
            List<TransparentMemberInfo> transparentMembers;
        };

        template<typename T>
        struct FilteredMemberRefList
        {
            List<RefPtr<Decl>> const&	decls;
            RefPtr<Substitutions>		substitutions;

            FilteredMemberRefList(
                List<RefPtr<Decl>> const&	decls,
                RefPtr<Substitutions>		substitutions)
                : decls(decls)
                , substitutions(substitutions)
            {}

            int Count() const
            {
                int count = 0;
                for (auto d : *this)
                    count++;
                return count;
            }

            List<T> ToArray() const
            {
                List<T> result;
                for (auto d : *this)
                    result.Add(d);
                return result;
            }

            struct Iterator
            {
                FilteredMemberRefList const* list;
                RefPtr<Decl>* ptr;
                RefPtr<Decl>* end;

                Iterator() : list(nullptr), ptr(nullptr) {}
                Iterator(
                    FilteredMemberRefList const* list,
                    RefPtr<Decl>* ptr,
                    RefPtr<Decl>* end)
                    : list(list)
                    , ptr(ptr)
                    , end(end)
                {}

                bool operator!=(Iterator other)
                {
                    return ptr != other.ptr;
                }

                void operator++()
                {
                    ptr = list->Adjust(ptr + 1, end);
                }

                T operator*()
                {
                    return DeclRef(ptr->Ptr(), list->substitutions).As<T>();
                }
            };

            Iterator begin() const { return Iterator(this, Adjust(decls.begin(), decls.end()), decls.end()); }
            Iterator end() const { return Iterator(this, decls.end(), decls.end()); }

            RefPtr<Decl>* Adjust(RefPtr<Decl>* ptr, RefPtr<Decl>* end) const
            {
                while (ptr != end)
                {
                    DeclRef declRef(ptr->Ptr(), substitutions);
                    if (declRef.As<T>())
                        return ptr;
                    ptr++;
                }
                return end;
            }
        };

        struct ContainerDeclRef : DeclRef
        {
            SPIRE_DECLARE_DECL_REF(ContainerDecl);

            FilteredMemberRefList<DeclRef> GetMembers() const
            {
                return FilteredMemberRefList<DeclRef>(GetDecl()->Members, substitutions);
            }

            template<typename T>
            FilteredMemberRefList<T> GetMembersOfType() const
            {
                return FilteredMemberRefList<T>(GetDecl()->Members, substitutions);
            }

        };

        //
        // Type Expressions
        //

        // A "type expression" is a term that we expect to resolve to a type during checking.
        // We store both the original syntax and the resolved type here.
        struct TypeExp
        {
            TypeExp() {}
            TypeExp(TypeExp const& other)
                : exp(other.exp)
                , type(other.type)
            {}
            explicit TypeExp(RefPtr<ExpressionSyntaxNode> exp)
                : exp(exp)
            {}
            TypeExp(RefPtr<ExpressionSyntaxNode> exp, RefPtr<ExpressionType> type)
                : exp(exp)
                , type(type)
            {}

            RefPtr<ExpressionSyntaxNode> exp;
            RefPtr<ExpressionType> type;

            bool Equals(ExpressionType* other) {
                return type->Equals(other);
            }
            bool Equals(RefPtr<ExpressionType> other) {
                return type->Equals(other.Ptr());
            }
            ExpressionType* Ptr() { return type.Ptr(); }
            operator RefPtr<ExpressionType>()
            {
                return type;
            }
            ExpressionType* operator->() { return Ptr(); }

            TypeExp Accept(SyntaxVisitor* visitor);
        };


        //
        // Declarations
        //

        // Base class for all variable-like declarations
        class VarDeclBase : public Decl
        {
        public:
            // Type of the variable
            TypeExp Type;

            ExpressionType* getType() { return Type.type.Ptr(); }

            // Initializer expression (optional)
            RefPtr<ExpressionSyntaxNode> Expr;
        };

        struct VarDeclBaseRef : DeclRef
        {
            SPIRE_DECLARE_DECL_REF(VarDeclBase);

            RefPtr<ExpressionType> GetType() const { return Substitute(GetDecl()->Type); }

            RefPtr<ExpressionSyntaxNode> getInitExpr() const { return Substitute(GetDecl()->Expr); }
        };

        // A field of a `struct` type
        class StructField : public VarDeclBase
        {
        public:
            StructField()
            {}
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct FieldDeclRef : VarDeclBaseRef
        {
            SPIRE_DECLARE_DECL_REF(StructField)
        };

        // An extension to apply to an existing type
        class ExtensionDecl : public ContainerDecl
        {
        public:
            TypeExp targetType;

            // next extension attached to the same nominal type
            ExtensionDecl* nextCandidateExtension = nullptr;


            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct ExtensionDeclRef : ContainerDeclRef
        {
            SPIRE_DECLARE_DECL_REF(ExtensionDecl);

            RefPtr<ExpressionType> GetTargetType() const { return Substitute(GetDecl()->targetType); }
        };

        // Declaration of a type that represents some sort of aggregate
        class AggTypeDecl : public ContainerDecl
        {
        public:
            // extensions that might apply to this declaration
            ExtensionDecl* candidateExtensions = nullptr;
            FilteredMemberList<StructField> GetFields()
            {
                return GetMembersOfType<StructField>();
            }
            StructField* FindField(String name)
            {
                for (auto field : GetFields())
                {
                    if (field->Name.Content == name)
                        return field.Ptr();
                }
                return nullptr;
            }
            int FindFieldIndex(String name)
            {
                int index = 0;
                for (auto field : GetFields())
                {
                    if (field->Name.Content == name)
                        return index;
                    index++;
                }
                return -1;
            }
        };

        struct AggTypeDeclRef : public ContainerDeclRef
        {
            SPIRE_DECLARE_DECL_REF(AggTypeDecl);
            
            ExtensionDecl* GetCandidateExtensions() const { return GetDecl()->candidateExtensions; }
        };

        class StructSyntaxNode : public AggTypeDecl
        {
        public:
            bool IsIntrinsic = false;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct StructDeclRef : public AggTypeDeclRef
        {
            SPIRE_DECLARE_DECL_REF(StructSyntaxNode);

            FilteredMemberRefList<FieldDeclRef> GetFields() const { return GetMembersOfType<FieldDeclRef>(); }
        };

        class ClassSyntaxNode : public AggTypeDecl
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct ClassDeclRef : public AggTypeDeclRef
        {
            SPIRE_DECLARE_DECL_REF(ClassSyntaxNode);

            FilteredMemberRefList<FieldDeclRef> GetFields() const { return GetMembersOfType<FieldDeclRef>(); }
        };

        // A trait which other types can conform to
        class TraitDecl : public AggTypeDecl
        {
        public:
            List<TypeExp> bases;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct TraitDeclRef : public AggTypeDeclRef
        {
            SPIRE_DECLARE_DECL_REF(TraitDecl);
        };


        // A declaration that states that the enclosing type supports a given trait
        //
        // TODO: this same construct might be used for represent other inheritance-like cases
        class TraitConformanceDecl : public Decl
        {
        public:
            // The type expression as written
            TypeExp base;

            // The trait that we found we conform to...
            TraitDeclRef traitDeclRef;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct TraitConformanceDeclRef : public DeclRef
        {
            SPIRE_DECLARE_DECL_REF(TraitConformanceDecl);

            TraitDeclRef GetTraitDeclRef() { return Substitute(GetDecl()->traitDeclRef).As<TraitDeclRef>(); }
        };

        // A declaration that represents a simple (non-aggregate) type
        class SimpleTypeDecl : public Decl
        {
        };

        struct SimpleTypeDeclRef : DeclRef
        {
            SPIRE_DECLARE_DECL_REF(SimpleTypeDecl)
        };

        // A `typedef` declaration
        class TypeDefDecl : public SimpleTypeDecl
        {
        public:
            TypeExp Type;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct TypeDefDeclRef : SimpleTypeDeclRef
        {
            SPIRE_DECLARE_DECL_REF(TypeDefDecl);

            RefPtr<ExpressionType> GetType() const { return Substitute(GetDecl()->Type); }
        };

        // A type alias of some kind (e.g., via `typedef`)
        class NamedExpressionType : public ExpressionType
        {
        public:
            NamedExpressionType(TypeDefDeclRef declRef)
                : declRef(declRef)
            {}

            TypeDefDeclRef declRef;

            virtual String ToString() const override;

        protected:
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
            virtual int GetHashCode() const override;
        };


        class StatementSyntaxNode : public ModifiableSyntaxNode
        {
        public:
        };

        // A scope for local declarations (e.g., as part of a statement)
        class ScopeDecl : public ContainerDecl
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class ScopeStmt : public StatementSyntaxNode
        {
        public:
            RefPtr<ScopeDecl> scopeDecl;
        };

        class BlockStatementSyntaxNode : public ScopeStmt
        {
        public:
            List<RefPtr<StatementSyntaxNode>> Statements;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class UnparsedStmt : public StatementSyntaxNode
        {
        public:
            // The tokens that were contained between `{` and `}`
            List<Token> tokens;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class ParameterSyntaxNode : public VarDeclBase
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct ParamDeclRef : VarDeclBaseRef
        {
            SPIRE_DECLARE_DECL_REF(ParameterSyntaxNode);
        };

        class FunctionDeclBase : public ContainerDecl
        {
        public:
            FilteredMemberList<ParameterSyntaxNode> GetParameters()
            {
                return GetMembersOfType<ParameterSyntaxNode>();
            }
            TypeExp ReturnType;
            RefPtr<StatementSyntaxNode> Body;
        };

        struct FuncDeclBaseRef : ContainerDeclRef
        {
            SPIRE_DECLARE_DECL_REF(FunctionDeclBase);

            RefPtr<ExpressionType> GetResultType() const
            {
                return Substitute(GetDecl()->ReturnType.type.Ptr());
            }

            // TODO: need to apply substitutions here!!!
            FilteredMemberRefList<ParamDeclRef> GetParameters()
            {
                return GetMembersOfType<ParamDeclRef>();
            }
        };

                // Function types are currently used for references to symbols that name
        // either ordinary functions, or "component functions."
        // We do not directly store a representation of the type, and instead
        // use a reference to the symbol to stand in for its logical type
        class FuncType : public ExpressionType
        {
        public:
            FuncDeclBaseRef declRef;

            virtual String ToString() const override;
        protected:
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual ExpressionType* CreateCanonicalType() override;
            virtual int GetHashCode() const override;
        };

        // A constructor/initializer to create instances of a type
        class ConstructorDecl : public FunctionDeclBase
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct ConstructorDeclRef : FuncDeclBaseRef
        {
            SPIRE_DECLARE_DECL_REF(ConstructorDecl);
        };

        class FunctionSyntaxNode : public FunctionDeclBase
        {
        public:
            String InternalName;
            bool IsInline() { return HasModifier<InlineModifier>(); }
            bool IsExtern() { return HasModifier<ExternModifier>(); }
            bool HasSideEffect() { return !HasModifier<IntrinsicModifier>(); }
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
            FunctionSyntaxNode()
            {
            }
        };

        struct FuncDeclRef : FuncDeclBaseRef
        {
            SPIRE_DECLARE_DECL_REF(FunctionSyntaxNode);
        };

        // Base class for expressions that will reference declarations
        class DeclRefExpr : public ExpressionSyntaxNode
        {
        public:
            // The scope in which to perform lookup
            ContainerDecl* scope = nullptr;

            // The declaration of the symbol being referenced
            DeclRef declRef;
        };

        class VarExpressionSyntaxNode : public DeclRefExpr
        {
        public:
            // The name of the symbol being referenced
            String Variable;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // Masks to be applied when lookup up declarations
        enum class LookupMask : uint8_t
        {
            Type = 0x1,
            Function = 0x2,
            Value = 0x4,

            All = Type | Function | Value,
        };

        // Represents one item found during lookup
        struct LookupResultItem
        {
            // Sometimes lookup finds an item, but there were additional
            // "hops" taken to reach it. We need to remember these steps
            // so that if/when we consturct a full expression we generate
            // appropriate AST nodes for all the steps.
            //
            // We build up a list of these "breadcrumbs" while doing
            // lookup, and store them alongside each item found.
            class Breadcrumb : public RefObject
            {
            public:
                enum class Kind
                {
                    Member, // A member was references
                    Deref, // A value with pointer(-like) type was dereferenced
                };

                Kind kind;
                DeclRef declRef;
                RefPtr<Breadcrumb> next;

                Breadcrumb(Kind kind, DeclRef declRef, RefPtr<Breadcrumb> next)
                    : kind(kind)
                    , declRef(declRef)
                    , next(next)
                {}
            };

            // A properly-specialized reference to the declaration that was found.
            DeclRef declRef;

            // Any breadcrumbs needed in order to turn that declaration
            // reference into a well-formed expression.
            //
            // This is unused in the simple case where a declaration
            // is being referenced directly (rather than through
            // transparent members).
            RefPtr<Breadcrumb> breadcrumbs;

            LookupResultItem() = default;
            explicit LookupResultItem(DeclRef declRef)
                : declRef(declRef)
            {}
            LookupResultItem(DeclRef declRef, RefPtr<Breadcrumb> breadcrumbs)
                : declRef(declRef)
                , breadcrumbs(breadcrumbs)
            {}
        };

        // Result of looking up a name in some lexical/semantic environment.
        // Can be used to enumerate all the declarations matching that name,
        // in the case where the result is overloaded.
        struct LookupResult
        {
            // The one item that was found, in the smple case
            LookupResultItem item;

            // All of the items that were found, in the complex case.
            // Note: if there was no overloading, then this list isn't
            // used at all, to avoid allocation.
            List<LookupResultItem> items;

            ContainerDecl*	scope = nullptr;
            ContainerDecl*	endScope = nullptr;
            LookupMask		mask = LookupMask::All;

            // Was at least one result found?
            bool isValid() const { return item.declRef.GetDecl() != nullptr; }

            bool isOverloaded() const { return items.Count() > 1; }
        };

        // An expression that references an overloaded set of declarations
        // having the same name.
        class OverloadedExpr : public ExpressionSyntaxNode
        {
        public:
            // Optional: the base expression is this overloaded result
            // arose from a member-reference expression.
            RefPtr<ExpressionSyntaxNode> base;

            // The lookup result that was ambiguous
            LookupResult lookupResult2;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class ConstantExpressionSyntaxNode : public ExpressionSyntaxNode
        {
        public:
            enum class ConstantType
            {
                Int, Bool, Float
            };
            ConstantType ConstType;
            union
            {
                int IntValue;
                float FloatValue;
            };
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        enum class Operator
        {
            Neg, Not, BitNot, PreInc, PreDec, PostInc, PostDec,
            Mul, Div, Mod,
            Add, Sub,
            Lsh, Rsh,
            Eql, Neq, Greater, Less, Geq, Leq,
            BitAnd, BitXor, BitOr,
            And,
            Or,
            Sequence,
            Select,
            Assign = 200, AddAssign, SubAssign, MulAssign, DivAssign, ModAssign,
            LshAssign, RshAssign, OrAssign, AndAssign, XorAssign,
        };
        String GetOperatorFunctionName(Operator op);
        String OperatorToString(Operator op);

        // An initializer list, e.g. `{ 1, 2, 3 }`
        class InitializerListExpr : public ExpressionSyntaxNode
        {
        public:
            List<RefPtr<ExpressionSyntaxNode>> args;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // A base expression being applied to arguments: covers
        // both ordinary `()` function calls and `<>` generic application
        class AppExprBase : public ExpressionSyntaxNode
        {
        public:
            RefPtr<ExpressionSyntaxNode> FunctionExpr;
            List<RefPtr<ExpressionSyntaxNode>> Arguments;
        };


        class InvokeExpressionSyntaxNode : public AppExprBase
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class OperatorExpressionSyntaxNode : public InvokeExpressionSyntaxNode
        {
        public:
            Operator Operator;
            void SetOperator(ContainerDecl * scope, Spire::Compiler::Operator op);
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class IndexExpressionSyntaxNode : public ExpressionSyntaxNode
        {
        public:
            RefPtr<ExpressionSyntaxNode> BaseExpression;
            RefPtr<ExpressionSyntaxNode> IndexExpression;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class MemberExpressionSyntaxNode : public DeclRefExpr
        {
        public:
            RefPtr<ExpressionSyntaxNode> BaseExpression;
            String MemberName;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class SwizzleExpr : public ExpressionSyntaxNode
        {
        public:
            RefPtr<ExpressionSyntaxNode> base;
            int elementCount;
            int elementIndices[4];

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // A dereference of a pointer or pointer-like type
        class DerefExpr : public ExpressionSyntaxNode
        {
        public:
            RefPtr<ExpressionSyntaxNode> base;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class TypeCastExpressionSyntaxNode : public ExpressionSyntaxNode
        {
        public:
            TypeExp TargetType;
            RefPtr<ExpressionSyntaxNode> Expression;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class SelectExpressionSyntaxNode : public OperatorExpressionSyntaxNode
        {
        public:
        };


        class EmptyStatementSyntaxNode : public StatementSyntaxNode
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class DiscardStatementSyntaxNode : public StatementSyntaxNode
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct Variable : public VarDeclBase
        {
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class VarDeclrStatementSyntaxNode : public StatementSyntaxNode
        {
        public:
            RefPtr<DeclBase> decl;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class UsingFileDecl : public Decl
        {
        public:
            Token fileName;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class ProgramSyntaxNode : public ContainerDecl
        {
        public:
            // Access members of specific types
            FilteredMemberList<UsingFileDecl> GetUsings()
            {
                return GetMembersOfType<UsingFileDecl>();
            }
            FilteredMemberList<FunctionSyntaxNode> GetFunctions()
            {
                return GetMembersOfType<FunctionSyntaxNode>();
            }

            FilteredMemberList<ClassSyntaxNode> GetClasses()
            {
                return GetMembersOfType<ClassSyntaxNode>();
            }
            FilteredMemberList<StructSyntaxNode> GetStructs()
            {
                return GetMembersOfType<StructSyntaxNode>();
            }
            FilteredMemberList<TypeDefDecl> GetTypeDefs()
            {
                return GetMembersOfType<TypeDefDecl>();
            }
#if 0
            void Include(ProgramSyntaxNode * other)
            {
                Members.AddRange(other->Members);
            }
#endif
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class IfStatementSyntaxNode : public StatementSyntaxNode
        {
        public:
            RefPtr<ExpressionSyntaxNode> Predicate;
            RefPtr<StatementSyntaxNode> PositiveStatement;
            RefPtr<StatementSyntaxNode> NegativeStatement;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // A statement that can be escaped with a `break`
        class BreakableStmt : public ScopeStmt
        {};

        class SwitchStmt : public BreakableStmt
        {
        public:
            RefPtr<ExpressionSyntaxNode> condition;
            RefPtr<StatementSyntaxNode> body;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // A statement that is expected to appear lexically nested inside
        // some other construct, and thus needs to keep track of the
        // outer statement that it is associated with...
        class ChildStmt : public StatementSyntaxNode
        {
        public:
            StatementSyntaxNode* parentStmt = nullptr;
        };

        // a `case` or `default` statement inside a `switch`
        //
        // Note(tfoley): A correct AST for a C-like language would treat
        // these as a labelled statement, and so they would contain a
        // sub-statement. I'm leaving that out for now for simplicity.
        class CaseStmtBase : public ChildStmt
        {
        public:
        };

        // a `case` statement inside a `switch`
        class CaseStmt : public CaseStmtBase
        {
        public:
            RefPtr<ExpressionSyntaxNode> expr;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // a `default` statement inside a `switch`
        class DefaultStmt : public CaseStmtBase
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // A statement that represents a loop, and can thus be escaped with a `continue`
        class LoopStmt : public BreakableStmt
        {};

        class ForStatementSyntaxNode : public LoopStmt
        {
        public:
            RefPtr<StatementSyntaxNode> InitialStatement;
            RefPtr<ExpressionSyntaxNode> SideEffectExpression, PredicateExpression;
            RefPtr<StatementSyntaxNode> Statement;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class WhileStatementSyntaxNode : public LoopStmt
        {
        public:
            RefPtr<ExpressionSyntaxNode> Predicate;
            RefPtr<StatementSyntaxNode> Statement;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class DoWhileStatementSyntaxNode : public LoopStmt
        {
        public:
            RefPtr<StatementSyntaxNode> Statement;
            RefPtr<ExpressionSyntaxNode> Predicate;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // The case of child statements that do control flow relative
        // to their parent statement.
        class JumpStmt : public ChildStmt
        {
        public:
            StatementSyntaxNode* parentStmt = nullptr;
        };

        class BreakStatementSyntaxNode : public JumpStmt
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class ContinueStatementSyntaxNode : public JumpStmt
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class ReturnStatementSyntaxNode : public StatementSyntaxNode
        {
        public:
            RefPtr<ExpressionSyntaxNode> Expression;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        class ExpressionStatementSyntaxNode : public StatementSyntaxNode
        {
        public:
            RefPtr<ExpressionSyntaxNode> Expression;
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // Note(tfoley): Moved this further down in the file because it depends on
        // `ExpressionSyntaxNode` and a forward reference just isn't good enough
        // for `RefPtr`.
        //
        class GenericAppExpr : public AppExprBase
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        // An expression representing re-use of the syntax for a type in more
        // than once conceptually-distinct declaration
        class SharedTypeExpr : public ExpressionSyntaxNode
        {
        public:
            // The underlying type expression that we want to share
            TypeExp base;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };


        // A modifier that indicates a built-in base type (e.g., `float`)
        class BuiltinTypeModifier : public Modifier
        {
        public:
            BaseType tag;
        };

        // A modifier that indicates a built-in type that isn't a base type (e.g., `vector`)
        //
        // TODO(tfoley): This deserves a better name than "magic"
        class MagicTypeModifier : public Modifier
        {
        public:
            String name;
            uint32_t tag;
        };

        // Modifiers that affect the storage layout for matrices
        class MatrixLayoutModifier : public Modifier {};

        // Modifiers that specify row- and column-major layout, respectively
        class RowMajorLayoutModifier : public MatrixLayoutModifier {};
        class ColumnMajorLayoutModifier : public MatrixLayoutModifier {};

        // The HLSL flavor of those modifiers
        class HLSLRowMajorLayoutModifier : public RowMajorLayoutModifier {};
        class HLSLColumnMajorLayoutModifier : public ColumnMajorLayoutModifier {};

        // The GLSL flavor of those modifiers
        //
        // Note(tfoley): The GLSL versions of these modifiers are "backwards"
        // in the sense that when a GLSL programmer requests row-major layout,
        // we actually interpret that as requesting column-major. This makes
        // sense because we interpret matrix conventions backwards from how
        // GLSL specifies them.
        class GLSLRowMajorLayoutModifier : public ColumnMajorLayoutModifier {};
        class GLSLColumnMajorLayoutModifier : public RowMajorLayoutModifier {};

        // More HLSL Keyword

        // HLSL `nointerpolation` modifier
        class HLSLNoInterpolationModifier : public Modifier {};

        // HLSL `linear` modifier
        class HLSLLinearModifier : public Modifier {};

        // HLSL `sample` modifier
        class HLSLSampleModifier : public Modifier {};

        // HLSL `centroid` modifier
        class HLSLCentroidModifier : public Modifier {};

        // HLSL `precise` modifier
        class HLSLPreciseModifier : public Modifier {};

        // HLSL `shared` modifier (which is used by the effect system,
        // and shouldn't be confused with `groupshared`)
        class HLSLEffectSharedModifier : public Modifier {};

        // HLSL `groupshared` modifier
        class HLSLGroupSharedModifier : public Modifier {};

        // HLSL `static` modifier (probably doesn't need to be
        // treated as HLSL-specific)
        class HLSLStaticModifier : public Modifier {};

        // HLSL `uniform` modifier (distinct meaning from GLSL
        // use of the keyword)
        class HLSLUniformModifier : public Modifier {};

        // HLSL `volatile` modifier (ignored)
        class HLSLVolatileModifier : public Modifier {};

        // An HLSL `[name(arg0, ...)]` style attribute.
        class HLSLAttribute : public Modifier
        {
        public:
            Token nameToken;
            List<RefPtr<ExpressionSyntaxNode>> args;
        };
        
        // An HLSL `[name(...)]` attribute that hasn't undergone
        // any semantic analysis.
        // After analysis, this might be transformed into a more specific case.
        class HLSLUncheckedAttribute : public HLSLAttribute
        {
        public:
        };

        // An HLSL `[numthreads(x,y,z)]` attribute
        class HLSLNumThreadsAttribute : public HLSLAttribute
        {
        public:
            // The number of threads to use along each axis
            int32_t x;
            int32_t y;
            int32_t z;
        };

        // HLSL modifiers for geometry shader input topology
        class HLSLGeometryShaderInputPrimitiveTypeModifier : public Modifier {};
        class HLSLPointModifier         : public HLSLGeometryShaderInputPrimitiveTypeModifier {};
        class HLSLLineModifier          : public HLSLGeometryShaderInputPrimitiveTypeModifier {};
        class HLSLTriangleModifier      : public HLSLGeometryShaderInputPrimitiveTypeModifier {};
        class HLSLLineAdjModifier       : public HLSLGeometryShaderInputPrimitiveTypeModifier {};
        class HLSLTriangleAdjModifier   : public HLSLGeometryShaderInputPrimitiveTypeModifier {};

        //

        // A generic declaration, parameterized on types/values
        class GenericDecl : public ContainerDecl
        {
        public:
            // The decl that is genericized...
            RefPtr<Decl> inner;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct GenericDeclRef : ContainerDeclRef
        {
            SPIRE_DECLARE_DECL_REF(GenericDecl);

            Decl* GetInner() const { return GetDecl()->inner.Ptr(); }
        };

        // The "type" of an expression that names a generic declaration.
        class GenericDeclRefType : public ExpressionType
        {
        public:
            GenericDeclRefType(GenericDeclRef declRef)
                : declRef(declRef)
            {}

            GenericDeclRef declRef;
            GenericDeclRef const& GetDeclRef() const { return declRef; }

            virtual String ToString() const override;

        protected:
            virtual bool EqualsImpl(const ExpressionType * type) const override;
            virtual int GetHashCode() const override;
            virtual ExpressionType* CreateCanonicalType() override;
        };



        class GenericTypeParamDecl : public SimpleTypeDecl
        {
        public:
            // The bound for the type parameter represents a trait that any
            // type used as this parameter must conform to
//            TypeExp bound;

            // The "initializer" for the parameter represents a default value
            TypeExp initType;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct GenericTypeParamDeclRef : SimpleTypeDeclRef
        {
            SPIRE_DECLARE_DECL_REF(GenericTypeParamDecl);
        };

        // A constraint placed as part of a generic declaration
        class GenericTypeConstraintDecl : public Decl
        {
        public:
            // A type constraint like `T : U` is constraining `T` to be "below" `U`
            // on a lattice of types. This may not be a subtyping relationship
            // per se, but it makes sense to use that terminology here, so we
            // think of these fields as the sub-type and sup-ertype, respectively.
            TypeExp sub;
            TypeExp sup;

            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct GenericTypeConstraintDeclRef : DeclRef
        {
            SPIRE_DECLARE_DECL_REF(GenericTypeConstraintDecl);

            RefPtr<ExpressionType> GetSub() { return Substitute(GetDecl()->sub); }
            RefPtr<ExpressionType> GetSup() { return Substitute(GetDecl()->sup); }
        };


        class GenericValueParamDecl : public VarDeclBase
        {
        public:
            virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
        };

        struct GenericValueParamDeclRef : VarDeclBaseRef
        {
            SPIRE_DECLARE_DECL_REF(GenericValueParamDecl);
        };

        // The logical "value" of a rererence to a generic value parameter
        class GenericParamIntVal : public IntVal
        {
        public:
            GenericValueParamDeclRef declRef;

            GenericParamIntVal(GenericValueParamDeclRef declRef)
                : declRef(declRef)
            {}

            virtual bool EqualsVal(Val* val) override;
            virtual String ToString() const override;
            virtual int GetHashCode() const override;
            virtual RefPtr<Val> SubstituteImpl(Substitutions* subst, int* ioDiff) override;
        };

        //

        class SyntaxVisitor : public Object
        {
        protected:
            DiagnosticSink * sink = nullptr;
            DiagnosticSink* getSink() { return sink; }
        public:
            SyntaxVisitor(DiagnosticSink * sink)
                : sink(sink)
            {}
            virtual RefPtr<ProgramSyntaxNode> VisitProgram(ProgramSyntaxNode* program)
            {
                for (auto & m : program->Members)
                    m = m->Accept(this).As<Decl>();
                return program;
            }

            virtual RefPtr<UsingFileDecl> VisitUsingFileDecl(UsingFileDecl * decl)
            {
                return decl;
            }

            virtual RefPtr<FunctionSyntaxNode> VisitFunction(FunctionSyntaxNode* func)
            {
                func->ReturnType = func->ReturnType.Accept(this);
                for (auto & member : func->Members)
                    member = member->Accept(this).As<Decl>();
                if (func->Body)
                    func->Body = func->Body->Accept(this).As<BlockStatementSyntaxNode>();
                return func;
            }
            virtual RefPtr<ScopeDecl> VisitScopeDecl(ScopeDecl* decl)
            {
                // By default don't visit children, because they will always
                // be encountered in the ordinary flow of the corresponding statement.
                return decl;
            }
            virtual RefPtr<StructSyntaxNode> VisitStruct(StructSyntaxNode * s)
            {
                for (auto & f : s->Members)
                    f = f->Accept(this).As<Decl>();
                return s;
            }
            virtual RefPtr<ClassSyntaxNode> VisitClass(ClassSyntaxNode * s)
            {
                for (auto & f : s->Members)
                    f = f->Accept(this).As<Decl>();
                return s;
            }
            virtual RefPtr<GenericDecl> VisitGenericDecl(GenericDecl * decl)
            {
                for (auto & m : decl->Members)
                    m = m->Accept(this).As<Decl>();
                decl->inner = decl->inner->Accept(this).As<Decl>();
                return decl;
            }
            virtual RefPtr<TypeDefDecl> VisitTypeDefDecl(TypeDefDecl* decl)
            {
                decl->Type = decl->Type.Accept(this);
                return decl;
            }
            virtual RefPtr<StatementSyntaxNode> VisitDiscardStatement(DiscardStatementSyntaxNode * stmt)
            {
                return stmt;
            }
            virtual RefPtr<StructField> VisitStructField(StructField * f)
            {
                f->Type = f->Type.Accept(this);
                return f;
            }
            virtual RefPtr<StatementSyntaxNode> VisitBlockStatement(BlockStatementSyntaxNode* stmt)
            {
                for (auto & s : stmt->Statements)
                    s = s->Accept(this).As<StatementSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitBreakStatement(BreakStatementSyntaxNode* stmt)
            {
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitContinueStatement(ContinueStatementSyntaxNode* stmt)
            {
                return stmt;
            }

            virtual RefPtr<StatementSyntaxNode> VisitDoWhileStatement(DoWhileStatementSyntaxNode* stmt)
            {
                if (stmt->Predicate)
                    stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
                if (stmt->Statement)
                    stmt->Statement = stmt->Statement->Accept(this).As<StatementSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitEmptyStatement(EmptyStatementSyntaxNode* stmt)
            {
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitForStatement(ForStatementSyntaxNode* stmt)
            {
                if (stmt->InitialStatement)
                    stmt->InitialStatement = stmt->InitialStatement->Accept(this).As<StatementSyntaxNode>();
                if (stmt->PredicateExpression)
                    stmt->PredicateExpression = stmt->PredicateExpression->Accept(this).As<ExpressionSyntaxNode>();
                if (stmt->SideEffectExpression)
                    stmt->SideEffectExpression = stmt->SideEffectExpression->Accept(this).As<ExpressionSyntaxNode>();
                if (stmt->Statement)
                    stmt->Statement = stmt->Statement->Accept(this).As<StatementSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitIfStatement(IfStatementSyntaxNode* stmt)
            {
                if (stmt->Predicate)
                    stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
                if (stmt->PositiveStatement)
                    stmt->PositiveStatement = stmt->PositiveStatement->Accept(this).As<StatementSyntaxNode>();
                if (stmt->NegativeStatement)
                    stmt->NegativeStatement = stmt->NegativeStatement->Accept(this).As<StatementSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<SwitchStmt> VisitSwitchStmt(SwitchStmt* stmt)
            {
                if (stmt->condition)
                    stmt->condition = stmt->condition->Accept(this).As<ExpressionSyntaxNode>();
                if (stmt->body)
                    stmt->body = stmt->body->Accept(this).As<BlockStatementSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<CaseStmt> VisitCaseStmt(CaseStmt* stmt)
            {
                if (stmt->expr)
                    stmt->expr = stmt->expr->Accept(this).As<ExpressionSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<DefaultStmt> VisitDefaultStmt(DefaultStmt* stmt)
            {
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitReturnStatement(ReturnStatementSyntaxNode* stmt)
            {
                if (stmt->Expression)
                    stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitVarDeclrStatement(VarDeclrStatementSyntaxNode* stmt)
            {
                stmt->decl = stmt->decl->Accept(this).As<Decl>();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitWhileStatement(WhileStatementSyntaxNode* stmt)
            {
                if (stmt->Predicate)
                    stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
                if (stmt->Statement)
                    stmt->Statement = stmt->Statement->Accept(this).As<StatementSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitExpressionStatement(ExpressionStatementSyntaxNode* stmt)
            {
                if (stmt->Expression)
                    stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
                return stmt;
            }

            virtual RefPtr<ExpressionSyntaxNode> VisitOperatorExpression(OperatorExpressionSyntaxNode* expr)
            {
                for (auto && child : expr->Arguments)
                    child->Accept(this);
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitConstantExpression(ConstantExpressionSyntaxNode* expr)
            {
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitIndexExpression(IndexExpressionSyntaxNode* expr)
            {
                if (expr->BaseExpression)
                    expr->BaseExpression = expr->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
                if (expr->IndexExpression)
                    expr->IndexExpression = expr->IndexExpression->Accept(this).As<ExpressionSyntaxNode>();
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * stmt)
            {
                if (stmt->BaseExpression)
                    stmt->BaseExpression = stmt->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitSwizzleExpression(SwizzleExpr * expr)
            {
                if (expr->base)
                    expr->base->Accept(this);
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitInvokeExpression(InvokeExpressionSyntaxNode* stmt)
            {
                stmt->FunctionExpr->Accept(this);
                for (auto & arg : stmt->Arguments)
                    arg = arg->Accept(this).As<ExpressionSyntaxNode>();
                return stmt;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitTypeCastExpression(TypeCastExpressionSyntaxNode * stmt)
            {
                if (stmt->Expression)
                    stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
                return stmt->Expression;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode* expr)
            {
                return expr;
            }

            virtual RefPtr<ParameterSyntaxNode> VisitParameter(ParameterSyntaxNode* param)
            {
                return param;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitGenericApp(GenericAppExpr* type)
            {
                return type;
            }

            virtual RefPtr<Variable> VisitDeclrVariable(Variable* dclr)
            {
                if (dclr->Expr)
                    dclr->Expr = dclr->Expr->Accept(this).As<ExpressionSyntaxNode>();
                return dclr;
            }

            virtual TypeExp VisitTypeExp(TypeExp const& typeExp)
            {
                TypeExp result = typeExp;
                result.exp = typeExp.exp->Accept(this).As<ExpressionSyntaxNode>();
                if (auto typeType = result.exp->Type.type.As<TypeExpressionType>())
                {
                    result.type = typeType->type;
                }
                return result;
            }

            virtual void VisitExtensionDecl(ExtensionDecl* /*decl*/)
            {}

            virtual void VisitConstructorDecl(ConstructorDecl* /*decl*/)
            {}

            virtual void VisitTraitDecl(TraitDecl* /*decl*/)
            {}

            virtual void VisitTraitConformanceDecl(TraitConformanceDecl* /*decl*/)
            {}

            virtual RefPtr<ExpressionSyntaxNode> VisitSharedTypeExpr(SharedTypeExpr* typeExpr)
            {
                return typeExpr;
            }

            virtual void VisitDeclGroup(DeclGroup* declGroup)
            {
                for (auto decl : declGroup->decls)
                {
                    decl->Accept(this);
                }
            }
        };

        // Note(tfoley): These logically belong to `ExpressionType`,
        // but order-of-declaration stuff makes that tricky
        //
        // TODO(tfoley): These should really belong to the compilation context!
        //
        void RegisterBuiltinDecl(
            RefPtr<Decl>                decl,
            RefPtr<BuiltinTypeModifier> modifier);
        void RegisterMagicDecl(
            RefPtr<Decl>                decl,
            RefPtr<MagicTypeModifier>   modifier);


    }
}

#endif