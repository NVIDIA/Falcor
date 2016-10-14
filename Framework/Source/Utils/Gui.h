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
#include <unordered_map>
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
            int32_t value;          ///< User defined index. Should be unique between different options.
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
            \param[in] str The string to display
            \param[in] sameLine Optional. If set to true, the widget will appear on the same line as the previous widget
        */
        void addText(const std::string& str, bool sameLine = false);

        /** Button. Will return true if the button was pressed
            \param[in] sameLine Optional. If set to true, the widget will appear on the same line as the previous widget
        */
        bool addButton(const std::string& label, bool sameLine = false);

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
            \param[in] sameLine Optional. If set to true, the widget will appear on the same line as the previous widget
        */
        void addFloatVar(const std::string& label, float* pVar, float minVal = -FLT_MAX, float maxVal = FLT_MAX, float step = 0.001f, bool sameLine = false);

        /** Adds a checkbox.
            \param[in] label The name of the checkbox.
            \param[in] pVar A pointer to a boolean that will be updated directly when the checkbox state changes.
            \param[in] sameLine Optional. If set to true, the widget will appear on the same line as the previous widget
        */
        void addCheckBox(const std::string& label, bool* pVar, bool sameLine = false);

        /** Adds an RGB color UI widget.
            \param[in] name The name of the widget.
            \param[in] pVar A pointer to a vector that will be updated directly when the widget state changes.
            \param[in] sameLine Optional. If set to true, the widget will appear on the same line as the previous widget
        */
        void addRgbColor(const std::string& name, glm::vec3* pVar, bool sameLine = false);

        /** Adds an RGBA color UI widget.
            \param[in] name The name of the widget.
            \param[in] pVar A pointer to a vector that will be updated directly when the widget state changes.
            \param[in] sameLine Optional. If set to true, the widget will appear on the same line as the previous widget
        */
        void addRgbaColor(const std::string& name, glm::vec4* pVar, bool sameLine = false);

        /** Adds an integer UI element.
            \param[in] name The name of the widget.
            \param[in] pVar A pointer to an integer that will be updated directly when the widget state changes.
            \param[in] minVal Optional. The minimum allowed value for the variable.
            \param[in] maxVal Optional. The maximum allowed value for the variable.
            \param[in] step Optional. The step rate.
            \param[in] sameLine Optional. If set to true, the widget will appear on the same line as the previous widget
        */
        void addIntVar(const std::string& name, int32_t* pVar, int minVal = -INT32_MAX, int maxVal = INT32_MAX, int step = 1, bool sameLine = false);

        /** Add a separator
        */
        void addSeparator();

        /** Adds a dropdown menu. This will update a user variable directly, so the user has to keep track of that for changes.
            If you want notifications whenever the select option changed, use Gui#addDropdownWithCallback().
            \param[in] name The name of the dropdown menu.
            \param[in] values A list of options to show in the dropdown menu.
            \param[in] pVar A pointer to a user variable that will be updated directly when a dropdown option changes. This correlates to the 'pValue' field in Gui#SDropdownValue struct.
            \param[in] sameLine Optional. If set to true, the widget will appear on the same line as the previous widget
        */
        void addDropdown(const std::string& name, const dropdown_list& values, int32_t* pVar, bool sameLine = false);

        /** Render a tooltip. This will display a small question mark next to the last label item rendered and will display the tooltip if the user hover over it
            \param[in] tip The tooltip's text
            \param[in] sameLine Optional. If set to true, the widget will appear on the same line as the previous widget
            */
        void addTooltip(const std::string& tip, bool sameLine = true);

        /** Create a new window
        */
        void pushWindow(const std::string& label, uint32_t width = 0, uint32_t height = 0, uint32_t x = 0, uint32_t y = 0);
        void popWindow();
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
