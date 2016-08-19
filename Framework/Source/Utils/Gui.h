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
#include "Core/RenderContext.h"

struct CTwBar;
typedef struct CTwBar TwBar;

namespace Falcor
{
    class Sample;
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

#define GUI_CALL __stdcall
        /** Callback definition of functions used when a button is pressed
            \param pUserData - user-defined data.
        */
        using ButtonCallback = void(GUI_CALL*)(void* pUserData);
        /** Callback definition of functions used the user changed a value in the UI
            \param pVal - Pointer to the value. The Type depends on the variable Type used to initialize the UI field.
            \param pUserData - user-defined data.
        */
        using SetVarCallback = void(GUI_CALL*)(const void* pVal, void* pUserData);
        /** Callback definition of a function used by the UI to retrieve a variable value.
            \param pVal - Pointer to the value. The Type depends on the variable Type used to initialize the UI field.
            \param pUserData - user-defined data.
        */
        using GetVarCallback = void(GUI_CALL*)(void* pVal, void* pUserData);

        /** This structs is used to initialize dropdowns
        */
        struct DropdownValue
        {
            int value;              ///< User defined index. Should be unique between different options.
            std::string label;      ///< Label of the dropdown option.
        };
        using dropdown_list = std::vector < DropdownValue > ;

        static void initialize(uint32_t windowWidth, uint32_t windowHeight, const RenderContext::SharedPtr& pRenderContext);

        /** create a new GUI object. New object create a new GUI window.
            \param[in] caption The title of the GUI window.
            \param[in] isVisible Optional. Sets the window visibility. By default the GUI is visible.
            \param[in] refreshRate Optional. Sets the window refresh rate. The default is 60HZ
        */
        static UniquePtr create(const std::string& caption, bool isVisible = true, float refreshRate = (1.f / 60.f));
        ~Gui();

        /** Render all of the GUI windows. If you want to render only part of windows, use SetVisibility() to hide windows.
        */
        static void drawAll();
        /** Set a global help message which is shown on the bottom-left corner of the screen.
        */
        static void setGlobalHelpMessage(const std::string& msg);

        /** Set global font size scaling. Has to be called BEFORE twInit()!
        */
        static void setGlobalFontScaling(float scale);

        /** Set the GUI window visibility
            \param[in] visible If true, window will be rendered. If false, window will not be rendered.
        */
        void setVisibility(bool visible);

        /** Get the size of the GUI window
            \param[out] size On return, will hold the width and height of the UI window
        */
        void getSize(uint32_t size[2]) const;
        /** Get the position of the GUI window
            \param[out] position On return, will hold the top-left screen coordinate of the UI window
        */
        void getPosition(uint32_t position[2]) const;

        /** Set the size of the GUI window
            \param[in] width Width of the GUI window
            \param[in] height Height of the GUI window
            */
        void setSize(uint32_t width, uint32_t height);
        /** Set the size of the GUI window
            \param[in] x Top-left x coordinate of the GUI window
            \param[in] y Top-left y coordinate of the GUI window
        */
        void setPosition(uint32_t x, uint32_t y);

        /** Set the refresh rate of the GUI.
            \param[in] rate The number of seconds between two updates.
        */
        void setRefreshRate(float rate);

        /** Add a separator
        */
        void addSeparator(const std::string& group = "");

        /** Adds a button
            \param[in] name The name of the button.
            \param[in] callback A user supplied callback function to call when the button is pressed.
            \param[in] pUserData A user data to pass when the callback function is called.
            \param[in] group - Optional. If not empty, will add the button under the requested UI group.
        */
        void addButton(const std::string& name, ButtonCallback callback, void* pUserData, const std::string& group = "");
        /** Adds a checkbox.
            \param[in] name The name of the checkbox.
            \param[in] pVar A pointer to a boolean that will be updated directly when the checkbox state changes.
            \param[in] group - Optional. If not empty, will add the button under the requested UI group.
        */
        void addCheckBox(const std::string& name, bool* pVar, const std::string& group = "");

        /** Adds a checkbox.
        \param[in] name The name of the checkbox.
        \param[in] setCallback - A user callback which will be called whenever the checkbox state changes.
        \param[in] getCallback - A user callback which the UI uses to query for the current checkbox state.
        \param[in] pUserData Private data which will be passed to the callback
        \param[in] group - Optional. If not empty, will add the button under the requested UI group.
        */
        void addCheckBoxWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group = "");

        /** Adds a text box.
        \param[in] name The name of the variable.
        \param[in] pVar A pointer to a string that will be updated directly when the text in the box changes.
        \param[in] group - Optional. If not empty, will add the button under the requested UI group.
        */
        void addTextBox(const std::string& name, std::string* pVar, const std::string& group = "");

        /** Adds a text box.
        \param[in] name The name of the variable.
        \param[in] setCallback - A user callback which will be called whenever the text in the box changes.
        \param[in] getCallback - A user callback which the UI uses to query for the current the text in the box 
        \param[in] pUserData Private data which will be passed to the callback
        \param[in] group - Optional. If not empty, will add the button under the requested UI group.
        */
        void addTextBoxWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group = "");

        /** Adds a directional UI widget.
            \param[in] name The name of the widget.
            \param[in] pVar A pointer to a vector that will be updated directly when the widget state changes.
            \param[in] group - Optional. If not empty, will add the checkbox under the requested UI group.
        */
        void addDir3FVar(const std::string& name, glm::vec3* pVar, const std::string& group = "");

		/** Adds a directional UI widget with callback functions.
            \param[in] name The name of the widget.
            \param[in] setCallback - A user callback which will be called whenever the color changes.
            \param[in] getCallback - A user callback which the UI uses to query for the current color state.
            \param[in] pUserData Private data which will be passed to the callback
            \param[in] group - Optional. If not empty, will add the checkbox under the requested UI group.
        */
        void addDir3FVarWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group = "");

        /** Adds a color UI widget.
            \param[in] name The name of the widget.
            \param[in] pVar A pointer to a vector that will be updated directly when the widget state changes.
            \param[in] group - Optional. If not empty, will add the element under the requested UI group.
        */
        void addRgbColor(const std::string& name, glm::vec3* pVar, const std::string& group = "");

        /** Adds a color UI widget with callback functions.
        \param[in] name The name of the widget.
        \param[in] setCallback - A user callback which will be called whenever the color changes.
        \param[in] getCallback - A user callback which the UI uses to query for the current color state.
        \param[in] pUserData Private data which will be passed to the callback
        \param[in] group - Optional. If not empty, will add the element under the requested UI group.
        */
        void addRgbColorWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group = "");

        /** Adds a floating-point UI element.
            \param[in] name The name of the widget.
            \param[in] pVar A pointer to a float that will be updated directly when the widget state changes.
            \param[in] group Optional. If not empty, will add the element under the requested UI group.
            \param[in] min Optional. The minimum allowed value for the float.
            \param[in] max Optional. The maximum allowed value for the float.
            \param[in] step Optional. The step rate for the float.
        */
        void addFloatVar(const std::string& name, float* pVar, const std::string& group = "", float min = -FLT_MAX, float max = FLT_MAX, float step = 0.001f);

        /** Adds a floating point variable with callback functions.
        \param[in] name The name of the widget.
        \param[in] setCallback - A user callback which will be called whenever the widget state changes.
        \param[in] getCallback - A user callback which the UI uses to query for the current widget state.
        \param[in] pUserData Private data which will be passed to the callback
        \param[in] group - Optional. If not empty, will add the button under the requested UI group.
        \param[in] min Optional. The minimum allowed value for the float.
        \param[in] max Optional. The maximum allowed value for the float.
        \param[in] step Optional. The step rate for the float.
        */
        void addFloatVarWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group = "", float min = -FLT_MAX, float max = FLT_MAX, float step = 0.001f);

        /** Adds a double-precision floating-point UI element.
        \param[in] name The name of the widget.
        \param[in] pVar A pointer to a float that will be updated directly when the widget state changes.
        \param[in] group Optional. If not empty, will add the element under the requested UI group.
        \param[in] min Optional. The minimum allowed value for the float.
        \param[in] max Optional. The maximum allowed value for the float.
        \param[in] step Optional. The step rate for the float.
        */
        void addDoubleVar(const std::string& name, double* pVar, const std::string& group = "", double min = -DBL_MAX, double max = DBL_MAX, double step = 0.001);

        /** Adds a double-precision floating point variable with callback functions.
        \param[in] name The name of the widget.
        \param[in] setCallback - A user callback which will be called whenever the widget state changes.
        \param[in] getCallback - A user callback which the UI uses to query for the current widget state.
        \param[in] pUserData Private data which will be passed to the callback
        \param[in] group - Optional. If not empty, will add the button under the requested UI group.
        \param[in] min Optional. The minimum allowed value for the float.
        \param[in] max Optional. The maximum allowed value for the float.
        \param[in] step Optional. The step rate for the float.
        */
        void addDoubleVarWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group = "", double min = -DBL_MAX, double max = DBL_MAX, double step = 0.001);

        /** Adds an integer UI element.
            \param[in] name The name of the widget.
            \param[in] pVar A pointer to an integer that will be updated directly when the widget state changes.
            \param[in] group Optional. If not empty, will add the element under the requested UI group.
            \param[in] min Optional. The minimum allowed value for the variable.
            \param[in] max Optional. The maximum allowed value for the variable.
            \param[in] step Optional. The step rate.
        */
        void addIntVar(const std::string& name, int32_t* pVar, const std::string& group = "", int min = -INT32_MAX, int max = INT32_MAX, int step = 1);

        /** Adds a integer variable with callback functions.
        \param[in] name The name of the widget.
        \param[in] setCallback - A user callback which will be called whenever the widget state changes.
        \param[in] getCallback - A user callback which the UI uses to query for the current widget state.
        \param[in] pUserData Private data which will be passed to the callback
        \param[in] group - Optional. If not empty, will add the button under the requested UI group.
        \param[in] min Optional. The minimum allowed value for the variable.
        \param[in] max Optional. The maximum allowed value for the variable.
        \param[in] step Optional. The step rate for the variable.
        */
        void addIntVarWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group = "", int min = -INT32_MAX, int max = INT32_MAX, int step = 1);

        /** Adds a dropdown menu. This will update a user variable directly, so the user has to keep track of that for changes.
            If you want notifications whenever the select option changed, use Gui#addDropdownWithCallback().
            \param[in] name The name of the dropdown menu.
            \param[in] values A list of options to show in the dropdown menu.
            \param[in] pVar A pointer to a user variable that will be updated directly when a dropdown option changes. This correlates to the 'pValue' field in Gui#SDropdownValue struct.
            \param[in] group - Optional. If not empty, will add the element under the requested UI group.
        */
        void addDropdown(const std::string& name, const dropdown_list& values, void* pVar, const std::string& group = "");
        /** Adds a dropdown menu. Whenever the selected options changes, it will call a user callback.
            \param[in] name The name of the dropdown menu.
            \param[in] values A list of options to show in the dropdown menu.
            \param[in] setCallback - A user callback which will be called whenever the selected option changes.
            \param[in] getCallback - A user callback which the UI uses to query for the current selected option. This allows the user to change options without actively calling the UI.
            \param[in] pUserData Private data which will be passed to the callback
            \param[in] group - Optional. If not empty, will add the dropdown under the requested UI group.
        */
        void addDropdownWithCallback(const std::string& name, const dropdown_list& values, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group = "");

        /** Hide/show a specific variable.
            \param[in] name name of the variable
            \param[in] isVisible True to show the variable, false to hide it.
        */
        void setVarVisibility(const std::string& name, const std::string& group, bool isVisible);

        /** Hide/show an entire group of variables
        \param[in] group name of the group
        \param[in] isVisible True to show the group, false to hide it.
        */
        void setGroupVisibility(const std::string& group, bool isVisible);

        /** Refresh the UI content. UI contents updated periodically. Call this function to force a refresh.
        */
        void refresh() const;
        /** Remove an element from the UI. This function silently ignores elements which do not exist.
            \param[in] name The name of the element to erase.
            \param[in] group - Optional. If not empty, will look for the element under the specified group.
        */
        void removeVar(const std::string& name, const std::string& group = "");

        /** Remove an entire group of elements from the UI, including all nested variables and groups.
        \param[in] group The name of the group to erase.
        */
        void removeGroup(const std::string& group);

        /** Nest element groups.
            \param[in] Parent The parent group. The child group will appear as part of it.
            \param[in] Child The child group.
        */
        void nestGroups(const std::string& Parent, const std::string& Child);
        /** Change the title of a variable. By default, variable title is the same as the variable name. Note that calling this function doesn't change the variable name, only the title in the UI.
            \param[in] varName The variable name
            \param[in] Title The title to show
        */
        void setVarTitle(const std::string& varName, const std::string& Title);

        /** Change the range of a floating-point variable
            \param[in] varName The variable name
            \param[in] group The group name used when creating the variable
            \param[in] min The min value
            \param[in] max The max value
        */
        void setVarRange(const std::string& varName, const std::string& group, float min, float max);

        /** Change the range of an integer variable
            \param[in] varName The variable name
            \param[in] group The group name used when creating the variable
            \param[in] min The min value
            \param[in] max The max value
        */
        void setVarRange(const std::string& varName, const std::string& group, int32_t min, int32_t max);

        /** Change whether a variable is in read-only or read-write mode (i.e., enable/disable interacting)
            \param[in] varName The variable name
            \param[in] group The group name used when creating the variable
            \param[in] readOnly If the variable will be readOnly (or user interactable)
        */
        void setVarReadOnly(const std::string& varName, const std::string& group, bool readOnly);

        /** Sets dropdown menu options to an existing variable
            \param[in] varName The name of the dropdown menu.
            \param[in] group The group name used when creating the variable
            \param[in] Values A list of options to show in the dropdown menu.
        */
        void setDropdownValues(const std::string& varName, const std::string& group, const dropdown_list& Values);

        /** Collapses a group
        */
        void collapseGroup(const std::string& group);

        /** Expands a group
        */
        void expandGroup(const std::string& group);
    protected:
        bool keyboardCallback(const KeyboardEvent& keyEvent);
        bool mouseCallback(const MouseEvent& mouseEvent);
        void windowSizeCallback(uint32_t width, uint32_t height);

    private:
        Gui() = default;

        template<uint32_t TwType, typename VarType>
        void addFloatVarWithCallbackInternal(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group, VarType min, VarType max, VarType step);

        template<uint32_t TwType, typename VarType>
        void addFloatVarInternal(const std::string& name, VarType* pVar, const std::string& group, VarType min, VarType max, VarType step);

        void displayTwError(const std::string& prefix);

        TwBar* mpTwBar;
        static bool sInitialized;
        static RenderContext::SharedPtr spRenderContext;

        struct StringCB
        {
            SetVarCallback pfnSetStringCB = nullptr;
            GetVarCallback pfnGetStringCB = nullptr;
            void* pUserData = nullptr;
        };

        std::vector<StringCB*> mStringCB;
        static void GUI_CALL getStringCB(void* pVal, void* pUserData);
        static void GUI_CALL setStringCB(const void* pVal, void* pUserData);
        static glm::vec2 sWindowSize;
    };
}
