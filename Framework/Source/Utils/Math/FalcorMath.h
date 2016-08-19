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
#include "glm/gtc/quaternion.hpp"
#include "glm/geometric.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace Falcor
{
    /*!
    *  \addtogroup Falcor
    *  @{
    */

    /** Creates a quaternion representing rotation between 2 vectors
        \param[in] from The source vector
        \param[in] to The destination vector
    */
    inline glm::quat createQuaternionFromVectors(const glm::vec3& from, const glm::vec3& to)
    {
        glm::quat quat;
        glm::vec3 nFrom = glm::normalize(from);
        glm::vec3 nTo = glm::normalize(to);

        float dot = glm::dot(nFrom, nTo);
        dot = max(min(dot, 1.0f), -1.0f);
        if(dot != 1)
        {
            float angle = acosf(dot);

            glm::vec3 cross = glm::cross(nFrom, nTo);
            glm::vec3 axis = glm::normalize(cross);

            quat = glm::angleAxis(angle, axis);
        }

        return quat;
    }

    /** Projects a 2D coordinate onto a unit sphere
        \param xy The 2D coordinate. if x and y are in the [0,1) range, then a z value can be calculate. Otherwise, xy is normalized and z is zero.
    */
    inline glm::vec3 project2DCrdToUnitSphere(glm::vec2 xy)
    {
        float xyLengthSquared = glm::dot(xy, xy);

        float z = 0;
        if(xyLengthSquared < 1)
        {
            z = sqrt(1 - xyLengthSquared);
        }
        else
        {
            xy = glm::normalize(xy);
        }
        return glm::vec3(xy.x, xy.y, z);
    }

    inline glm::mat4 perspectiveMatrix(float fovy, float aspect, float zNear, float zFar)
    {
        assert(abs(aspect - std::numeric_limits<float>::epsilon()) > static_cast<float>(0));
        assert(zFar > zNear);

        const float tanHalfFovy = tan(fovy * 0.5f);
        const float zRange = zFar / (zNear - zFar);

        glm::mat4 result(0);
        result[0][0] = 1 / (aspect * tanHalfFovy);
        result[1][1] = 1 / (tanHalfFovy);
        result[2][2] = zRange;
        result[2][3] = -1.0f;
        result[3][2] = zRange * zNear;
        return result;
    }

    inline glm::mat4 orthographicMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        glm::mat4 result = glm::ortho(left, right, bottom, top);
        float zRange = 1 / (zNear - zFar);
        result[2][2] = zRange;
        result[3][2] = zNear * zRange;

        return result;
    }
/*! @} */
}