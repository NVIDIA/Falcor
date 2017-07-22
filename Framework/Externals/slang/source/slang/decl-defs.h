// decl-defs.h

// Syntax class definitions for declarations.

// A group of declarations that should be treated as a unit
SYNTAX_CLASS(DeclGroup, DeclBase)
    SYNTAX_FIELD(List<RefPtr<Decl>>, decls)
END_SYNTAX_CLASS()

// A "container" decl is a parent to other declarations
ABSTRACT_SYNTAX_CLASS(ContainerDecl, Decl)
    SYNTAX_FIELD(List<RefPtr<Decl>>, Members)

    RAW(
    template<typename T>
    FilteredMemberList<T> getMembersOfType()
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
    )
END_SYNTAX_CLASS()

// Base class for all variable-like declarations
ABSTRACT_SYNTAX_CLASS(VarDeclBase, Decl)

    // Type of the variable
    SYNTAX_FIELD(TypeExp, Type)

    RAW(
    ExpressionType* getType() { return Type.type.Ptr(); }
    )

    // Initializer expression (optional)
    SYNTAX_FIELD(RefPtr<ExpressionSyntaxNode>, Expr)
END_SYNTAX_CLASS()


// A field of a `struct` type
SIMPLE_SYNTAX_CLASS(StructField, VarDeclBase)

// An `AggTypeDeclBase` captures the shared functionality
// between true aggregate type declarations and extension
// declarations:
//
// - Both can container members (they are `ContainerDecl`s)
// - Both can have declared bases
// - Both expose a `this` variable in their body
//
ABSTRACT_SYNTAX_CLASS(AggTypeDeclBase, ContainerDecl)
END_SYNTAX_CLASS()

// An extension to apply to an existing type
SYNTAX_CLASS(ExtensionDecl, AggTypeDeclBase)
    SYNTAX_FIELD(TypeExp, targetType)

    // next extension attached to the same nominal type
    DECL_FIELD(ExtensionDecl*, nextCandidateExtension RAW(= nullptr))
END_SYNTAX_CLASS()

// Declaration of a type that represents some sort of aggregate
ABSTRACT_SYNTAX_CLASS(AggTypeDecl, AggTypeDeclBase)

RAW(
    // extensions that might apply to this declaration
    ExtensionDecl* candidateExtensions = nullptr;
    FilteredMemberList<StructField> GetFields()
    {
        return getMembersOfType<StructField>();
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
    )
END_SYNTAX_CLASS()

SIMPLE_SYNTAX_CLASS(StructSyntaxNode, AggTypeDecl)

SIMPLE_SYNTAX_CLASS(ClassSyntaxNode, AggTypeDecl)

// An interface which other types can conform to
SIMPLE_SYNTAX_CLASS(InterfaceDecl, AggTypeDecl)

// A kind of pseudo-member that represents an explicit
// or implicit inheritance relationship.
//
SYNTAX_CLASS(InheritanceDecl, Decl)
    // The type expression as written
    SYNTAX_FIELD(TypeExp, base)
END_SYNTAX_CLASS()

// TODO: may eventually need sub-classes for explicit/direct vs. implicit/indirect inheritance


// A declaration that represents a simple (non-aggregate) type
//
// TODO: probably all types will be aggregate decls eventually,
// so that we can easily store conformances/constraints on type variables
ABSTRACT_SYNTAX_CLASS(SimpleTypeDecl, Decl)
END_SYNTAX_CLASS()

// A `typedef` declaration
SYNTAX_CLASS(TypeDefDecl, SimpleTypeDecl)
    SYNTAX_FIELD(TypeExp, Type)
END_SYNTAX_CLASS()

// A scope for local declarations (e.g., as part of a statement)
SIMPLE_SYNTAX_CLASS(ScopeDecl, ContainerDecl)

SIMPLE_SYNTAX_CLASS(ParameterSyntaxNode, VarDeclBase)

// Base class for things that have parameter lists and can thus be applied to arguments ("called")
ABSTRACT_SYNTAX_CLASS(CallableDecl, ContainerDecl)
    RAW(
    FilteredMemberList<ParameterSyntaxNode> GetParameters()
    {
        return getMembersOfType<ParameterSyntaxNode>();
    })

    SYNTAX_FIELD(TypeExp, ReturnType)
END_SYNTAX_CLASS()

// Base class for callable things that may also have a body that is evaluated to produce their result
ABSTRACT_SYNTAX_CLASS(FunctionDeclBase, CallableDecl)
    SYNTAX_FIELD(RefPtr<StatementSyntaxNode>, Body)
END_SYNTAX_CLASS()

// A constructor/initializer to create instances of a type
SIMPLE_SYNTAX_CLASS(ConstructorDecl, FunctionDeclBase)

// A subscript operation used to index instances of a type
SIMPLE_SYNTAX_CLASS(SubscriptDecl, CallableDecl)

// An "accessor" for a subscript or property
SIMPLE_SYNTAX_CLASS(AccessorDecl, FunctionDeclBase)

SIMPLE_SYNTAX_CLASS(GetterDecl, AccessorDecl)
SIMPLE_SYNTAX_CLASS(SetterDecl, AccessorDecl)

SIMPLE_SYNTAX_CLASS(FunctionSyntaxNode, FunctionDeclBase)

SIMPLE_SYNTAX_CLASS(Variable, VarDeclBase);

// A "module" of code (essentiately, a single translation unit)
// that provides a scope for some number of declarations.
SIMPLE_SYNTAX_CLASS(ProgramSyntaxNode, ContainerDecl)

SYNTAX_CLASS(ImportDecl, Decl)
    // The name of the module we are trying to import
    FIELD(Token, nameToken)

    // The scope that we want to import into
    FIELD(RefPtr<Scope>, scope)

    // The module that actually got imported
    DECL_FIELD(RefPtr<ProgramSyntaxNode>, importedModuleDecl)
END_SYNTAX_CLASS()

// A generic declaration, parameterized on types/values
SYNTAX_CLASS(GenericDecl, ContainerDecl)
    // The decl that is genericized...
    SYNTAX_FIELD(RefPtr<Decl>, inner)
END_SYNTAX_CLASS()

SYNTAX_CLASS(GenericTypeParamDecl, SimpleTypeDecl)
    // The bound for the type parameter represents a trait that any
    // type used as this parameter must conform to
//            TypeExp bound;

    // The "initializer" for the parameter represents a default value
    SYNTAX_FIELD(TypeExp, initType)
END_SYNTAX_CLASS()

// A constraint placed as part of a generic declaration
SYNTAX_CLASS(GenericTypeConstraintDecl, Decl)
    // A type constraint like `T : U` is constraining `T` to be "below" `U`
    // on a lattice of types. This may not be a subtyping relationship
    // per se, but it makes sense to use that terminology here, so we
    // think of these fields as the sub-type and sup-ertype, respectively.
    SYNTAX_FIELD(TypeExp, sub)
    SYNTAX_FIELD(TypeExp, sup)
END_SYNTAX_CLASS()

SIMPLE_SYNTAX_CLASS(GenericValueParamDecl, VarDeclBase)

// Declaration of a user-defined modifier
SYNTAX_CLASS(ModifierDecl, Decl)
    // The name of the C++ class to instantiate
    // (this is a reference to a class in the compiler source code,
    // and not the user's source code)
    FIELD(Token, classNameToken)
END_SYNTAX_CLASS()

// An empty declaration (which might still have modifiers attached).
//
// An empty declaration is uncommon in HLSL, but
// in GLSL it is often used at the global scope
// to declare metadata that logically belongs
// to the entry point, e.g.:
//
//     layout(local_size_x = 16) in;
//
SIMPLE_SYNTAX_CLASS(EmptyDecl, Decl)
