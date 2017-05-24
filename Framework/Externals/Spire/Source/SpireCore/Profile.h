#ifndef SPIRE_PROFILE_H_INCLUDED
#define SPIRE_PROFILE_H_INCLUDED

#include "../CoreLib/Basic.h"
#include "../../Spire.h"

namespace Spire
{
    namespace Compiler
    {
        enum class Language
        {
            Unknown,
#define LANGUAGE(TAG, NAME) TAG,
#include "ProfileDefs.h"
        };

        enum class ProfileFamily
        {
            Unknown,
#define PROFILE_FAMILY(TAG) TAG,
#include "ProfileDefs.h"
        };

        enum class ProfileVersion
        {
            Unknown,
#define PROFILE_VERSION(TAG, FAMILY) TAG,
#include "ProfileDefs.h"
        };

        enum class Stage : SpireStage
        {
            Unknown = SPIRE_STAGE_NONE,
#define PROFILE_STAGE(TAG, NAME, VAL) TAG = VAL,
#include "ProfileDefs.h"
        };

        struct Profile
        {
            typedef uint32_t RawVal;
            enum : RawVal
            {
            Unknown,

#define PROFILE(TAG, NAME, STAGE, VERSION) TAG = (uint32_t(Stage::STAGE) << 16) | uint32_t(ProfileVersion::VERSION),
#include "ProfileDefs.h"
            };

            Profile() {}
            Profile(RawVal raw)
                : raw(raw)
            {}

            Stage GetStage() const { return Stage((uint32_t(raw) >> 16) & 0xFFFF); }
            ProfileVersion GetVersion() const { return ProfileVersion(uint32_t(raw) & 0xFFFF); }

            static Profile LookUp(char const* name);

            RawVal raw = Unknown;
        };
    }
}

#endif
