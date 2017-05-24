#ifndef CONSTANT_POOL_H
#define CONSTANT_POOL_H

#include "ShaderCompiler.h"
#include "IL.h"

namespace Spire
{
    namespace Compiler
    {
        class ConstantPoolImpl
        {
        private:
            ILUndefinedOperand undefOperand;
            Dictionary<ConstKey<int>, ILConstOperand*> intConsts;
            Dictionary<ConstKey<float>, ILConstOperand*> floatConsts;
            List<RefPtr<ILConstOperand>> constants;
            RefPtr<ILConstOperand> trueConst, falseConst;
        public:
            ILUndefinedOperand * GetUndefinedOperand()
            {
                return &undefOperand;
            }
            ILOperand * CreateDefaultValue(ILType * type)
            {
                ILOperand * value = 0;
                if (type->IsFloat())
                    value = CreateConstant(0.0f);
                else if (type->IsInt())
                    value = CreateConstant(0);
                else if (type->IsUInt())
                    value = CreateConstant(0, ILBaseType::UInt);
                else if (type->IsBool())
                    value = CreateConstant(0, ILBaseType::Bool);
                else if (auto vectorType = dynamic_cast<ILVectorType*>(type))
                {
                    if (vectorType->BaseType == ILBaseType::Float)
                        value = CreateConstant(0.0f, vectorType->Size);
                    else if (vectorType->BaseType == ILBaseType::Int)
                        value = CreateConstant(0, vectorType->Size);
                    else
                        throw NotImplementedException("default value for this type is not implemented.");
                }
                else
                    throw NotImplementedException("default value for this type is not implemented.");
                return value;
            }
            ILConstOperand * CreateConstantIntVec(int val, int val2)
            {
                ILConstOperand * rs = 0;
                auto key = ConstKey<int>::FromValues(val, val2);
                if (intConsts.TryGetValue(key, rs))
                    return rs;
                rs = new ILConstOperand();
                rs->Type = new ILVectorType(ILBaseType::Int, 2);
                rs->IntValues[0] = val;
                rs->IntValues[1] = val2;
                intConsts[key] = rs;
                rs->Name = rs->ToString();
                constants.Add(rs);
                return rs;
            }

            ILConstOperand * CreateConstantIntVec(int val, int val2, int val3)
            {
                ILConstOperand * rs = 0;
                auto key = ConstKey<int>::FromValues(val, val2, val3);
                if (intConsts.TryGetValue(key, rs))
                    return rs;
                rs = new ILConstOperand();
                rs->Type = new ILVectorType(ILBaseType::Int, 3);
                rs->IntValues[0] = val;
                rs->IntValues[1] = val2;
                rs->IntValues[2] = val3;

                intConsts[key] = rs;
                rs->Name = rs->ToString();
                constants.Add(rs);

                return rs;
            }
            ILConstOperand * CreateConstantIntVec(int val, int val2, int val3, int val4)
            {
                ILConstOperand * rs = 0;
                auto key = ConstKey<int>::FromValues(val, val2, val3, val4);
                if (intConsts.TryGetValue(key, rs))
                    return rs;
                rs = new ILConstOperand();
                rs->Type = new ILVectorType(ILBaseType::Int, 4);
                rs->IntValues[0] = val;
                rs->IntValues[1] = val2;
                rs->IntValues[2] = val3;
                rs->IntValues[3] = val4;
                intConsts[key] = rs;
                rs->Name = rs->ToString();
                constants.Add(rs);

                return rs;
            }

            ILConstOperand * CreateConstant(ILConstOperand * c)
            {
                if (auto basicType = dynamic_cast<ILBasicType*>(c->Type.Ptr()))
                {
                    switch (basicType->Type)
                    {
                    case ILBaseType::Float:
                        return CreateConstant(c->FloatValues[0]);
                    case ILBaseType::Int:
                        return CreateConstant(c->IntValues[0]);
                    default:
                        throw NotImplementedException();
                    }
                }
                else if (auto vectorType = dynamic_cast<ILVectorType*>(c->Type.Ptr()))
                {
                    switch (basicType->Type)
                    {
                    case ILBaseType::Float:
                        if (vectorType->Size == 2)
                            return CreateConstant(c->FloatValues[0], c->FloatValues[1]);
                        if (vectorType->Size == 3)
                            return CreateConstant(c->FloatValues[0], c->FloatValues[1], c->FloatValues[2]);
                        if (vectorType->Size == 4)
                            return CreateConstant(c->FloatValues[0], c->FloatValues[1], c->FloatValues[2], c->FloatValues[3]);
                    case ILBaseType::Int:
                        if (vectorType->Size == 2)
                            return CreateConstantIntVec(c->IntValues[0], c->IntValues[1]);
                        if (vectorType->Size == 3)
                            return CreateConstantIntVec(c->IntValues[0], c->IntValues[1], c->IntValues[2]);
                        if (vectorType->Size == 4)
                            return CreateConstantIntVec(c->IntValues[0], c->IntValues[1], c->IntValues[2], c->IntValues[3]);
                    default:
                        throw NotImplementedException();
                    }
                }
                throw NotImplementedException();
            }

            ILConstOperand * CreateConstant(bool b)
            {
                if (b) 
                    return trueConst.Ptr();
                else
                    return falseConst.Ptr();
            }

            ILConstOperand * CreateConstant(int val, ILBaseType bt)
            {
                ILConstOperand * rs = 0;
                if (intConsts.TryGetValue(ConstKey<int>(val, (int)bt*16 + 1), rs))
                    return rs;
                rs = new ILConstOperand();
                rs->Type = new ILBasicType(bt);
                rs->IntValues[0] = val;
                intConsts[ConstKey<int>(val, (int)bt * 16 + 1)] = rs;
                rs->Name = rs->ToString();
                constants.Add(rs);
                return rs;
            }

            ILConstOperand * CreateConstant(int val, int size = 0)
            {
                ILConstOperand * rs = 0;
                if (intConsts.TryGetValue(ConstKey<int>(val, size), rs))
                    return rs;
                rs = new ILConstOperand();
                if (size <= 1)
                    rs->Type = new ILBasicType(ILBaseType::Int);
                else
                    rs->Type = new ILVectorType(ILBaseType::Int, size);
                rs->IntValues[0] = val;
                intConsts[ConstKey<int>(val, size)] = rs;
                rs->Name = rs->ToString();
                constants.Add(rs);
                return rs;
            }

            ILConstOperand * CreateConstant(float val, int size = 0)
            {
                ILConstOperand * rs = 0;
                if (floatConsts.TryGetValue(ConstKey<float>(val, size), rs))
                    return rs;
                if (Math::IsNaN(val) || Math::IsInf(val))
                {
                    throw InvalidOperationException("Attempting to create NAN constant.");
                }
                rs = new ILConstOperand();
                if (size <= 1)
                    rs->Type = new ILBasicType(ILBaseType::Float);
                else
                    rs->Type = new ILVectorType(ILBaseType::Float, size);
                for (int i = 0; i < 16; i++)
                    rs->FloatValues[i] = val;
                floatConsts[ConstKey<float>(val, size)] = rs;
                rs->Name = rs->ToString();
                constants.Add(rs);

                return rs;
            }

            ILConstOperand * CreateConstant(float val, float val2)
            {
                ILConstOperand * rs = 0;
                if (Math::IsNaN(val) || Math::IsInf(val) || Math::IsNaN(val2) || Math::IsInf(val2))
                {
                    throw InvalidOperationException("Attempting to create NAN constant.");
                }
                auto key = ConstKey<float>::FromValues(val, val2);
                if (floatConsts.TryGetValue(key, rs))
                    return rs;
                rs = new ILConstOperand();
                rs->Type = new ILVectorType(ILBaseType::Float, 2);
                rs->FloatValues[0] = val;
                rs->FloatValues[1] = val2;
                floatConsts[key] = rs;
                rs->Name = rs->ToString();
                constants.Add(rs);

                return rs;
            }

            ILConstOperand * CreateConstant(float val, float val2, float val3)
            {
                ILConstOperand * rs = 0;
                if (Math::IsNaN(val) || Math::IsInf(val) || Math::IsNaN(val2) || Math::IsInf(val2) || Math::IsNaN(val3) || Math::IsInf(val3))
                {
                    throw InvalidOperationException("Attempting to create NAN constant.");
                }
                auto key = ConstKey<float>::FromValues(val, val2, val3);
                if (floatConsts.TryGetValue(key, rs))
                    return rs;
                rs = new ILConstOperand();
                rs->Type = new ILVectorType(ILBaseType::Float, 3);
                rs->FloatValues[0] = val;
                rs->FloatValues[1] = val2;
                rs->FloatValues[2] = val3;

                floatConsts[key] = rs;
                rs->Name = rs->ToString();
                constants.Add(rs);

                return rs;
            }

            ILConstOperand * CreateConstant(float val, float val2, float val3, float val4)
            {
                if (Math::IsNaN(val) || Math::IsInf(val) || Math::IsNaN(val2) || Math::IsInf(val2) || Math::IsNaN(val3) || Math::IsInf(val3) || Math::IsNaN(val4) || Math::IsInf(val4))
                {
                    throw InvalidOperationException("Attempting to create NAN constant.");
                }
                ILConstOperand * rs = 0;
                auto key = ConstKey<float>::FromValues(val, val2, val3, val4);
                if (floatConsts.TryGetValue(key, rs))
                    return rs;
                rs = new ILConstOperand();
                rs->Type = new ILVectorType(ILBaseType::Float, 4);
                rs->FloatValues[0] = val;
                rs->FloatValues[1] = val2;
                rs->FloatValues[2] = val3;
                rs->FloatValues[3] = val4;

                floatConsts[key] = rs;
                rs->Name = rs->ToString();
                constants.Add(rs);

                return rs;
            }

            ConstantPoolImpl()
            {
                trueConst = new ILConstOperand();
                trueConst->Type = new ILBasicType(ILBaseType::Bool);
                trueConst->IntValues[0] = trueConst->IntValues[1] = trueConst->IntValues[2] = trueConst->IntValues[3] = 1;
                trueConst->Name = "true";

                falseConst = new ILConstOperand();
                falseConst->Type = new ILBasicType(ILBaseType::Bool);
                falseConst->IntValues[0] = falseConst->IntValues[1] = falseConst->IntValues[2] = falseConst->IntValues[3] = 0;
                trueConst->Name = "false";

            }
        };

        ConstantPool::ConstantPool()
        {
            impl = new ConstantPoolImpl();
        }
        ConstantPool::~ConstantPool()
        {
            delete impl;
        }
        ILUndefinedOperand * ConstantPool::GetUndefinedOperand()
        {
            return impl->GetUndefinedOperand();
        }
        ILConstOperand * ConstantPool::CreateConstant(ILConstOperand * c)
        {
            return impl->CreateConstant(c);
        }
        ILConstOperand * ConstantPool::CreateConstantIntVec(int val0, int val1)
        {
            return impl->CreateConstantIntVec(val0, val1);

        }
        ILConstOperand * ConstantPool::CreateConstantIntVec(int val0, int val1, int val2)
        {
            return impl->CreateConstantIntVec(val0, val1, val2);
        }
        ILConstOperand * ConstantPool::CreateConstantIntVec(int val0, int val1, int val3, int val4)
        {
            return impl->CreateConstantIntVec(val0, val1, val3, val4);
        }
        ILConstOperand * ConstantPool::CreateConstant(bool b)
        {
            return impl->CreateConstant(b);
        }
        ILConstOperand * ConstantPool::CreateConstant(int val, int vectorSize)
        {
            return impl->CreateConstant(val, vectorSize);
        }
        ILConstOperand * ConstantPool::CreateConstant(float val, int vectorSize)
        {
            return impl->CreateConstant(val, vectorSize);
        }
        ILConstOperand * ConstantPool::CreateConstant(float val, float val1)
        {
            return impl->CreateConstant(val, val1);
        }
        ILConstOperand * ConstantPool::CreateConstant(float val, float val1, float val2)
        {
            return impl->CreateConstant(val, val1, val2);
        }
        ILConstOperand * ConstantPool::CreateConstant(float val, float val1, float val2, float val3)
        {
            return impl->CreateConstant(val, val1, val2, val3);
        }
        ILOperand * ConstantPool::CreateDefaultValue(ILType * type)
        {
            return impl->CreateDefaultValue(type);
        }
    }
}

#endif