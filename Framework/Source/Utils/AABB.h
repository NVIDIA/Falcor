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
#pragma once
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/common.hpp"

namespace Falcor
{
    struct BoundingBox
    {
        glm::vec3 center;
        glm::vec3 extent;

        bool operator==(const BoundingBox& other)
        {
            return (other.center == center) && (other.extent == extent);
        }

        BoundingBox transform(const glm::mat4& mat) const
        {
            glm::vec3 min = center - extent;
            glm::vec3 max = center + extent;

            glm::vec3 xa = glm::vec3(mat[0] * min.x);
            glm::vec3 xb = glm::vec3(mat[0] * max.x);
            glm::vec3 xMin = glm::min(xa, xb);
            glm::vec3 xMax = glm::max(xa, xb);

            glm::vec3 ya = glm::vec3(mat[1] * min.y);
            glm::vec3 yb = glm::vec3(mat[1] * max.y);
            glm::vec3 yMin = glm::min(ya, yb);
            glm::vec3 yMax = glm::max(ya, yb);

            glm::vec3 za = glm::vec3(mat[2] * min.z);
            glm::vec3 zb = glm::vec3(mat[2] * max.z);
            glm::vec3 zMin = glm::min(za, zb);
            glm::vec3 zMax = glm::max(za, zb);


            glm::vec3 newMin = xMin + yMin + zMin + glm::vec3(mat[3]);
            glm::vec3 newMax = xMax + yMax + zMax + glm::vec3(mat[3]);

            return BoundingBox::fromMinMax(newMin, newMax);
        }

        static BoundingBox fromMinMax(const glm::vec3& min, const glm::vec3 max)
        {
            BoundingBox box;
            box.center = (max + min) * glm::vec3(0.5f);
            box.extent = (max - min) * glm::vec3(0.5f);
            return box;
        }
    };
}