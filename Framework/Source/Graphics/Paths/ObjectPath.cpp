/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "Framework.h"
#include "ObjectPath.h"
#include "MovableObject.h"
#include "glm/common.hpp"
#include <algorithm>

namespace Falcor
{
    ObjectPath::SharedPtr ObjectPath::create()
    {
        return SharedPtr(new ObjectPath);
    }

    ObjectPath::~ObjectPath() = default;

    uint32_t ObjectPath::addKeyFrame(float time, const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
    {
        Frame keyFrame;
        keyFrame.time = time;
        keyFrame.target = target;
        keyFrame.position = position;
        keyFrame.up = up;
        mDirty = true;

        if(mKeyFrames.size() == 0 || mKeyFrames[0].time > time)
        {
            mKeyFrames.insert(mKeyFrames.begin(), keyFrame);
            return 0;
        }
        else
        {
            for(size_t i = 0 ; i < mKeyFrames.size() ; i++)
            {
                auto& current = mKeyFrames[i];
                // If we already have a key-frame at the same time, replace it
                if(current.time == time)
                {
                    current = keyFrame;
                    return (uint32_t)i;
                }

                // If this is not the last frame, Check if we are in between frames
                if(i < mKeyFrames.size() - 1)
                {
                    auto& Next = mKeyFrames[i + 1];
                    if(current.time < time && Next.time > time)
                    {
                        mKeyFrames.insert(mKeyFrames.begin() + i + 1, keyFrame);
                        return (uint32_t)i + 1;
                    }
                }
            }

            // If we got here, need to push it to the end of the list
            mKeyFrames.push_back(keyFrame);
            return (uint32_t)mKeyFrames.size() - 1;
        }
    }

    void ObjectPath::animate(double currentTime)
    {
        if(mKeyFrames.size() == 0)
            return;

        bool shouldInterpolate = true;
        double animTime = currentTime;
        const auto& firstFrame = mKeyFrames[0];
        const auto& lastFrame = mKeyFrames[mKeyFrames.size() - 1];
        if(mRepeatAnimation)
        {
            float delta = lastFrame.time - firstFrame.time;
            if(delta)
            {
                animTime = float(fmod(currentTime, delta));
                animTime += firstFrame.time;
            }
            else
                animTime = lastFrame.time;
        }

        if(animTime >= lastFrame.time)
        {
            mCurrentFrame = lastFrame;
        }
        else if(animTime <= firstFrame.time)
        {
            mCurrentFrame = firstFrame;
        }
        else
        {
            // Find out where we are
            bool foundFrame = false;
            for(uint32_t i = 0 ; i < (uint32_t)mKeyFrames.size() - 1; i++)
            {
                const auto& curKey = mKeyFrames[i];
                const auto& nextKey = mKeyFrames[i + 1];

                if(animTime >= curKey.time && animTime < nextKey.time)
                {
                    // Found the animation keys. Interpolate
                    switch(mMode)
                    {
                    case Interpolation::Linear:
                        linearInterpolation(i, animTime);
                        break;
                    case Interpolation::CubicSpline:
                        cubicSplineInterpolation(i, animTime);
                        break;
                    default:
                        should_not_get_here();
                    }
                    foundFrame = true;
                    break;
                }
            }
            assert(foundFrame);
        }

        for(auto& pObj : mpObjects)
        {
            pObj->move(mCurrentFrame.position, mCurrentFrame.target, mCurrentFrame.up);
        }
    }

    void ObjectPath::linearInterpolation(uint32_t currentFrame, double currentTime)
    {
        const Frame& current = mKeyFrames[currentFrame];
        const Frame& next = mKeyFrames[currentFrame + 1];
        double delta = next.time - current.time;
        double curTime = currentTime - current.time;
        float factor = float(curTime / delta);

        mCurrentFrame.position = glm::mix(current.position, next.position, factor);
        mCurrentFrame.target = glm::mix(current.target, next.target, factor);
        mCurrentFrame.up = glm::mix(current.up, next.up, factor);
    }

    void ObjectPath::cubicSplineInterpolation(uint32_t currentFrame, double currentTime)
    {
        if(mDirty)
        {
            mDirty = false;
            std::vector<glm::vec3> positions, targets, ups;
            for(auto& a : mKeyFrames)
            {
                positions.push_back(a.position);
                targets.push_back(a.target);
                ups.push_back(a.up);
            }

            mpPositionSpline = std::make_unique<Vec3CubicSpline>(positions.data(), uint32_t(mKeyFrames.size()));
            mpTargetSpline   = std::make_unique<Vec3CubicSpline>(targets.data(),   uint32_t(mKeyFrames.size()));
            mpUpSpline       = std::make_unique<Vec3CubicSpline>(ups.data(),       uint32_t(mKeyFrames.size()));
        }

        const Frame& current = mKeyFrames[currentFrame];
        const Frame& next = mKeyFrames[currentFrame + 1];
        double delta = next.time - current.time;
        double curTime = currentTime - current.time;
        float factor = (float)(curTime / delta);

        mCurrentFrame.position = mpPositionSpline->interpolate(currentFrame, factor);
        mCurrentFrame.target = mpTargetSpline->interpolate(currentFrame, factor);
        mCurrentFrame.up = mpUpSpline->interpolate(currentFrame, factor);
    }

    void ObjectPath::attachObject(const IMovableObject::SharedPtr& pObject)
    {
        // Only attach the object if its not already found
        if(std::find(mpObjects.begin(), mpObjects.end(), pObject) == mpObjects.end())
        {
            mpObjects.push_back(pObject);
        }
    }

    void ObjectPath::detachObject(const IMovableObject::SharedPtr& pObject)
    {
        auto& it = std::find(mpObjects.begin(), mpObjects.end(), pObject);
        if(it != mpObjects.end())
        {
            mpObjects.erase(it);
        }
    }

    void ObjectPath::removeKeyFrame(uint32_t frameID)
    {
        mKeyFrames.erase(mKeyFrames.begin() + frameID);
    }

    uint32_t ObjectPath::setFrameTime(uint32_t frameID, float time)
    {
        const auto Frame = mKeyFrames[frameID];
        removeKeyFrame(frameID);
        return addKeyFrame(time, Frame.position, Frame.target, Frame.up);
    }
}