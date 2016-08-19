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

float rand_next(inout uint s)
{
    const uint LCG_A = 1664525u;
    const uint LCG_C = 1013904223u;
    s = (LCG_A * s + LCG_C);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

#define GRID 8
#define REFERENCE 0
#define BOX 1
#define TRIANGLE 2
#define GAA 3
#define GAA_PLUS 4

#define MODE GAA_PLUS
#define DRAW_POINTS_LINES
//#define RANDOM_SAMPLES
//#define ANIMATE
const vec3 lineq = vec3(-0.15, -1, 0.75* GRID); // our implicit surface

bool is_positive_halfplane(in vec2 pos, in vec3 lineq)
{
    return dot(vec3(pos, 1), lineq) >= 0.0f;
}

float distance_to_line(in vec2 pos, in vec3 lineq)
{
    return abs(dot(vec3(pos, 1), lineq)) / length(lineq.xy);
}

bool line(in vec2 pos, in vec3 lineq, float thickness)
{
    float d = distance_to_line(pos, lineq);
    return d < thickness;
}

bool line(in vec2 pos, float slope, float intercept, float thickness)
{
    return line(pos, vec3(slope, -1, intercept) ,thickness);
}

bool box(in ivec2 pos, in ivec2 minCorner, in ivec2 maxCorner, in ivec2 borderSize)
{
    minCorner += borderSize;
    maxCorner -= borderSize;
    return  all(greaterThanEqual(pos, minCorner)) && all(lessThan(pos, maxCorner));
}

bool rectangle(in ivec2 pos, in ivec2 minCorner, in ivec2 maxCorner, in ivec2 borderSize)
{
    return  box(pos, minCorner, maxCorner, ivec2(0)) &&
        !box(pos, minCorner, maxCorner, borderSize);
}

vec4 drawPoint(in vec2 pixel, in vec2 point, in vec4 pc, inout vec4 fragColor)
{
    const float pointSize = 50.0 / GRID;
    float alpha = length(pixel - point);
    alpha = exp(-alpha * 2.0 / pointSize);
#ifdef DRAW_POINTS_LINES
    fragColor = alpha * pc + (1.0 - alpha) * fragColor;
#endif
    return fragColor;
}



vec4 shade_sample(in vec2 sampl)
{
    return is_positive_halfplane(sampl, lineq) ? vec4(1, 0, 0, 0) : vec4(0, 0, 1, 0);
}

vec2 get_sample(in int i, in int j, in uint seed)
{
    uint idx = j * GRID + i;
    vec2 sampl = vec2(0.5);
#ifdef RANDOM_SAMPLES
    vec2 rnd = vec2(rand_next(idx), rand_next(idx)) - vec2(0.5);
    sampl += rnd * 0.75;
#endif
    return vec2(i, j) + sampl;
}


void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    ivec2 pos = ivec2(fragCoord.xy);
    const ivec2 zoomBoxOrigin = ivec2(0, 0);
    ivec2 zoomBoxDim = ivec2(iResolution.y);
    ivec2 redDim = zoomBoxDim / GRID;

    uint seed = 1;

    if (box(pos, zoomBoxOrigin, zoomBoxOrigin + zoomBoxDim, ivec2(0)))
    {
        vec2 point = vec2(0.0);
        float norm = 0.0;
        int i = int(pos.x / redDim.x);
        int j = int(pos.y / redDim.y);
        int idx = j * GRID + i;
        seed = idx;// uint(iGlobalTime * 30) + idx;
        vec2 pixel_center = vec2(i, j) + vec2(0.5);
        ivec2 origo = zoomBoxOrigin + redDim * ivec2(i, j);

        vec2 pp = get_sample(i, j, seed);

        vec2 adjusted = pp * vec2(redDim);

        if (box(pos, origo, origo + redDim, ivec2(0)))
        {
            fragColor = vec4(0);
            ivec2 rate = ivec2(32);
#if MODE == REFERENCE
            for (int kk = 0; kk < rate.x; kk++)
                for (int kkk = 0; kkk < rate.y; kkk++)
                    fragColor += shade_sample(vec2(i,j) + vec2(kk,kkk) / rate);
            fragColor /= rate.x * rate.y;
#elif MODE == TRIANGLE
            float ww = 0.0f;
            for (int kk = -1; kk <= 1; kk++)
                for (int kkk = -1; kkk <= 1; kkk++)
                {
                    ivec2 n = clamp(ivec2(i,j) + ivec2(kk,kkk), ivec2(0,0), ivec2(GRID,GRID));
                    vec4 col;
                    vec2 sm = get_sample(n.x, n.y, seed);
                    vec2 sample_diff = vec2(sm - pixel_center);
                    float w = max(0.0f, 1.5 - abs(sample_diff.x)) * max(0.0f, 1.5 - abs(sample_diff.y));
                    fragColor += shade_sample(sm) * w;
                    ww += w;
                }
            fragColor /= ww;
#elif MODE == GAA
            bool hori = abs(lineq.x) < 1;
            float d = dot(vec3(pixel_center, 1), lineq) / length(lineq.xy);
            float d_p = dot(vec3(pp, 1), lineq) / length(lineq.xy);
            float coverage = 0.5 - abs(d);
            int side_sample = int(sign(d_p));
            int side_center = int(sign(d));
            ivec2 offset = ivec2(0, 0);
            if (hori)
            {
                offset.y = side_sample;
            }
            else
            {
                offset.x = side_sample;
            }
            ivec2 n = clamp(ivec2(i, j) + offset, ivec2(0, 0), ivec2(GRID, GRID));
            vec2 sm = get_sample(n.x, n.y, seed);
            fragColor = vec4(0);
                    
            if (side_sample != side_center)
                coverage = 1 - coverage;

            fragColor += shade_sample(sm) * (coverage);
            fragColor += shade_sample(pp) * (1 - coverage);

#elif MODE == GAA_PLUS
            fragColor = vec4(0);
            float ww = 0.0f;
            float d = dot(vec3(pixel_center, 1), lineq) / length(lineq.xy);
            float coverage = 0.5 - abs(d);
            int side_center = int(sign(d));
            for (int kk = -1; kk <= 1; kk++)
                for (int kkk = -1; kkk <= 1; kkk++)
                {
                    ivec2 n = clamp(ivec2(i, j) + ivec2(kk, kkk), ivec2(0, 0), ivec2(GRID, GRID));
                    vec4 col;
                    vec2 sm = get_sample(n.x, n.y, seed);
                    vec2 sample_diff = vec2(sm - pixel_center);
                    float d_sm = dot(vec3(pp, 1), lineq) / length(lineq.xy);
                    float coverage_sm = 0.5 - abs(d_sm);
                    coverage_sm = coverage_sm > 0 ? coverage_sm : 1.0f;
                    int side_sm = int(sign(d_sm));
                    float w = (side_sm != side_center) ?  0 : 1;
                    fragColor += shade_sample(sm) * w;
                    ww += w;
                }
            fragColor = coverage > 0 ? vec4(coverage) : vec4(1,0,0,0);
            //vec4(abs(coverage));

            //fragColor = coverage > 0? vec4(coverage, 0, 0, 0) : vec4(0);
#else
            fragColor += shade_sample(pp);
#endif
            fragColor = drawPoint(fragCoord.xy, adjusted, vec4(1), fragColor);
        }

        if (pp.x >= 0.0 && pp.y >= 0.0)
        {
            point += adjusted;
            norm += 1.0;
        }

        point /= norm;

        // Pixel delimiters
#ifdef DRAW_POINTS_LINES
        if (line(pos, lineq * vec3(1, 1, redDim.x), 1.0))
        {
            fragColor = vec4(1.0);
        }
#endif
    }


    //  fragColor = vec4(uv,0.5+0.5*sin(iGlobalTime),1.0);
}