#ifndef SPIRE_REFLECTION_H
#define SPIRE_REFLECTION_H

#include "../CoreLib/Basic.h"
#include "Syntax.h"

#include "../../Spire.h"

namespace Spire {

// TODO(tfoley): Need to move these somewhere universal

typedef intptr_t    Int;
typedef int64_t     Int64;

typedef uintptr_t   UInt;
typedef uint64_t    UInt64;

namespace Compiler {

class ProgramLayout;
class TypeLayout;

String emitReflectionJSON(
    ProgramLayout* programLayout);

//

SpireTypeKind getReflectionTypeKind(ExpressionType* type);

SpireTypeKind getReflectionParameterCategory(TypeLayout* typeLayout);

UInt getReflectionFieldCount(ExpressionType* type);
UInt getReflectionFieldByIndex(ExpressionType* type, UInt index);
UInt getReflectionFieldByIndex(TypeLayout* typeLayout, UInt index);

}}

#endif // SPIRE_REFLECTION_H
