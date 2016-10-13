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
#include <vector>
#include "glm/vec3.hpp"
#include "UserInput.h"
#include "API/ProgramVars.h"
#include "Graphics/Program.h"
#include "Graphics/PipelineStateCache.h"

#define GUI_CALL _cdecl
namespace Falcor
{
    class RenderContext;
    struct KeyboardEvent;
    struct MouseEvent;

    /** A class wrapping the external GUI library
    */
    class Gui
    {
        friend class Sample;
    public:
        using UniquePtr = std::unique_ptr<Gui>;
        using UniqueConstPtr = std::unique_ptr<const Gui>;

        /** This structs is used to initialize dropdowns
        */
        struct DropdownValue
        {
            int value;              ///< User defined index. Should be unique between different options.
            std::string label;      ///< Label of the dropdown option.
        };
        using dropdown_list = std::vector < DropdownValue > ;

        /** Create a new GUI object. Each object is essentially a container for a GUI window
        */
        static UniquePtr create(uint32_t width, uint32_t height);

        /** Render the GUI
        */
        void render(RenderContext* pContext, float elapsedTime);

        /** Handle window resize events
        */
        void onWindowResize(uint32_t width, uint32_t height);

        /** Handle mouse events
        */
        bool onMouseEvent(const MouseEvent& event);

        /** Handle keyboard events
        */
        bool onKeyboardEvent(const KeyboardEvent& event);

        /** Static text
        */
        void addText(const std::string& str);

        /** Button. Will return true if the button was pressed
        */
        bool addButton(const std::string& label);

        /** Begin a collapsible group block
            returns true if the group is expanded, otherwise false. Use it to avoid making unnecessary calls
        */
        bool pushGroup(const std::string& label);

        /** End a collapsible group block
        */
        void popGroup();

        /** Adds a floating-point UI element.
            \param[in] label The name of the widget.
            \param[in] pVar A pointer to a float that will be updated directly when the widget state changes.
            \param[in] minVal Optional. The minimum allowed value for the float.
            \param[in] maxVal Optional. The maximum allowed value for the float.
            \param[in] step Optional. The step rate for the float.
        */
        void addFloatVar(const std::string& label, float* pVar, float minVal = -FLT_MAX, float maxVal = FLT_MAX, float step = 0.001f);

        /** Adds a checkbox.
        \param[in] label The name of the checkbox.
        \param[in] pVar A pointer to a boolean that will be updated directly when the checkbox state changes.
        */
        void addCheckBox(const std::string& label, bool* pVar);

        void sameLine();
    protected:
        bool keyboardCallback(const KeyboardEvent& keyEvent);
        bool mouseCallback(const MouseEvent& mouseEvent);
        void windowSizeCallback(uint32_t width, uint32_t height);

    private:
        Gui() = default;
        void init();
        void createVao(uint32_t vertexCount, uint32_t indexCount);

        Vao::SharedPtr mpVao;
        VertexLayout::SharedPtr mpLayout;
        ProgramVars::SharedPtr mpProgramVars;
        Program::SharedPtr mpProgram;
        PipelineStateCache::SharedPtr mpPipelineStateCache;
        uint32_t mGroupStackSize = 0;
    };
}
