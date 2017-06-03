#ifndef RASTER_RENDERER_SCOPE_DICTIONARY_H
#define RASTER_RENDERER_SCOPE_DICTIONARY_H

#include "../CoreLib/Basic.h"

using namespace CoreLib::Basic;

namespace Spire
{
    namespace Compiler
    {
        template <typename TKey, typename TValue>
        class ScopeDictionary
        {
        public:
            LinkedList<Dictionary<TKey, TValue>> dicts;
        public:
            void PushScope()
            {
                dicts.AddLast();
            }
            void PopScope()
            {
                dicts.Delete(dicts.LastNode());
            }
            bool TryGetValue(const TKey & key, TValue & value)
            {
                for (auto iter = dicts.LastNode(); iter; iter = iter->GetPrevious())
                {
                    bool rs = iter->Value.TryGetValue(key, value);
                    if (rs)
                        return true;
                }
                return false;
            }
            bool TryGetValueInCurrentScope(const TKey & key, TValue & value)
            {
                return dicts.Last().TryGetValue(key, value);
            }
            void Add(const TKey & key, const TValue & value)
            {
                dicts.Last().Add(key, value);
            }
            void Set(const TKey & key, const TValue & value)
            {
                dicts.Last()[key] = value;
            }
        };
    }
}

#endif