// ParameterBinding.cpp
#include "ParameterBinding.h"

#include "ShaderCompiler.h"
#include "TypeLayout.h"

#include "../../Spire.h"

#define SPIRE_EXHAUSTIVE_SWITCH() default: assert(!"unexpected"); break;

namespace Spire {
namespace Compiler {

// Information on ranges of registers already claimed/used
struct UsedRange
{
    int begin;
    int end;
};
bool operator<(UsedRange left, UsedRange right)
{
    if (left.begin != right.begin)
        return left.begin < right.begin;
    if (left.end != right.end)
        return left.end < right.end;
    return false;
}

struct UsedRanges
{
    List<UsedRange> ranges;

    // Add a range to the set, either by extending
    // an existing range, or by adding a new one...
    void Add(UsedRange const& range)
    {
        for (auto& rr : ranges)
        {
            if (rr.begin == range.end)
            {
                rr.begin = range.begin;
                return;
            }
            else if (rr.end == range.begin)
            {
                rr.end = range.end;
                return;
            }
        }
        ranges.Add(range);
        ranges.Sort();
    }

    void Add(int begin, int end)
    {
        UsedRange range;
        range.begin = begin;
        range.end = end;
        Add(range);
    }


    // Try to find space for `count` entries
    int Allocate(int count)
    {
        int begin = 0;

        int rangeCount = ranges.Count();
        for (int rr = 0; rr < rangeCount; ++rr)
        {
            // try to fit in before this range...

            int end = ranges[rr].begin;

            // If there is enough space...
            if (end >= begin + count)
            {
                // ... then claim it and be done
                Add(begin, begin + count);
                return begin;
            }

            // ... otherwise, we need to look at the
            // space between this range and the next
            begin = ranges[rr].end;
        }

        // We've run out of ranges to check, so we
        // can safely go after the last one!
        Add(begin, begin + count);
        return begin;
    }
};

struct ParameterBindingInfo
{
    size_t              space;
    size_t              index;
    size_t              count;
};

enum
{
    kLayoutResourceKindCount = SPIRE_PARAMETER_CATEGORY_MIXED,
};

// Information on a single parameter
struct ParameterInfo : RefObject
{
    // Layout info for the concrete variables that will make up this parameter
    List<RefPtr<VarLayout>> varLayouts;

    ParameterBindingInfo    bindingInfo[kLayoutResourceKindCount];

    // The next parameter that has the same name...
    ParameterInfo* nextOfSameName;

    ParameterInfo()
    {
        // Make sure we aren't claiming any resources yet
        for( int ii = 0; ii < kLayoutResourceKindCount; ++ii )
        {
            bindingInfo[ii].count = 0;
        }
    }
};

struct SharedParameterBindingContext
{
    LayoutRulesImpl* defaultLayoutRules;

    // All shader parameters we've discovered so far, and started to lay out...
    List<RefPtr<ParameterInfo>> parameters;

    // A dictionary to accellerate looking up parameters by name
    Dictionary<String, ParameterInfo*> mapNameToParameterInfo;

    // The program layout we are trying to construct
    RefPtr<ProgramLayout> programLayout;
};

struct ParameterBindingContext
{
    SharedParameterBindingContext* shared;

    // TODO: information for implicit constant buffer!

    // The layout rules to use while computing usage...
    LayoutRulesImpl* layoutRules;

    UsedRanges usedResourceRanges[kLayoutResourceKindCount];
};

// Check whether a global variable declaration represents
// a shader parameter, or is there for some other purpose.
//
// TODO: this *probably* needs to be parameterized on language rules.
bool isGlobalVariableAShaderParameter(
    VarDeclBase*    varDecl)
{
    // We need to determine if this variable represents a shader
    // parameter, or just an ordinary global variable...

    // HLSL `static` modifier indicates "thread local"
    if(varDecl->HasModifier<HLSLStaticModifier>())
        return false;

    // HLSL `groupshared` modifier indicates "thread-group local"
    if(varDecl->HasModifier<HLSLGroupSharedModifier>())
        return false;

    // TODO(tfoley): there may be other cases that we need to handle here

    // TODO(tfoley): GLSL rules are almost completely flipped from HLSL.
    // A GLSL global is "just a global" unless it is specifically decorated
    // as a shader parameter...

    return true;
}


static bool IsGlobalVariableAShaderParameter(
    ParameterBindingContext*    /*context*/,
    RefPtr<VarDeclBase>         varDecl)
{
    return isGlobalVariableAShaderParameter(varDecl.Ptr());
}

struct LayoutSemanticInfo
{
    LayoutResourceKind  kind; // the register kind
    int                 space;
    int                 index;

    // TODO: need to deal with component-granularity binding...
};

LayoutSemanticInfo ExtractLayoutSemanticInfo(
    ParameterBindingContext*    /*context*/,
    HLSLLayoutSemantic*         semantic)
{
    LayoutSemanticInfo info;
    info.space = 0;
    info.index = 0;
    info.kind = LayoutResourceKind::None;

    auto registerName = semantic->registerName.Content;
    if (registerName.Length() == 0)
        return info;

    LayoutResourceKind kind = LayoutResourceKind::None;
    switch (registerName[0])
    {
    case 'b':
        kind = LayoutResourceKind::ConstantBuffer;
        break;

    case 't':
        kind = LayoutResourceKind::ShaderResource;
        break;

    case 'u':
        kind = LayoutResourceKind::UnorderedAccess;
        break;

    case 's':
        kind = LayoutResourceKind::SamplerState;
        break;

    default:
        // TODO: issue an error here!
        return info;
    }

    // TODO: need to parse and handle `space` binding
    int space = 0;

    int index = 0;
    for (int ii = 1; ii < registerName.Length(); ++ii)
    {
        int c = registerName[ii];
        if (c >= '0' && c <= '9')
        {
            index = index * 10 + (c - '0');
        }
        else
        {
            // TODO: issue an error here!
            return info;
        }
    }

    // TODO: handle component mask part of things...

    info.kind = kind;
    info.index = index;
    info.space = space;
    return info;
}

static bool doesParameterMatch(
    ParameterBindingContext*    context,
    RefPtr<VarLayout>           varLayout,
    ParameterInfo*              parameterInfo)
{
    // TODO: need to implement this eventually
    return true;
}

// Collect a single declaration into our set of parameters
static void collectParameter(
    ParameterBindingContext*    context,
    RefPtr<VarDeclBase>         varDecl)
{
    // Find out the layout for the type...
    RefPtr<TypeLayout> typeLayout = CreateTypeLayout(
        varDecl->Type.Ptr(),
        context->layoutRules);

    // Now create a variable layout that we can use
    RefPtr<VarLayout> varLayout = new VarLayout();
    varLayout->typeLayout = typeLayout;
    varLayout->varDecl = DeclRef(varDecl.Ptr(), nullptr).As<VarDeclBaseRef>();

    // This declaration may represent the same logical parameter
    // as a declaration that came from a different translation unit.
    // If that is the case, we want to re-use the same `VarLayout`
    // across both parameters.
    //
    // First we look for an existing entry matching the name
    // of this parameter:
    auto parameterName = varDecl->Name.Content;
    ParameterInfo* parameterInfo = nullptr;
    if( context->shared->mapNameToParameterInfo.TryGetValue(parameterName, parameterInfo) )
    {
        // If the parameters have the same name, but don't "match" according to some reasonable rules,
        // then we need to bail out.
        if( !doesParameterMatch(context, varLayout, parameterInfo) )
        {
            parameterInfo = nullptr;
        }
    }

    // If we didn't find a matching parameter, then we need to create one here
    if( !parameterInfo )
    {
        parameterInfo = new ParameterInfo();
        context->shared->parameters.Add(parameterInfo);
        context->shared->mapNameToParameterInfo.Add(parameterName, parameterInfo);
    }
    else
    {
        varLayout->flags |= VarLayoutFlag::IsRedeclaration;
    }

    // Add this variable declaration to the list of declarations for the parameter
    parameterInfo->varLayouts.Add(varLayout);
}

// Given a single parameter, collect whatever information we have on
// how it has been explicitly bound, which may come from multiple declarations
void generateParameterBindings(
    ParameterBindingContext*    context,
    RefPtr<ParameterInfo>       parameterInfo)
{
    // There must be at least one declaration for the parameter.
    assert(parameterInfo->varLayouts.Count() != 0);

    // Iterate over all declarations looking for explicit binding information.
    for( auto& varLayout : parameterInfo->varLayouts )
    {
        auto typeLayout = varLayout->typeLayout;
        auto varDecl = varLayout->varDecl;

        // If the declaration has explicit binding modifiers, then
        // here is where we want to extract and apply them...

        // Look for HLSL `register` or `packoffset` semantics.
        for (auto semantic : varDecl.GetDecl()->GetModifiersOfType<HLSLLayoutSemantic>())
        {
            // Need to extract the information encoded in the semantic
            LayoutSemanticInfo semanticInfo = ExtractLayoutSemanticInfo(context, semantic);
            auto kind = semanticInfo.kind;
            if (kind == LayoutResourceKind::None)
                continue;

            // TODO: need to special-case when this is a `c` register binding...

            // Find the appropriate resource-binding information
            // inside the type, to see if we even use any resources
            // of the given kind.

            auto typeRes = typeLayout->FindResourceInfo(kind);
            int count = 0;
            if (typeRes)
            {
                count = (int) typeRes->count;
            }
            else
            {
                // TODO: warning here!
            }

            auto& bindingInfo = parameterInfo->bindingInfo[(int)kind];
            if( bindingInfo.count != 0 )
            {
                // We already have a binding here, so we want to
                // confirm that it matches the new one that is
                // incoming...
                if( bindingInfo.count != count
                    || bindingInfo.index != semanticInfo.index
                    || bindingInfo.space != semanticInfo.space )
                {
                    // TODO: diagnose!
                }

                // TODO(tfoley): `register` semantics can technically be
                // profile-specific (not sure if anybody uses that)...
            }
            else
            {
                bindingInfo.count = count;
                bindingInfo.index = semanticInfo.index;
                bindingInfo.space = semanticInfo.space;

                // If things are bound in `space0` (the default), then we need
                // to lay claim to the register range used, so that automatic
                // assignment doesn't go and use the same registers.
                if (semanticInfo.space == 0)
                {
                    context->usedResourceRanges[(int)semanticInfo.kind].Add(
                        semanticInfo.index,
                        semanticInfo.index + count);
                }
            }
        }
    }
}

// Generate the binding information for a shader parameter.
static void completeBindingsForParameter(
    ParameterBindingContext*    context,
    RefPtr<ParameterInfo>       parameterInfo)
{
    // For any resource kind used by the parameter
    // we need to update its layout information
    // to include a binding for that resource kind.
    //
    // We will use the first declaration of the parameter as
    // a stand-in for all the declarations, so it is important
    // that earlier code has validated that the declarations
    // "match".

    assert(parameterInfo->varLayouts.Count() != 0);
    auto firstVarLayout = parameterInfo->varLayouts.First();
    auto firstTypeLayout = firstVarLayout->typeLayout;

    for(auto typeRes : firstTypeLayout->resourceInfos)
    {
        // Did we already apply some explicit binding information
        // for this resource kind?
        auto kind = typeRes.kind;
        auto& bindingInfo = parameterInfo->bindingInfo[(int)kind];
        if( bindingInfo.count != 0 )
        {
            // If things have already been bound, our work is done.
            continue;
        }

        auto count = typeRes.count;
        bindingInfo.count = count;
        bindingInfo.index = context->usedResourceRanges[(int)kind].Allocate((int) count);

        // For now we only auto-generate bindings in space zero
        bindingInfo.space = 0;
    }

    // At this point we should have explicit binding locations chosen for
    // all the relevant resource kinds, so we can apply these to the
    // declarations:

    for(auto& varLayout : parameterInfo->varLayouts)
    {
        for(auto k = 0; k < kLayoutResourceKindCount; ++k)
        {
            auto kind = LayoutResourceKind(k);
            auto& bindingInfo = parameterInfo->bindingInfo[k];

            // skip resources we aren't consuming
            if(bindingInfo.count == 0)
                continue;

            // Add a record to the variable layout
            auto varRes = varLayout->AddResourceInfo(kind);
            varRes->space = (int) bindingInfo.space;
            varRes->index = (int) bindingInfo.index;
        }
    }
}

static void collectGlobalScopeParameters(
    ParameterBindingContext*    context,
    ProgramSyntaxNode*          program)
{
    // First enumerate parameters at global scope
    for( auto decl : program->Members )
    {
        // A shader parameter is always a variable,
        // so skip declarations that aren't variables.
        auto varDecl = decl.As<VarDeclBase>();
        if (!varDecl)
            continue;

        // Skip globals that don't look like parameters
        if (!IsGlobalVariableAShaderParameter(context, varDecl))
            continue;

        collectParameter(context, varDecl);
    }

    // Next, we need to enumerate the parameters of
    // each entry point (which requires knowing what the
    // entry points *are*)

    // TODO(tfoley): Entry point functions should be identified
    // by looking for a generated modifier that is attached
    // to global-scope function declarations.
}

// TODO: move this declaration to somewhere logical:
void BuildMemberDictionary(ContainerDecl* decl);

struct SimpleSemanticInfo
{
    String  name;
    int     index;
};

SimpleSemanticInfo decomposeSimpleSemantic(
    HLSLSimpleSemantic* semantic)
{
    auto composedName = semantic->name.Content;

    // look for a trailing sequence of decimal digits
    // at the end of the composed name
    int length = composedName.Length();
    int indexLoc = length;
    while( indexLoc > 0 )
    {
        auto c = composedName[indexLoc-1];
        if( c >= '0' && c <= '9' )
        {
            indexLoc--;
            continue;
        }
        else
        {
            break;
        }
    }

    SimpleSemanticInfo info;

    // 
    if( indexLoc == length )
    {
        // No index suffix
        info.name = composedName;
        info.index = 0;
    }
    else
    {
        // The name is everything before the digits
        info.name = composedName.SubString(0, indexLoc);
        info.index = strtol(composedName.SubString(indexLoc, length - indexLoc).begin(), nullptr, 10);
    }
    return info;
}

enum class EntryPointParameterDirection
{
    Input,
    Output,
};

struct EntryPointParameterState
{
    String*                         optSemanticName;
    int*                            ioSemanticIndex;
    EntryPointParameterDirection    direction;
    int                             semanticSlotCount;
};

static void processSimpleEntryPointInput(
    ParameterBindingContext*        context,
    RefPtr<ExpressionType>          type,
    EntryPointParameterState const& state)
{
    auto optSemanticName    =  state.optSemanticName;
    auto semanticIndex      = *state.ioSemanticIndex;
    auto semanticSlotCount  =  state.semanticSlotCount;
}

static void processSimpleEntryPointOutput(
    ParameterBindingContext*        context,
    RefPtr<ExpressionType>          type,
    EntryPointParameterState const& state)
{
    auto optSemanticName    =  state.optSemanticName;
    auto semanticIndex      = *state.ioSemanticIndex;
    auto semanticSlotCount  =  state.semanticSlotCount;

    if(!optSemanticName)
        return;

    auto semanticName = *optSemanticName;

    // Note: I'm just doing something expedient here and detecting `SV_Target`
    // outputs and claiming the appropriate register range right away.
    //
    // TODO: we should really be building up some representation of all of this,
    // once we've gone to the trouble of looking it all up...
    if( semanticName.ToLower() == "sv_target" )
    {
        context->usedResourceRanges[int(LayoutResourceKind::UnorderedAccess)].Add(semanticIndex, semanticIndex + semanticSlotCount);
    }
}

static void processSimpleEntryPointParameter(
    ParameterBindingContext*        context,
    RefPtr<ExpressionType>          type,
    EntryPointParameterState const& inState,
    int                             semanticSlotCount = 1)
{
    EntryPointParameterState state = inState;
    state.semanticSlotCount = semanticSlotCount;

    switch( state.direction )
    {
    case EntryPointParameterDirection::Input:
        processSimpleEntryPointInput(context, type, state);
        break;

    case EntryPointParameterDirection::Output:
        processSimpleEntryPointOutput(context, type, state);
        break;

    SPIRE_EXHAUSTIVE_SWITCH()
    }

    *state.ioSemanticIndex += state.semanticSlotCount;
}

static void processEntryPointParameter(
    ParameterBindingContext*        context,
    RefPtr<ExpressionType>          type,
    EntryPointParameterState const& state);

static void processEntryPointParameterWithPossibleSemantic(
    ParameterBindingContext*        context,
    Decl*                           declForSemantic,
    RefPtr<ExpressionType>          type,
    EntryPointParameterState const& state)
{
    // If there is no explicit semantic already in effect, *and* we find an explicit
    // semantic on the associated declaration, then we'll use it.
    if( !state.optSemanticName )
    {
        if( auto semantic = declForSemantic->FindModifier<HLSLSimpleSemantic>() )
        {
            auto semanticInfo = decomposeSimpleSemantic(semantic);
            int semanticIndex = semanticInfo.index;

            EntryPointParameterState subState = state;
            subState.optSemanticName = &semanticInfo.name;
            subState.ioSemanticIndex = &semanticIndex;

            processEntryPointParameter(context, type, subState);
        }
    }

    // Default case: either there was an explicit semantic in effect already,
    // *or* we couldn't find an explicit semantic to apply on the given
    // declaration, so we will just recursive with whatever we have at
    // the moment.
    processEntryPointParameter(context, type, state);
}


static void processEntryPointParameter(
    ParameterBindingContext*        context,
    RefPtr<ExpressionType>          type,
    EntryPointParameterState const& state)
{
    // Scalar and vector types are treated as outputs directly
    if(auto basicType = type->As<BasicExpressionType>())
    {
        processSimpleEntryPointParameter(context, basicType, state);
    }
    else if(auto basicType = type->As<VectorExpressionType>())
    {
        processSimpleEntryPointParameter(context, basicType, state);
    }
    // A matrix is processed as if it was an array of rows
    else if( auto matrixType = type->As<MatrixExpressionType>() )
    {
        auto rowCount = GetIntVal(matrixType->rowCount);
        processSimpleEntryPointParameter(context, basicType, state, rowCount);
    }
    else if( auto arrayType = type->As<ArrayExpressionType>() )
    {
        auto elementCount = GetIntVal(arrayType->ArrayLength);

        for( int ii = 0; ii < elementCount; ++ii )
        {
            processEntryPointParameter(context, arrayType->BaseType, state);
        }
    }
    // Ignore a bunch of types that don't make sense here...
    else if(auto textureType = type->As<TextureType>()) {}
    else if(auto samplerStateType = type->As<SamplerStateType>()) {}
    else if(auto constantBufferType = type->As<ConstantBufferType>()) {}
    // Catch declaration-reference types late in the sequence, since
    // otherwise they will include all of the above cases...
    else if( auto declRefType = type->As<DeclRefType>() )
    {
        auto declRef = declRefType->declRef;

        if (auto structDeclRef = declRef.As<StructDeclRef>())
        {
            // Need to recursively walk the fields of the structure now...
            for( auto field : structDeclRef.GetFields() )
            {
                processEntryPointParameterWithPossibleSemantic(
                    context,
                    field.GetDecl(),
                    field.GetType(),
                    state);
            }
        }
        else
        {
            assert(!"unimplemented");
        }
    }
    else
    {
        assert(!"unimplemented");
    }
}

static void collectEntryPointParameters(
    ParameterBindingContext*        context,
    EntryPointOption const&         entryPoint,
    ProgramSyntaxNode*              translationUnitSyntax)
{
    // First, look for the entry point with the specified name

    // Make sure we've got a query-able member dictionary
    BuildMemberDictionary(translationUnitSyntax);

    Decl* entryPointDecl;
    if( !translationUnitSyntax->memberDictionary.TryGetValue(entryPoint.name, entryPointDecl) )
    {
        // No such entry point!
        return;
    }
    if( entryPointDecl->nextInContainerWithSameName )
    {
        // Not the only decl of that name!
        return;
    }

    FunctionSyntaxNode* entryPointFuncDecl = dynamic_cast<FunctionSyntaxNode*>(entryPointDecl);
    if( !entryPointFuncDecl )
    {
        // Not a function!
        return;
    }

    // Create the layout object here
    auto entryPointLayout = new EntryPointLayout();
    entryPointLayout->profile = entryPoint.profile;
    entryPointLayout->entryPoint = entryPointFuncDecl;


    context->shared->programLayout->entryPoints.Add(entryPointLayout);

    // Okay, we seemingly have an entry-point function, and now we need to collect info on its parameters too
    //
    // TODO: Long-term we probably want complete information on all inputs/outputs of an entry point,
    // but for now we are really just trying to scrape information on fragment outputs, so lets do that:
    //
    // TODO: check whether we should enumerate the parameters before the return type, or vice versa

    int defaultSemanticIndex = 0;

    EntryPointParameterState state;
    state.ioSemanticIndex = &defaultSemanticIndex;
    state.optSemanticName = nullptr;
    state.semanticSlotCount = 0;

    for( auto m : entryPointFuncDecl->Members )
    {
        auto paramDecl = m.As<VarDeclBase>();
        if(!paramDecl)
            continue;

        // We have an entry-point parameter, and need to figure out what to do with it.

        // If it appears to be an input, process it as such.
        if( paramDecl->HasModifier<InModifier>() || paramDecl->HasModifier<InOutModifier>() || !paramDecl->HasModifier<OutModifier>() )
        {
            state.direction = EntryPointParameterDirection::Input;

            processEntryPointParameterWithPossibleSemantic(
                context,
                paramDecl.Ptr(),
                paramDecl->Type.type,
                state);
        }

        // If it appears to be an output, process it as such.
        if(paramDecl->HasModifier<OutModifier>() || paramDecl->HasModifier<InOutModifier>())
        {
            state.direction = EntryPointParameterDirection::Output;

            processEntryPointParameterWithPossibleSemantic(
                context,
                paramDecl.Ptr(),
                paramDecl->Type.type,
                state);
        }
    }

    // If we can find an output type for the entry point, then process it as
    // an output parameter.
    if( auto resultType = entryPointFuncDecl->ReturnType.type )
    {
        state.direction = EntryPointParameterDirection::Output;

        processEntryPointParameterWithPossibleSemantic(
            context,
            entryPointFuncDecl,
            resultType,
            state);
    }
}

static void collectParameters(
    ParameterBindingContext*        context,
    CollectionOfTranslationUnits*   program)
{
    for( auto& translationUnit : program->translationUnits )
    {
        // First look at global-scope parameters
        collectGlobalScopeParameters(context, translationUnit.SyntaxNode.Ptr());

        // Next consider parameters for entry points
        for( auto& entryPoint : translationUnit.options.entryPoints )
        {
            collectEntryPointParameters(context, entryPoint, translationUnit.SyntaxNode.Ptr());
        }
    }
}

static RefPtr<FunctionSyntaxNode>
findEntryPointFunc(
    ProgramSyntaxNode*  code,
    String const&       name)
{
    
}

void GenerateParameterBindings(
    CollectionOfTranslationUnits*   program)
{
    // TODO: need to get this from somewhere!
    auto rules = GetLayoutRulesImpl(LayoutRule::HLSLConstantBuffer);

    RefPtr<ProgramLayout> programLayout = new ProgramLayout;

    // Create a context to hold shared state during the process
    // of generating parameter bindings
    SharedParameterBindingContext sharedContext;
    sharedContext.defaultLayoutRules = rules;
    sharedContext.programLayout = programLayout;

    // Create a sub-context to collect parameters that get
    // declared into the global scope
    ParameterBindingContext context;
    context.shared = &sharedContext;
    context.layoutRules = sharedContext.defaultLayoutRules;

    // Walk through AST to discover all the parameters
    collectParameters(&context, program);

    // Now walk through the parameters to generate initial binding information
    for( auto& parameter : sharedContext.parameters )
    {
        generateParameterBindings(&context, parameter);
    }

    bool anyGlobalUniforms = false;
    for( auto& parameterInfo : sharedContext.parameters )
    {
        assert(parameterInfo->varLayouts.Count() != 0);
        auto firstVarLayout = parameterInfo->varLayouts.First();

        // Does the field have any uniform data?
        if( firstVarLayout->typeLayout->FindResourceInfo(LayoutResourceKind::Uniform) )
        {
            anyGlobalUniforms = true;
            break;
        }
    }

    // If there are any global-scope uniforms, then we need to
    // allocate a constant-buffer binding for them here.
    ParameterBindingInfo globalConstantBufferBinding;
    if( anyGlobalUniforms )
    {
        globalConstantBufferBinding.index =
            context.usedResourceRanges[
                (int)LayoutResourceKind::ConstantBuffer].Allocate(1);

        // For now we only auto-generate bindings in space zero
        globalConstantBufferBinding.space = 0;
    }


    // Now walk through again to actually give everything
    // ranges of registers...
    for( auto& parameter : sharedContext.parameters )
    {
        completeBindingsForParameter(&context, parameter);
    }

    // TODO: need to deal with parameters declared inside entry-point
    // parameter lists at some point...


    // Next we need to create a type layout to reflect the information
    // we have collected.

    RefPtr<StructTypeLayout> globalScopeStructLayout = new StructTypeLayout();
    globalScopeStructLayout->rules = context.layoutRules;

    LayoutInfo structLayoutInfo = rules->BeginStructLayout();
    for( auto& parameterInfo : sharedContext.parameters )
    {
        assert(parameterInfo->varLayouts.Count() != 0);
        auto firstVarLayout = parameterInfo->varLayouts.First();

        // Does the field have any uniform data?
        auto uniformInfo = firstVarLayout->typeLayout->FindResourceInfo(LayoutResourceKind::Uniform);
        size_t uniformSize = uniformInfo ? uniformInfo->count : 0;
        if( uniformSize != 0 )
        {
            // Make sure uniform fields get laid out properly...

            LayoutInfo fieldInfo;
            fieldInfo.size      = uniformSize;
            fieldInfo.alignment = firstVarLayout->typeLayout->uniformAlignment;

            size_t uniformOffset = rules->AddStructField(
                &structLayoutInfo,
                fieldInfo);

            for( auto& varLayout : parameterInfo->varLayouts )
            {
                varLayout->findOrAddResourceInfo(LayoutResourceKind::Uniform)->index = uniformOffset;
            }
        }

        globalScopeStructLayout->fields.Add(firstVarLayout);

        for( auto& varLayout : parameterInfo->varLayouts )
        {
            globalScopeStructLayout->mapVarToLayout.Add(varLayout->varDecl.GetDecl(), varLayout);
        }
    }
    rules->EndStructLayout(&structLayoutInfo);

    RefPtr<TypeLayout> globalScopeLayout = globalScopeStructLayout;

    // If there are global-scope uniforms, then we need to wrap
    // up a global constant buffer type layout to hold them
    if( anyGlobalUniforms )
    {
        auto globalConstantBufferLayout = createConstantBufferTypeLayout(
            nullptr,
            globalScopeStructLayout,
            rules);

        globalScopeLayout = globalConstantBufferLayout;
    }

    // We now have a bunch of layout information, which we should
    // record into a suitable object that represents the program
    programLayout->globalScopeLayout = globalScopeLayout;
    program->layout = programLayout;
}

}}
