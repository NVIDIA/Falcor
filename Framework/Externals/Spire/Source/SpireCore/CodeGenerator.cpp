#include "SyntaxVisitors.h"
#include "ScopeDictionary.h"
#include "CodeWriter.h"
#include "StringObject.h"
#include "Naming.h"
#include "TypeLayout.h"
#include "../CoreLib/Tokenizer.h"
#include <assert.h>

namespace Spire
{
    namespace Compiler
    {
        const int MaxBindingValue = 128;

        template<typename Func>
        class ImportNodeVisitor : public SyntaxVisitor
        {
        public:
            const Func * func;
            ImportNodeVisitor(const Func & f)
                : SyntaxVisitor(nullptr), func(&f)
            {}
            virtual RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * expr) override
            {
                (*func)(expr);
                return expr;
            }
        };

        template<typename Func>
        void EnumerateImportExpressions(SyntaxNode * node, const Func & f)
        {
            ImportNodeVisitor<Func> visitor(f);
            node->Accept(&visitor);
        }

        class CodeGenerator : public ICodeGenerator
        {
        private:
            SymbolTable * symTable;
            ILWorld * currentWorld = nullptr;
            ComponentDefinitionIR * currentComponent = nullptr;
            ILOperand * returnRegister = nullptr;
            ImportExpressionSyntaxNode * currentImport = nullptr;
            ShaderIR * currentShader = nullptr;
            RefPtr<ILShader> compiledShader;
            CompileResult & result;
            List<ILOperand*> exprStack;
            CodeWriter codeWriter;
            ScopeDictionary<String, ILOperand*> variables;
            Dictionary<String, RefPtr<ILType>> genericTypeMappings;
            Dictionary<StructSyntaxNode*, RefPtr<ILStructType>> structTypes;

            void PushStack(ILOperand * op)
            {
                exprStack.Add(op);
            }
            ILOperand * PopStack()
            {
                auto rs = exprStack.Last();
                exprStack.SetSize(exprStack.Count() - 1);
                return rs;
            }
            AllocVarInstruction * AllocVar(ExpressionType * etype)
            {
                AllocVarInstruction * varOp = 0;
                RefPtr<ILType> type = TranslateExpressionType(etype);
                auto arrType = dynamic_cast<ILArrayType*>(type.Ptr());

                if (arrType)
                {
                    varOp = codeWriter.AllocVar(arrType->BaseType, result.Program->ConstantPool->CreateConstant(arrType->ArrayLength));
                }
                else
                {
                    assert(type);
                    varOp = codeWriter.AllocVar(type, result.Program->ConstantPool->CreateConstant(0));
                }
                return varOp;
            }
            FetchArgInstruction * FetchArg(ExpressionType * etype, int argId)
            {
                auto type = TranslateExpressionType(etype);
                auto arrType = dynamic_cast<ILArrayType*>(type.Ptr());
                FetchArgInstruction * varOp = 0;
                if (arrType)
                {
                    auto baseType = arrType->BaseType.Release();
                    varOp = codeWriter.FetchArg(baseType, argId);
                }
                else
                {
                    varOp = codeWriter.FetchArg(type, argId);
                }
                return varOp;
            }
            void TranslateStages(PipelineSyntaxNode * pipeline)
            {
                for (auto & stage : pipeline->GetStages())
                {
                    RefPtr<ILStage> ilStage = new ILStage();
                    ilStage->Position = stage->Position;
                    ilStage->Name = stage->Name.Content;
                    ilStage->StageType = stage->StageType.Content;
                    for (auto & attrib : stage->Attributes)
                    {
                        StageAttribute sattrib;
                        sattrib.Name = attrib.Key;
                        sattrib.Position = attrib.Value.Position;
                        sattrib.Value = attrib.Value.Content;
                        ilStage->Attributes[attrib.Key] = sattrib;
                    }
                    compiledShader->Stages[stage->Name.Content] = ilStage;
                }
            }
            String GetComponentFunctionName(ComponentSyntaxNode * comp)
            {
                StringBuilder nameSb;
                nameSb << comp->ParentDecl->Name.Content << "." << comp->Name.Content;
                return EscapeCodeName(nameSb.ProduceString());
            }
        public:
            virtual RefPtr<StructSyntaxNode> VisitStruct(StructSyntaxNode * st) override
            {
                RefPtr<ILStructType> structType = TranslateStructType(st);
                result.Program->Structs.Add(structType);
                return st;
            }
            virtual void ProcessFunction(FunctionSyntaxNode * func) override
            {
                VisitFunction(func);
            }
            virtual void ProcessStruct(StructSyntaxNode * st) override
            {
                VisitStruct(st);
            }
            virtual void ProcessGlobalVar(VarDeclBase * var) override
            {
                // TODO(tfoley): implement this properly
                AllocVarInstruction * varOp = AllocVar(var->Type.Ptr());
                varOp->Name = EscapeCodeName(var->Name.Content);
                variables.Add(var->Name.Content, varOp);
            }

            void GenerateParameterBindingInfo(ShaderIR * shader)
            {
                Dictionary<int, ModuleInstanceIR*> usedDescriptorSetBindings;
                // initialize module parameter layouts for all module instances in this shader
                for (auto module : shader->ModuleInstances)
                {
                    if (module->BindingIndex != -1)
                    {
                        ModuleInstanceIR * existingModule;
                        if (usedDescriptorSetBindings.TryGetValue(module->BindingIndex, existingModule))
                        {
                            getSink()->diagnose(module->UsingPosition, Diagnostics::bindingAlreadyOccupiedByModule, module->BindingIndex, existingModule->BindingName);
                            getSink()->diagnose(existingModule->UsingPosition, Diagnostics::seeUsingOf, existingModule->SyntaxNode->Name.Content);
                        }
                        usedDescriptorSetBindings[module->BindingIndex] = module.Ptr();
                    }
                }
                // report error if shader uses a top-level module without specifying its binding
                for (auto & module : shader->ModuleInstances)
                {
                    if (module->BindingIndex == -1)
                    {
                        bool hasParam = false;
                        for (auto & comp : module->SyntaxNode->GetMembersOfType<ComponentSyntaxNode>())
                            if (comp->IsParam())
                            {
                                hasParam = true;
                                break;
                            }
                        if (hasParam)
                            getSink()->diagnose(module->UsingPosition, Diagnostics::topLevelModuleUsedWithoutSpecifyingBinding, module->SyntaxNode->Name);
                    }
                }
                shader->ModuleInstances.Sort([](RefPtr<ModuleInstanceIR> & x, RefPtr<ModuleInstanceIR> & y) {return x->BindingIndex < y->BindingIndex; });
                for (auto module : shader->ModuleInstances)
                {
                    if (module->BindingIndex != -1)
                    {
                        auto set = new ILModuleParameterSet();
                        set->BindingName = module->BindingName;
                        set->DescriptorSetId = module->BindingIndex;
                        set->UniformBufferLegacyBindingPoint = set->DescriptorSetId;
                        compiledShader->ModuleParamSets[module->BindingName] = set;
                    }
                }
                // allocate binding slots for shader resources (textures, buffers, samplers etc.), as required by legacy APIs
                Dictionary<int, ComponentDefinitionIR*> usedTextureBindings, usedBufferBindings, usedSamplerBindings, usedStorageBufferBindings;
                int textureBindingAllcator = 0, samplerBindingAllocator = 0, storageBufferBindingAllocator = 0, bufferBindingAllocator = 0;
                // first pass: add components to module layout definition, and assign them user-defined binding slots (if any).
                for (auto def : shader->Definitions)
                {
                    if (def->SyntaxNode->IsParam())
                    {
                        auto module = compiledShader->ModuleParamSets[def->ModuleInstance->BindingName]().Ptr();
                        RefPtr<ILModuleParameterInstance> param = new ILModuleParameterInstance();
                        param->Module = module;
                        param->Name = def->OriginalName;
                        param->Type = TranslateExpressionType(def->SyntaxNode->Type);
                        auto resType = param->Type->GetBindableResourceType();
                        // if this parameter is ordinary-value typed, assign it a buffer range
                        if (resType == BindableResourceType::NonBindable)
                        {
                            param->BindingPoints.Clear();
                            param->BufferOffset = (int)RoundToAlignment(module->BufferSize, (int)GetTypeAlignment(param->Type.Ptr(), LayoutRule::Std140));
                            module->BufferSize = param->BufferOffset + (int)GetTypeSize(param->Type.Ptr(), LayoutRule::Std140);
                        }
                        else
                        {
                            // otherwise, check for binding slot collision if the user assigns a binding slot explicitly
                            param->BufferOffset = -1;
                            Dictionary<int, ComponentDefinitionIR*> * bindingRegistry = nullptr;
                            switch (resType)
                            {
                            case BindableResourceType::Texture:
                                bindingRegistry = &usedTextureBindings;
                                break;
                            case BindableResourceType::Sampler:
                                bindingRegistry = &usedSamplerBindings;
                                break;
                            case BindableResourceType::Buffer:
                                bindingRegistry = &usedBufferBindings;
                                break;
                            case BindableResourceType::StorageBuffer:
                                bindingRegistry = &usedStorageBufferBindings;
                                break;
                            }

                            Token bindingValStr;
                            if (def->SyntaxNode->FindSimpleAttribute("Binding", bindingValStr))
                            {
                                int bindingVal = StringToInt(bindingValStr.Content);

                                ComponentDefinitionIR * otherComp = nullptr;
                                if (bindingRegistry->TryGetValue(bindingVal, otherComp))
                                {
                                    getSink()->diagnose(def->SyntaxNode->Position, Diagnostics::bindingAlreadyOccupiedByComponent, bindingVal, otherComp->OriginalName);
                                    getSink()->diagnose(otherComp->SyntaxNode->Position, Diagnostics::seeDefinitionOf, otherComp->OriginalName);
                                }
                                if (bindingVal < 0 || bindingVal >= MaxBindingValue)
                                {
                                    getSink()->diagnose(def->SyntaxNode->Position, Diagnostics::invalidBindingValue, bindingVal);
                                }
                                (*bindingRegistry)[bindingVal] = def.Ptr();
                                param->BindingPoints.Clear();
                                param->BindingPoints.Add(bindingVal);
                            }
                        }
                        module->Parameters.Add(def->UniqueName, param);
                    }
                }

                // second pass: assign binding slots for rest of resource components whose binding is not explicitly specified by user
                for (auto def : shader->Definitions)
                {
                    if (def->SyntaxNode->IsParam())
                    {
                        auto module = compiledShader->ModuleParamSets[def->ModuleInstance->BindingName]().Ptr();
                        auto & param = **module->Parameters.TryGetValue(def->UniqueName);
                        auto bindableResType = param.Type->GetBindableResourceType();
                        if (bindableResType != BindableResourceType::NonBindable)
                        {
                            Dictionary<int, ComponentDefinitionIR*> * bindingRegistry = nullptr;
                            if (param.BindingPoints.Count())
                                continue;
                            int * bindingAllocator = nullptr;
                            switch (bindableResType)
                            {
                            case BindableResourceType::Texture:
                                bindingRegistry = &usedTextureBindings;
                                bindingAllocator = &textureBindingAllcator;
                                break;
                            case BindableResourceType::Sampler:
                                bindingRegistry = &usedSamplerBindings;
                                bindingAllocator = &samplerBindingAllocator;
                                break;
                            case BindableResourceType::Buffer:
                                bindingRegistry = &usedBufferBindings;
                                bindingAllocator = &bufferBindingAllocator;
                                break;
                            case BindableResourceType::StorageBuffer:
                                bindingRegistry = &usedStorageBufferBindings;
                                bindingAllocator = &storageBufferBindingAllocator;
                                break;
                            }
                            while (bindingRegistry->ContainsKey(*bindingAllocator))
                            {
                                (*bindingAllocator)++;
                            }
                            param.BindingPoints.Add(*bindingAllocator);
                            int maxBinding = GetMaxResourceBindings(bindableResType);
                            if (*bindingAllocator > maxBinding)
                            {
                                getSink()->diagnose(def->SyntaxNode->Position, Diagnostics::bindingExceedsLimit, *bindingAllocator, def->SyntaxNode->Name);
                                getSink()->diagnose(def->SyntaxNode->Position, Diagnostics::seeModuleBeingUsedIn, def->ModuleInstance->SyntaxNode->Name, def->ModuleInstance->BindingName);
                            }
                            (*bindingAllocator)++;
                        }
                    }
                }
            }

            ParameterQualifier GetParamQualifier(ParameterSyntaxNode* paramDecl)
            {
                if(paramDecl->HasModifier<InOutModifier>())
                    return ParameterQualifier::InOut;
                else if(paramDecl->HasModifier<OutModifier>())
                    return ParameterQualifier::Out;
                else
                    return ParameterQualifier::In;
            }

            EnumerableDictionary<String, Token> CopyLayoutAttributes(Decl* decl)
            {
                EnumerableDictionary<String, Token> attrs;
                for (auto attr : decl->GetLayoutAttributes())
                {
                    attrs[attr->Key] = attr->Value;
                }
                return attrs;
            }

            virtual void ProcessShader(ShaderIR * shader) override
            {
                currentShader = shader;
                auto pipeline = shader->Shader->Pipeline;
                compiledShader = new ILShader();
                compiledShader->Name = EscapeCodeName(shader->Shader->Name);
                compiledShader->Position = shader->Shader->Position;

                GenerateParameterBindingInfo(shader);

                TranslateStages(pipeline->SyntaxNode);
                result.Program->Shaders.Add(compiledShader);

                genericTypeMappings.Clear();

                // pass 1: iterating all worlds
                // create ILWorld and ILRecordType objects for all worlds

                for (auto & world : pipeline->Worlds)
                {
                    auto w = new ILWorld();
                    auto recordType = new ILRecordType();
                    recordType->TypeName = world.Key;
                    genericTypeMappings[world.Key] = recordType;
                    w->Name = world.Key;
                    w->OutputType = recordType;
                    w->Attributes = CopyLayoutAttributes(world.Value);
                    w->Shader = compiledShader.Ptr();
                    w->IsAbstract = world.Value->IsAbstract();
                    auto impOps = pipeline->GetImportOperatorsFromSourceWorld(world.Key);
                    w->Position = world.Value->Position;
                    compiledShader->Worlds[world.Key] = w;
                }

                // pass 2: iterating all worlds:
                // 1) Gather list of components for each world, and store it in worldComps dictionary.
                // 2) For each abstract world, add its components to record type

                Dictionary<String, List<ComponentDefinitionIR*>> worldComps;
                auto worlds = From(pipeline->Worlds).Select([](KeyValuePair<String, WorldSyntaxNode*> kv) {return kv.Key; }).Concat(FromSingle(String("<uniform>")));
                //worlds.Add("<uniform>");
                for (auto world : worlds)
                {
                    // gather list of components
                    List<ComponentDefinitionIR*> components;
                    for (auto & compDef : shader->Definitions)
                        if (compDef->World == world)
                            components.Add(compDef.Ptr());
                    // for abstract world, fill in record type now
                    if (world != "<uniform>" && pipeline->Worlds[world]()->IsAbstract())
                    {
                        auto compiledWorld = compiledShader->Worlds[world]();
                        for (auto & comp : components)
                        {
                            ILObjectDefinition compDef;
                            compDef.Attributes = CopyLayoutAttributes(comp->SyntaxNode.Ptr());
                            compDef.Name = comp->UniqueName;
                            compDef.Type = TranslateExpressionType(comp->Type.Ptr());
                            compDef.Position = comp->SyntaxNode->Position;
                            compDef.Binding = -1;

                            compiledWorld->OutputType->Members.AddIfNotExists(compDef.Name, compDef);
                        }
                    }
                    // put the list in worldComps
                    worldComps[world] = components;
                }

                // now we need to deal with import operators
                // create world input declarations base on input components
                for (auto & world : compiledShader->Worlds)
                {
                    auto &components = worldComps[world.Key]();
                    for (auto & comp : components)
                    {
                        if (comp->SyntaxNode->IsInput())
                        {
                            ILObjectDefinition def;
                            def.Name = comp->UniqueName;
                            def.Type = TranslateExpressionType(comp->Type.Ptr());
                            def.Position = comp->SyntaxNode->Position;
                            def.Attributes = CopyLayoutAttributes(comp->SyntaxNode.Ptr());
                            world.Value->Inputs.Add(def);
                        }
                    }
                }

                // fill in record types
                for (auto & comps : worldComps)
                {
                    for (auto & comp : comps.Value)
                    {
                        // for each import operator call "import[w0->w1](x)", add x to w0's record type
                        EnumerateImportExpressions(comp->SyntaxNode.Ptr(), [&](ImportExpressionSyntaxNode * importExpr)
                        {
                            auto recType = genericTypeMappings[importExpr->ImportOperatorDef->SourceWorld.Content]().As<ILRecordType>();
                            ILObjectDefinition entryDef;
                            entryDef.Attributes = CopyLayoutAttributes(comp->SyntaxNode.Ptr());
                            entryDef.Name = importExpr->ComponentUniqueName;
                            entryDef.Type = TranslateExpressionType(importExpr->Type.Ptr());
                            entryDef.Position = importExpr->Position;
                            recType->Members.AddIfNotExists(importExpr->ComponentUniqueName, entryDef);
                        });
                        // if comp is output, add comp to its world's record type
                        if (comp->SyntaxNode->IsOutput())
                        {
                            auto recType = genericTypeMappings[comp->World]().As<ILRecordType>();
                            ILObjectDefinition entryDef;
                            entryDef.Attributes = CopyLayoutAttributes(comp->SyntaxNode.Ptr());
                            entryDef.Name = comp->UniqueName;
                            entryDef.Type = TranslateExpressionType(comp->Type.Ptr());
                            entryDef.Position = comp->SyntaxNode->Position;
                            recType->Members.AddIfNotExists(comp->UniqueName, entryDef);
                        }
                    }
                }

                // sort components by dependency
                for (auto & world : compiledShader->Worlds)
                {
                    auto &components = worldComps[world.Key]();
                    DependencySort(components, [](ComponentDefinitionIR * def)
                    {
                        return def->Dependency;
                    });
                }

                // generate component functions
                for (auto & comp : shader->Definitions)
                {
                    currentComponent = comp.Ptr();
                    if (comp->SyntaxNode->IsComponentFunction())
                    {
                        auto funcName = GetComponentFunctionName(comp->SyntaxNode.Ptr());
                        if (result.Program->Functions.ContainsKey(funcName))
                            continue;
                        RefPtr<ILFunction> func = new ILFunction();
                        RefPtr<FunctionSymbol> funcSym = new FunctionSymbol();
                        func->Name = funcName;
                        func->ReturnType = TranslateExpressionType(comp->Type);
                        symTable->Functions[funcName] = funcSym;
                        result.Program->Functions[funcName] = func;
                        for (auto dep : comp->GetComponentFunctionDependencyClosure())
                        {
                            if (dep->SyntaxNode->IsComponentFunction())
                            {
                                funcSym->ReferencedFunctions.Add(GetComponentFunctionName(dep->SyntaxNode.Ptr()));
                            }
                        }
                        int id = 0;
                        Dictionary<String, ILOperand*> refComponents;
                        variables.PushScope();
                        codeWriter.PushNode();
                        for (auto & dep : comp->GetComponentFunctionDependencyClosure())
                        {
                            if (!dep->SyntaxNode->IsComponentFunction())
                            {
                                auto paramType = TranslateExpressionType(dep->Type);
                                String paramName = EscapeCodeName("p" + String(id) + "_" + dep->OriginalName);
                                func->Parameters.Add(paramName, ILParameter(paramType));
                                auto argInstr = codeWriter.FetchArg(paramType, id + 1);
                                argInstr->Name = paramName;
                                variables.Add(dep->UniqueName, argInstr);
                                id++;
                            }
                        }
                        for (auto & param : comp->SyntaxNode->GetParameters())
                        {
                            auto paramType = TranslateExpressionType(param->Type);
                            String paramName = EscapeCodeName("p" + String(id) + "_" + param->Name.Content);
                            func->Parameters.Add(paramName, ILParameter(paramType, GetParamQualifier(param.Ptr())));
                            auto argInstr = codeWriter.FetchArg(paramType, id + 1);
                            argInstr->Name = paramName;
                            variables.Add(param->Name.Content, argInstr);
                            id++;
                        }
                        if (comp->SyntaxNode->Expression)
                        {
                            comp->SyntaxNode->Expression->Accept(this);
                            codeWriter.Insert(new ReturnInstruction(PopStack()));
                        }
                        else
                        {
                            comp->SyntaxNode->BlockStatement->Accept(this);
                        }
                        variables.PopScope();
                        func->Code = codeWriter.PopNode();
                    }
                    currentComponent = nullptr;
                }
                variables.PushScope();
                // push parameter components to variables table
                if (auto paramComps = worldComps.TryGetValue("<uniform>"))
                {
                    for (auto & comp : *paramComps)
                    {
                        variables.Add(comp->UniqueName, compiledShader->ModuleParamSets[comp->ModuleInstance->BindingName]()->Parameters[comp->UniqueName]().Ptr());
                    }
                }
                for (auto & world : pipeline->Worlds)
                {
                    if (world.Value->IsAbstract())
                        continue;
                    NamingCounter = 0;

                    auto & components = worldComps[world.Key].GetValue();
                    auto compiledWorld = compiledShader->Worlds[world.Key].GetValue().Ptr();
                    currentWorld = compiledWorld;
                    codeWriter.PushNode();
                    variables.PushScope();
                    HashSet<String> localComponents;
                    for (auto & comp : components)
                        localComponents.Add(comp->UniqueName);

                    DependencySort(components, [](ComponentDefinitionIR * def)
                    {
                        return def->Dependency;
                    });

                    for (auto & comp : components)
                    {
                        if (!comp->SyntaxNode->IsComponentFunction())
                            VisitComponent(comp);
                    }

                    variables.PopScope();
                    compiledWorld->Code = codeWriter.PopNode();
                    EvalReferencedFunctionClosure(compiledWorld);
                    currentWorld = nullptr;
                }
                variables.PopScope();
                currentShader = nullptr;
            }

            void EvalReferencedFunctionClosure(ILWorld * world)
            {
                List<String> workList;
                for (auto & rfunc : world->ReferencedFunctions)
                    workList.Add(rfunc);
                for (int i = 0; i < workList.Count(); i++)
                {
                    auto rfunc = workList[i];
                    RefPtr<FunctionSymbol> funcSym;
                    if (symTable->Functions.TryGetValue(rfunc, funcSym))
                    {
                        for (auto & rrfunc : funcSym->ReferencedFunctions)
                        {
                            world->ReferencedFunctions.Add(rrfunc);
                            workList.Add(rrfunc);
                        }
                    }
                }
            }
            virtual RefPtr<ComponentSyntaxNode> VisitComponent(ComponentSyntaxNode *) override
            {
                throw NotImplementedException();
            }
            void VisitComponent(ComponentDefinitionIR * comp)
            {
                currentComponent = comp;
                String varName = EscapeCodeName(currentComponent->OriginalName);
                RefPtr<ILType> type = TranslateExpressionType(currentComponent->Type);

                if (comp->SyntaxNode->IsInput())
                {
                    auto loadInput = new LoadInputInstruction(type.Ptr(), comp->UniqueName);
                    codeWriter.Insert(loadInput);
                    variables.Add(currentComponent->UniqueName, loadInput);
                    return;
                }
                else if (comp->SyntaxNode->IsParam())
                {
                    auto moduleInst = compiledShader->ModuleParamSets[comp->ModuleInstance->BindingName]();
                    auto param = moduleInst->Parameters[comp->UniqueName]().Ptr();
                    variables.Add(currentComponent->UniqueName, param);
                    return;
                }

                ILOperand * componentVar = nullptr;

                if (currentComponent->SyntaxNode->Expression)
                {
                    currentComponent->SyntaxNode->Expression->Accept(this);
                    componentVar = exprStack.Last();
                    exprStack.Clear();
                }
                else if (currentComponent->SyntaxNode->BlockStatement)
                {
                    returnRegister = nullptr;
                    currentComponent->SyntaxNode->BlockStatement->Accept(this);
                    componentVar = returnRegister;
                }

                if (currentWorld->OutputType->Members.ContainsKey(currentComponent->UniqueName))
                {
                    auto exp = new ExportInstruction(currentComponent->UniqueName, currentWorld, componentVar);
                    codeWriter.Insert(exp);
                }

                /*if (!currentComponent->Type->IsTexture() && !currentComponent->Type->IsArray())
                {
                auto vartype = TranslateExpressionType(currentComponent->Type.Ptr(), &recordTypes);
                auto var = codeWriter.AllocVar(vartype, result.Program->ConstantPool->CreateConstant(1));
                var->Name = varName;
                codeWriter.Store(var, componentVar);
                componentVar = var;
                }
                else*/
                componentVar->Name = varName;
                currentWorld->Components[currentComponent->UniqueName] = componentVar;
                variables.Add(currentComponent->UniqueName, componentVar);
                currentComponent = nullptr;
            }
            virtual RefPtr<FunctionSyntaxNode> VisitFunction(FunctionSyntaxNode* function) override
            {
                if (function->IsExtern())
                    return function;
                RefPtr<ILFunction> func = new ILFunction();
                result.Program->Functions.Add(function->InternalName, func);
                func->Name = function->InternalName;
                func->ReturnType = TranslateExpressionType(function->ReturnType);
                variables.PushScope();
                codeWriter.PushNode();
                int id = 0;
                for (auto &param : function->GetParameters())
                {
                    func->Parameters.Add(param->Name.Content, ILParameter(TranslateExpressionType(param->Type), GetParamQualifier(param.Ptr())));
                    auto op = FetchArg(param->Type.Ptr(), ++id);
                    op->Name = EscapeCodeName(String("p_") + param->Name.Content);
                    variables.Add(param->Name.Content, op);
                }
                function->Body->Accept(this);
                func->Code = codeWriter.PopNode();
                variables.PopScope();
                return function;
            }
            virtual RefPtr<StatementSyntaxNode> VisitBlockStatement(BlockStatementSyntaxNode* stmt) override
            {
                variables.PushScope();
                for (auto & subStmt : stmt->Statements)
                    subStmt->Accept(this);
                variables.PopScope();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitWhileStatement(WhileStatementSyntaxNode* stmt) override
            {
                RefPtr<WhileInstruction> instr = new WhileInstruction();
                variables.PushScope();
                codeWriter.PushNode();
                stmt->Predicate->Accept(this);
                codeWriter.Insert(new ReturnInstruction(PopStack()));
                instr->ConditionCode = codeWriter.PopNode();
                codeWriter.PushNode();
                stmt->Statement->Accept(this);
                instr->BodyCode = codeWriter.PopNode();
                codeWriter.Insert(instr.Release());
                variables.PopScope();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitDoWhileStatement(DoWhileStatementSyntaxNode* stmt) override
            {
                RefPtr<DoInstruction> instr = new DoInstruction();
                variables.PushScope();
                codeWriter.PushNode();
                stmt->Predicate->Accept(this);
                codeWriter.Insert(new ReturnInstruction(PopStack()));
                instr->ConditionCode = codeWriter.PopNode();
                codeWriter.PushNode();
                stmt->Statement->Accept(this);
                instr->BodyCode = codeWriter.PopNode();
                codeWriter.Insert(instr.Release());
                variables.PopScope();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitForStatement(ForStatementSyntaxNode* stmt) override
            {
                RefPtr<ForInstruction> instr = new ForInstruction();
                variables.PushScope();
                if (auto initStmt = stmt->InitialStatement.Ptr())
                {
                    // TODO(tfoley): any of this push-pop malarky needed here?
                    initStmt->Accept(this);
                }
                if (stmt->PredicateExpression)
                {
                    codeWriter.PushNode();
                    stmt->PredicateExpression->Accept(this);
                    PopStack();
                    instr->ConditionCode = codeWriter.PopNode();
                }

                if (stmt->SideEffectExpression)
                {
                    codeWriter.PushNode();
                    stmt->SideEffectExpression->Accept(this);
                    PopStack();
                    instr->SideEffectCode = codeWriter.PopNode();
                }

                codeWriter.PushNode();
                stmt->Statement->Accept(this);
                instr->BodyCode = codeWriter.PopNode();
                codeWriter.Insert(instr.Release());
                variables.PopScope();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitIfStatement(IfStatementSyntaxNode* stmt) override
            {
                RefPtr<IfInstruction> instr = new IfInstruction();
                variables.PushScope();
                stmt->Predicate->Accept(this);
                instr->Operand = PopStack();
                codeWriter.PushNode();
                stmt->PositiveStatement->Accept(this);
                instr->TrueCode = codeWriter.PopNode();
                if (stmt->NegativeStatement)
                {
                    codeWriter.PushNode();
                    stmt->NegativeStatement->Accept(this);
                    instr->FalseCode = codeWriter.PopNode();
                }
                codeWriter.Insert(instr.Release());
                variables.PopScope();
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitReturnStatement(ReturnStatementSyntaxNode* stmt) override
            {
                returnRegister = nullptr;
                if (currentWorld != nullptr && currentComponent != nullptr && !currentImport)
                {
                    if (stmt->Expression)
                    {
                        stmt->Expression->Accept(this);
                        returnRegister = PopStack();
                        if (!currentComponent->SyntaxNode->IsComponentFunction())
                        {
                            if (currentWorld->OutputType->Members.ContainsKey(currentComponent->UniqueName))
                            {
                                auto exp = new ExportInstruction(currentComponent->UniqueName, currentWorld, returnRegister);
                                codeWriter.Insert(exp);
                            }
                        }
                        else
                        {
                            codeWriter.Insert(new ReturnInstruction(returnRegister));
                        }
                    }
                }
                else
                {
                    if (stmt->Expression)
                    {
                        stmt->Expression->Accept(this);
                        returnRegister = PopStack();
                    }
                    codeWriter.Insert(new ReturnInstruction(returnRegister));
                }
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitBreakStatement(BreakStatementSyntaxNode* stmt) override
            {
                codeWriter.Insert(new BreakInstruction());
                return stmt;
            }
            virtual RefPtr<StatementSyntaxNode> VisitContinueStatement(ContinueStatementSyntaxNode* stmt) override
            {
                codeWriter.Insert(new ContinueInstruction());
                return stmt;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitSelectExpression(SelectExpressionSyntaxNode * expr) override
            {
                expr->SelectorExpr->Accept(this);
                auto predOp = PopStack();
                expr->Expr0->Accept(this);
                auto v0 = PopStack();
                expr->Expr1->Accept(this);
                auto v1 = PopStack();
                PushStack(codeWriter.Select(predOp, v0, v1));
                return expr;
            }
            ILOperand * EnsureBoolType(ILOperand * op, RefPtr<ExpressionType> type)
            {
                if (!type->Equals(ExpressionType::GetBool()))
                {
                    auto cmpeq = new CmpneqInstruction();
                    cmpeq->Operands[0] = op;
                    cmpeq->Operands[1] = result.Program->ConstantPool->CreateConstant(0);
                    cmpeq->Type = new ILBasicType(ILBaseType::Int);
                    codeWriter.Insert(cmpeq);
                    return cmpeq;
                }
                else
                    return op;
            }
            virtual RefPtr<StatementSyntaxNode> VisitDiscardStatement(DiscardStatementSyntaxNode * stmt) override
            {
                codeWriter.Discard();
                return stmt;
            }

            RefPtr<Variable> VisitDeclrVariable(Variable* varDecl)
            {
                AllocVarInstruction * varOp = AllocVar(varDecl->Type.Ptr());
                varOp->Name = EscapeCodeName(varDecl->Name.Content);
                variables.Add(varDecl->Name.Content, varOp);
                if (varDecl->Expr)
                {
                    varDecl->Expr->Accept(this);
                    Assign(varOp, PopStack());
                }
                return varDecl;
            }

            virtual RefPtr<StatementSyntaxNode> VisitExpressionStatement(ExpressionStatementSyntaxNode* stmt) override
            {
                stmt->Expression->Accept(this);
                PopStack();
                return stmt;
            }
            void Assign(ILOperand * left, ILOperand * right)
            {
                if (auto add = dynamic_cast<AddInstruction*>(left))
                {
                    auto baseOp = add->Operands[0].Ptr();
                    codeWriter.Update(baseOp, add->Operands[1].Ptr(), right);
                    add->Erase();
                }
                else if (auto swizzle = dynamic_cast<SwizzleInstruction*>(left))
                {
                    auto baseOp = swizzle->Operand.Ptr();
                    int index = 0;
                    for (int i = 0; i < swizzle->SwizzleString.Length(); i++)
                    {
                        switch (swizzle->SwizzleString[i])
                        {
                        case 'r':
                        case 'x':
                            index = 0;
                            break;
                        case 'g':
                        case 'y':
                            index = 1;
                            break;
                        case 'b':
                        case 'z':
                            index = 2;
                            break;
                        case 'a':
                        case 'w':
                            index = 3;
                            break;
                        }
                        codeWriter.Update(baseOp, result.Program->ConstantPool->CreateConstant(index),
                            codeWriter.Retrieve(right, result.Program->ConstantPool->CreateConstant(i)));
                    }
                    swizzle->Erase();
                }
                else
                    codeWriter.Store(left, right);
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitBinaryExpression(BinaryExpressionSyntaxNode* expr) override
            {
                expr->RightExpression->Accept(this);
                auto right = PopStack();
                if (expr->Operator == Operator::Assign)
                {
                    expr->LeftExpression->Access = ExpressionAccess::Write;
                    expr->LeftExpression->Accept(this);
                    auto left = PopStack();
                    Assign(left, right);
                    PushStack(left);
                }
                else
                {
                    expr->LeftExpression->Access = ExpressionAccess::Read;
                    expr->LeftExpression->Accept(this);
                    auto left = PopStack();
                    BinaryInstruction * rs = 0;
                    switch (expr->Operator)
                    {
                    case Operator::Add:
                    case Operator::AddAssign:
                        rs = new AddInstruction();
                        break;
                    case Operator::Sub:
                    case Operator::SubAssign:
                        rs = new SubInstruction();
                        break;
                    case Operator::Mul:
                    case Operator::MulAssign:
                        rs = new MulInstruction();
                        break;
                    case Operator::Mod:
                    case Operator::ModAssign:
                        rs = new ModInstruction();
                        break;
                    case Operator::Div:
                    case Operator::DivAssign:
                        rs = new DivInstruction();
                        break;
                    case Operator::And:
                        rs = new AndInstruction();
                        break;
                    case Operator::Or:
                        rs = new OrInstruction();
                        break;
                    case Operator::BitAnd:
                    case Operator::AndAssign:
                        rs = new BitAndInstruction();
                        break;
                    case Operator::BitOr:
                    case Operator::OrAssign:
                        rs = new BitOrInstruction();
                        break;
                    case Operator::BitXor:
                    case Operator::XorAssign:
                        rs = new BitXorInstruction();
                        break;
                    case Operator::Lsh:
                    case Operator::LshAssign:
                        rs = new ShlInstruction();
                        break;
                    case Operator::Rsh:
                    case Operator::RshAssign:
                        rs = new ShrInstruction();
                        break;
                    case Operator::Eql:
                        rs = new CmpeqlInstruction();
                        break;
                    case Operator::Neq:
                        rs = new CmpneqInstruction();
                        break;
                    case Operator::Greater:
                        rs = new CmpgtInstruction();
                        break;
                    case Operator::Geq:
                        rs = new CmpgeInstruction();
                        break;
                    case Operator::Leq:
                        rs = new CmpleInstruction();
                        break;
                    case Operator::Less:
                        rs = new CmpltInstruction();
                        break;
                    default:
                        throw NotImplementedException("Code gen not implemented for this operator.");
                    }
                    rs->Operands.SetSize(2);
                    rs->Operands[0] = left;
                    rs->Operands[1] = right;
                    rs->Type = TranslateExpressionType(expr->Type);
                    codeWriter.Insert(rs);
                    switch (expr->Operator)
                    {
                    case Operator::AddAssign:
                    case Operator::SubAssign:
                    case Operator::MulAssign:
                    case Operator::DivAssign:
                    case Operator::ModAssign:
                    case Operator::LshAssign:
                    case Operator::RshAssign:
                    case Operator::AndAssign:
                    case Operator::OrAssign:
                    case Operator::XorAssign:
                    {
                        expr->LeftExpression->Access = ExpressionAccess::Write;
                        expr->LeftExpression->Accept(this);
                        auto target = PopStack();
                        Assign(target, rs);
                        break;
                    }
                    default:
                        break;
                    }
                    PushStack(rs);
                }
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitProject(ProjectExpressionSyntaxNode * project) override
            {
                project->BaseExpression->Accept(this);
                auto rs = PopStack();
                auto proj = new ProjectInstruction();
                proj->ComponentName = currentImport->ComponentUniqueName;
                proj->Operand = rs;
                codeWriter.Insert(proj);
                PushStack(proj);
                return project;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitConstantExpression(ConstantExpressionSyntaxNode* expr) override
            {
                ILConstOperand * op;
                if (expr->ConstType == ConstantExpressionSyntaxNode::ConstantType::Float)
                {
                    op = result.Program->ConstantPool->CreateConstant(expr->FloatValue);
                }
                else if (expr->ConstType == ConstantExpressionSyntaxNode::ConstantType::Bool)
                {
                    op = result.Program->ConstantPool->CreateConstant(expr->IntValue != 0);
                }
                else
                {
                    op = result.Program->ConstantPool->CreateConstant(expr->IntValue);
                }
                PushStack(op);
                return expr;
            }
            void GenerateIndexExpression(ILOperand * base, ILOperand * idx, bool read)
            {
                if (read)
                {
                    auto ldInstr = codeWriter.Retrieve(base, idx);
                    ldInstr->Attribute = base->Attribute;
                    PushStack(ldInstr);
                }
                else
                {
                    PushStack(codeWriter.Add(base, idx));
                }
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * expr) override
            {
                variables.PushScope();
                List<ILOperand*> arguments;
                int argIndex = 0;
                for (auto param : expr->ImportOperatorDef->GetParameters())
                {
                    expr->Arguments[argIndex]->Accept(this);
                    auto argOp = PopStack();
                    arguments.Add(argOp);
                    variables.Add(param->Name.Content, argOp);
                    argIndex++;
                }
                currentImport = expr;
                auto oldTypeMapping = genericTypeMappings.TryGetValue(expr->ImportOperatorDef->TypeName.Content);
                auto componentType = TranslateExpressionType(expr->Type);
                genericTypeMappings[expr->ImportOperatorDef->TypeName.Content] = componentType;
                codeWriter.PushNode();
                expr->ImportOperatorDef->Body->Accept(this);
                currentImport = nullptr;
                auto impInstr = new ImportInstruction(expr->Arguments.Count());
                for (int i = 0; i < expr->Arguments.Count(); i++)
                    impInstr->Arguments[i] = arguments[i];
                impInstr->ImportOperator = codeWriter.PopNode();
                variables.PopScope();
                if (oldTypeMapping)
                    genericTypeMappings[expr->ImportOperatorDef->TypeName.Content] = *oldTypeMapping;
                else
                    genericTypeMappings.Remove(expr->ImportOperatorDef->TypeName.Content);
                impInstr->ComponentName = expr->ComponentUniqueName;
                impInstr->Type = TranslateExpressionType(expr->Type);
                codeWriter.Insert(impInstr);
                PushStack(impInstr);
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitIndexExpression(IndexExpressionSyntaxNode* expr) override
            {
                expr->BaseExpression->Access = expr->Access;
                expr->BaseExpression->Accept(this);
                auto base = PopStack();
                expr->IndexExpression->Access = ExpressionAccess::Read;
                expr->IndexExpression->Accept(this);
                auto idx = PopStack();
                GenerateIndexExpression(base, idx,
                    expr->Access == ExpressionAccess::Read);
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
            {
                RefPtr<Object> refObj;
                if (expr->Tags.TryGetValue("ComponentReference", refObj))
                {
                    if (auto refComp = refObj.As<StringObject>())
                    {
                        ILOperand * op;
                        if (variables.TryGetValue(refComp->Content, op))
                            PushStack(op);
                        else
                            throw InvalidProgramException("referencing undefined component/variable. probable cause: unchecked circular reference.");
                    }
                }
                else
                {
                    expr->BaseExpression->Access = expr->Access;
                    expr->BaseExpression->Accept(this);
                    auto base = PopStack();
                    auto generateSingleMember = [&](char memberName)
                    {
                        int idx = 0;
                        if (memberName == 'y' || memberName == 'g')
                            idx = 1;
                        else if (memberName == 'z' || memberName == 'b')
                            idx = 2;
                        else if (memberName == 'w' || memberName == 'a')
                            idx = 3;

                        GenerateIndexExpression(base, result.Program->ConstantPool->CreateConstant(idx),
                            expr->Access == ExpressionAccess::Read);
                    };
                    if (expr->BaseExpression->Type->IsVectorType())
                    {
                        if (expr->MemberName.Length() == 1)
                        {
                            generateSingleMember(expr->MemberName[0]);
                        }
                        else
                        {
                            auto rs = new SwizzleInstruction();
                            rs->Type = TranslateExpressionType(expr->Type.Ptr());
                            rs->SwizzleString = expr->MemberName;
                            rs->Operand = base;
                            codeWriter.Insert(rs);
                            PushStack(rs);
                        }
                    }
                    else if(auto declRefType = expr->BaseExpression->Type->AsDeclRefType())
                    {
                        if (auto structDecl = declRefType->declRef.As<StructDeclRef>())
                        {
                            int id = structDecl.GetDecl()->FindFieldIndex(expr->MemberName);
                            GenerateIndexExpression(base, result.Program->ConstantPool->CreateConstant(id),
                                expr->Access == ExpressionAccess::Read);
                        }
                        else
                            throw NotImplementedException("member expression codegen");
                    }
                    else
                        throw NotImplementedException("member expression codegen");
                }
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitInvokeExpression(InvokeExpressionSyntaxNode* expr) override
            {
                List<ILOperand*> args;
                String funcName;
                bool hasSideEffect = false;
                if (auto funcType = expr->FunctionExpr->Type->As<FuncType>())
                {
                    if (funcType->Func)
                    {
                        funcName = funcType->Func->SyntaxNode->IsExtern() ? funcType->Func->SyntaxNode->Name.Content : funcType->Func->SyntaxNode->InternalName;
                        for (auto & param : funcType->Func->SyntaxNode->GetParameters())
                        {
                            if (param->HasModifier<OutModifier>())
                            {
                                hasSideEffect = true;
                                break;
                            }
                        }
                    }
                    else if (funcType->Component)
                    {
                        auto funcCompName = expr->FunctionExpr->Tags["ComponentReference"]().As<StringObject>()->Content;
                        auto funcComp = *(currentShader->DefinitionsByComponent[funcCompName]().TryGetValue(currentComponent->World));
                        funcName = GetComponentFunctionName(funcComp->SyntaxNode.Ptr());
                        for (auto & param : funcComp->SyntaxNode->GetParameters())
                        {
                            if (param->HasModifier<OutModifier>())
                            {
                                hasSideEffect = true;
                                break;
                            }
                        }
                        // push additional arguments
                        for (auto & dep : funcComp->GetComponentFunctionDependencyClosure())
                        {
                            if (!dep->SyntaxNode->IsComponentFunction())
                            {
                                ILOperand * op = nullptr;
                                if (variables.TryGetValue(dep->UniqueName, op))
                                    args.Add(op);
                                else
                                    throw InvalidProgramException("cannot resolve reference for implicit component function argument.");
                            }
                        }
                    }
                }
                if (currentWorld)
                {
                    currentWorld->ReferencedFunctions.Add(funcName);
                }
                for (auto arg : expr->Arguments)
                {
                    arg->Accept(this);
                    args.Add(PopStack());
                }
                auto instr = new CallInstruction(args.Count());
                instr->SideEffect = hasSideEffect;
                instr->Function = funcName;
                for (int i = 0; i < args.Count(); i++)
                    instr->Arguments[i] = args[i];
                instr->Type = TranslateExpressionType(expr->Type);
                codeWriter.Insert(instr);
                PushStack(instr);
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr) override
            {
                expr->Expression->Accept(this);
                auto base = PopStack();
                if (expr->Expression->Type->Equals(expr->Type))
                {
                    PushStack(base);
                }
                else if (expr->Expression->Type->Equals(ExpressionType::GetFloat()) &&
                    expr->Type->Equals(ExpressionType::GetInt()))
                {
                    auto instr = new Float2IntInstruction(base);
                    codeWriter.Insert(instr);
                    PushStack(instr);
                }
                else if (expr->Expression->Type->Equals(ExpressionType::GetInt()) &&
                    expr->Type->Equals(ExpressionType::GetFloat()))
                {
                    auto instr = new Int2FloatInstruction(base);
                    codeWriter.Insert(instr);
                    PushStack(instr);
                }
                else
                {
                    getSink()->diagnose(expr, Diagnostics::invalidTypeCast, expr->Expression->Type, expr->Type);
                }
                return expr;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitUnaryExpression(UnaryExpressionSyntaxNode* expr) override
            {
                if (expr->Operator == Operator::PostDec || expr->Operator == Operator::PostInc
                    || expr->Operator == Operator::PreDec || expr->Operator == Operator::PreInc)
                {
                    expr->Expression->Access = ExpressionAccess::Read;
                    expr->Expression->Accept(this);
                    auto base = PopStack();
                    BinaryInstruction * instr;
                    if (expr->Operator == Operator::PostDec)
                        instr = new SubInstruction();
                    else
                        instr = new AddInstruction();
                    instr->Operands.SetSize(2);
                    instr->Operands[0] = base;
                    if (expr->Type->Equals(ExpressionType::GetFloat()))
                        instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1.0f);
                    else
                        instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1);
                    instr->Type = TranslateExpressionType(expr->Type);
                    codeWriter.Insert(instr);

                    expr->Expression->Access = ExpressionAccess::Write;
                    expr->Expression->Accept(this);
                    auto dest = PopStack();
                    auto store = new StoreInstruction(dest, instr);
                    codeWriter.Insert(store);
                    PushStack(base);
                }
                else if (expr->Operator == Operator::PreDec || expr->Operator == Operator::PreInc)
                {
                    expr->Expression->Access = ExpressionAccess::Read;
                    expr->Expression->Accept(this);
                    auto base = PopStack();
                    BinaryInstruction * instr;
                    if (expr->Operator == Operator::PostDec)
                        instr = new SubInstruction();
                    else
                        instr = new AddInstruction();
                    instr->Operands.SetSize(2);
                    instr->Operands[0] = base;
                    if (expr->Type->Equals(ExpressionType::GetFloat()))
                        instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1.0f);
                    else
                        instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1);
                    instr->Type = TranslateExpressionType(expr->Type);
                    codeWriter.Insert(instr);

                    expr->Expression->Access = ExpressionAccess::Write;
                    expr->Expression->Accept(this);
                    auto dest = PopStack();
                    auto store = new StoreInstruction(dest, instr);
                    codeWriter.Insert(store);
                    PushStack(instr);
                }
                else
                {
                    expr->Expression->Accept(this);
                    auto base = PopStack();
                    auto genUnaryInstr = [&](ILOperand * input)
                    {
                        UnaryInstruction * rs = 0;
                        switch (expr->Operator)
                        {
                        case Operator::Not:
                            input = EnsureBoolType(input, expr->Expression->Type);
                            rs = new NotInstruction();
                            break;
                        case Operator::Neg:
                            rs = new NegInstruction();
                            break;
                        case Operator::BitNot:
                            rs = new BitNotInstruction();
                            break;
                        default:
                            throw NotImplementedException("Code gen is not implemented for this operator.");
                        }
                        rs->Operand = input;
                        rs->Type = input->Type;
                        codeWriter.Insert(rs);
                        return rs;
                    };
                    PushStack(genUnaryInstr(base));
                }
                return expr;
            }
            bool GenerateVarRef(String name, ExpressionAccess access)
            {
                ILOperand * var = 0;
                String srcName = name;
                if (!variables.TryGetValue(srcName, var))
                {
                    return false;
                }
                if (access == ExpressionAccess::Read)
                {
                    PushStack(var);
                }
                else
                {
                    PushStack(var);
                }
                return true;
            }
            virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode* expr) override
            {
                RefPtr<Object> refObj;
                if (expr->Tags.TryGetValue("ComponentReference", refObj))
                {
                    if (auto refComp = refObj.As<StringObject>())
                    {
                        ILOperand * op;
                        if (variables.TryGetValue(refComp->Content, op))
                            PushStack(op);
                        else
                            throw InvalidProgramException(String("referencing undefined component/variable '") + refComp->Content + "'. probable cause: unchecked circular reference.");
                    }
                }
                else if (!GenerateVarRef(expr->Variable, expr->Access))
                {
                    throw InvalidProgramException("identifier is neither a variable nor a recognized component.");
                }
                return expr;
            }
        private:
            CodeGenerator & operator = (const CodeGenerator & other) = delete;
        public:
            CodeGenerator(SymbolTable * symbols, DiagnosticSink * pErr, CompileResult & _result)
                : ICodeGenerator(pErr), symTable(symbols), result(_result)
            {
                result.Program = new ILProgram();
                codeWriter.SetConstantPool(result.Program->ConstantPool.Ptr());
            }

        private:
            RefPtr<ILStructType> TranslateStructType(StructSyntaxNode* structDecl)
            {
                RefPtr<ILStructType> ilStructType;

                if (structTypes.TryGetValue(structDecl, ilStructType))
                {
                    return ilStructType;
                }

                ilStructType = new ILStructType();
                ilStructType->TypeName = structDecl->Name.Content;
                ilStructType->IsIntrinsic = structDecl->IsIntrinsic;


                for (auto field : structDecl->GetFields())
                {
                    ILStructType::ILStructField ilField;
                    ilField.FieldName = field->Name.Content;
                    ilField.Type = TranslateExpressionType(field->Type.Ptr());
                    ilStructType->Members.Add(ilField);
                }

                structTypes.Add(structDecl, ilStructType);
                return ilStructType;
            }

            RefPtr<ILType> TranslateExpressionType(ExpressionType * type)
            {
                if (auto basicType = type->AsBasicType())
                {
                    auto base = new ILBasicType();
                    base->Type = (ILBaseType)basicType->BaseType;
                    return base;
                }
                else if (auto genericType = type->As<ImportOperatorGenericParamType>())
                {
                    return genericTypeMappings[genericType->GenericTypeVar]();
                }
                else if (auto vecType = type->AsVectorType())
                {
                    auto elementType = vecType->elementType->AsBasicType();
                    int elementCount = GetIntVal(vecType->elementCount);
                    assert(elementType);

                    static const struct {
                        BaseType	elementType;
                        int			elementCount;
                        ILBaseType	ilBaseType;
                    } kMapping[] = {
                        { BaseType::Float, 2, /*ILBaseType::*/Float2 },
                        { BaseType::Float, 3, /*ILBaseType::*/Float3 },
                        { BaseType::Float, 4, /*ILBaseType::*/Float4 },
                        { BaseType::Int, 2, /*ILBaseType::*/Int2 },
                        { BaseType::Int, 3, /*ILBaseType::*/Int3 },
                        { BaseType::Int, 4, /*ILBaseType::*/Int4 },
                        { BaseType::UInt, 2, /*ILBaseType::*/UInt2 },
                        { BaseType::UInt, 3, /*ILBaseType::*/UInt3 },
                        { BaseType::UInt, 4, /*ILBaseType::*/UInt4 },
                    };
                    static const int kMappingCount = sizeof(kMapping) / sizeof(kMapping[0]);

                    for (int ii = 0; ii < kMappingCount; ++ii)
                    {
                        if (elementCount != kMapping[ii].elementCount) continue;
                        if (elementType->BaseType != kMapping[ii].elementType) continue;

                        auto base = new ILBasicType();
                        base->Type = kMapping[ii].ilBaseType;
                        return base;
                    }
                    throw NotImplementedException("vector type");
                }
                else if (auto matType = type->AsMatrixType())
                {
                    auto elementType = matType->elementType->AsBasicType();
                    int rowCount = GetIntVal(matType->rowCount);
                    int colCount =  GetIntVal(matType->colCount);
                    assert(elementType);

                    static const struct {
                        BaseType	elementType;
                        int			rowCount;
                        int			colCount;
                        ILBaseType	ilBaseType;
                    } kMapping[] = {
                        { BaseType::Float, 3, 3, /*ILBaseType::*/Float3x3 },
                        { BaseType::Float, 4, 4, /*ILBaseType::*/Float4x4 },
                    };
                    static const int kMappingCount = sizeof(kMapping) / sizeof(kMapping[0]);

                    for (int ii = 0; ii < kMappingCount; ++ii)
                    {
                        if (rowCount != kMapping[ii].rowCount) continue;
                        if (colCount != kMapping[ii].colCount) continue;
                        if (elementType->BaseType != kMapping[ii].elementType) continue;

                        auto base = new ILBasicType();
                        base->Type = kMapping[ii].ilBaseType;
                        return base;
                    }
                    throw NotImplementedException("matrix type");
                }

                else if (auto cbufferType = type->As<ConstantBufferType>())
                {
                    auto ilType = new ILGenericType();
                    ilType->GenericTypeName = "ConstantBuffer";
                    ilType->BaseType = TranslateExpressionType(cbufferType->elementType.Ptr());
                    return ilType;
                }

                else if (auto declRefType = type->AsDeclRefType())
                {
                    auto decl = declRefType->declRef.decl;
                    if (auto worldDecl = dynamic_cast<WorldSyntaxNode*>(decl))
                    {
                        return genericTypeMappings[worldDecl->Name.Content];
                    }
                    else if (auto structDecl = dynamic_cast<StructSyntaxNode*>(decl))
                    {
                        return TranslateStructType(structDecl);
                    }
                    else
                    {
                        throw NotImplementedException("decl type");
                    }
                }
                else if (auto arrType = type->AsArrayType())
                {
                    auto nArrType = new ILArrayType();
                    nArrType->BaseType = TranslateExpressionType(arrType->BaseType.Ptr());
                    nArrType->ArrayLength = arrType->ArrayLength ? GetIntVal(arrType->ArrayLength) : 0;
                    return nArrType;
                }
#if TIMREMOVED
                else if (auto genType = type->AsGenericType())
                {
                    auto gType = new ILGenericType();
                    gType->GenericTypeName = genType->GenericTypeName;
                    gType->BaseType = TranslateExpressionType(genType->BaseType.Ptr());
                    return gType;
                }
#endif
                throw NotImplementedException("decl type");
            }

            RefPtr<ILType> TranslateExpressionType(const RefPtr<ExpressionType> & type)
            {
                return TranslateExpressionType(type.Ptr());
            }

        };

        ICodeGenerator * CreateCodeGenerator(SymbolTable * symbols, CompileResult & result)
        {
            return new CodeGenerator(symbols, result.GetErrorWriter(), result);
        }
    }
}
