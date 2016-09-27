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
#include "Gui.h"
#include <sstream>
#include "AntTweakBar.h"
#include "OS.h"
#include "Utils/UserInput.h"
#include "Core/RenderContext.h"

namespace Falcor
{
    bool Gui::sInitialized = false;
    glm::vec2 Gui::sWindowSize;

    RenderContext::SharedPtr Gui::spRenderContext;

    TwMouseButtonID getAtbMouseButton(MouseEvent::Type t)
    {
        switch(t)
        {
        case MouseEvent::Type::LeftButtonDown:
        case MouseEvent::Type::LeftButtonUp:
            return TW_MOUSE_LEFT;
        case MouseEvent::Type::MiddleButtonDown:
        case MouseEvent::Type::MiddleButtonUp:
            return TW_MOUSE_MIDDLE;
        case MouseEvent::Type::RightButtonDown:
        case MouseEvent::Type::RightButtonUp:
            return TW_MOUSE_RIGHT;
        default:
            should_not_get_here();
            return (TwMouseButtonID)0;
        }
    }
    int32_t getAtbModifiers(const InputModifiers& mod)
    {
        int32_t GlfwMod = TW_KMOD_NONE;
        if(mod.isAltDown)
            GlfwMod |= TW_KMOD_ALT;
        if(mod.isCtrlDown)
            GlfwMod |= TW_KMOD_CTRL;
        if(mod.isShiftDown)
            GlfwMod |= TW_KMOD_SHIFT;
        return GlfwMod;
    }

    int32_t getAtbKey(KeyboardEvent::Key k)
    {
        // ASCII codes
        if(k < KeyboardEvent::Key::SpecialKeys)
        {
            return int32_t(k);
        }

        // Everything else
        switch(k)
        {
        case KeyboardEvent::Key::Escape:
            return TW_KEY_ESCAPE;
        case KeyboardEvent::Key::Enter:
            return TW_KEY_RETURN;
        case KeyboardEvent::Key::Tab:
            return TW_KEY_TAB;
        case KeyboardEvent::Key::Backspace:
            return TW_KEY_BACKSPACE;
        case KeyboardEvent::Key::Insert:
            return TW_KEY_INSERT;
        case KeyboardEvent::Key::Del:
            return TW_KEY_DELETE;
        case KeyboardEvent::Key::Right:
            return TW_KEY_RIGHT;
        case KeyboardEvent::Key::Left:
            return TW_KEY_LEFT;
        case KeyboardEvent::Key::Down:
            return TW_KEY_DOWN;
        case KeyboardEvent::Key::Up:
            return TW_KEY_UP;
        case KeyboardEvent::Key::PageUp:
            return TW_KEY_PAGE_UP;
        case KeyboardEvent::Key::PageDown:
            return TW_KEY_PAGE_DOWN;
        case KeyboardEvent::Key::Home:
            return TW_KEY_HOME;
        case KeyboardEvent::Key::End:
            return TW_KEY_END;
        case KeyboardEvent::Key::Pause:
            return TW_KEY_PAUSE;
        case KeyboardEvent::Key::F1:
            return TW_KEY_F1;
        case KeyboardEvent::Key::F2:
            return TW_KEY_F2;
        case KeyboardEvent::Key::F3:
            return TW_KEY_F3;
        case KeyboardEvent::Key::F4:
            return TW_KEY_F4;
        case KeyboardEvent::Key::F5:
            return TW_KEY_F5;
        case KeyboardEvent::Key::F6:
            return TW_KEY_F6;
        case KeyboardEvent::Key::F7:
            return TW_KEY_F7;
        case KeyboardEvent::Key::F8:
            return TW_KEY_F8;
        case KeyboardEvent::Key::F9:
            return TW_KEY_F9;
        case KeyboardEvent::Key::F10:
            return TW_KEY_F10;
        case KeyboardEvent::Key::F11:
            return TW_KEY_F11;
        case KeyboardEvent::Key::F12:
            return TW_KEY_F12;
        case KeyboardEvent::Key::Keypad0:
            return '0';
        case KeyboardEvent::Key::Keypad1:
            return '1';
        case KeyboardEvent::Key::Keypad2:
            return '2';
        case KeyboardEvent::Key::Keypad3:
            return '3';
        case KeyboardEvent::Key::Keypad4:
            return '4';
        case KeyboardEvent::Key::Keypad5:
            return '5';
        case KeyboardEvent::Key::Keypad6:
            return '6';
        case KeyboardEvent::Key::Keypad7:
            return '7';
        case KeyboardEvent::Key::Keypad8:
            return '8';
        case KeyboardEvent::Key::Keypad9:
            return '9';
        case KeyboardEvent::Key::KeypadDel:
            return TW_KEY_DELETE;
        case KeyboardEvent::Key::KeypadDivide:
            return '/';
        case KeyboardEvent::Key::KeypadMultiply:
            return '*';
        case KeyboardEvent::Key::KeypadSubtract:
            return '-';
        case KeyboardEvent::Key::KeypadAdd:
            return '+';
        case KeyboardEvent::Key::KeypadEnter:
            return TW_KEY_RETURN;
        case KeyboardEvent::Key::KeypadEqual:
            return '=';
        default:
            return 0;
        }
    }

    void TW_CALL copyStdStringToClient(std::string& dst, const std::string& src)
    {
        dst = src;
    }

    void GUI_CALL Gui::getStringCB(void* pVal, void* pUserData)
    {
        std::string userString;
        StringCB* pCB = (StringCB*)pUserData;
        pCB->pfnGetStringCB(&userString, pCB->pUserData);
        TwCopyStdStringToLibrary(*(std::string*)pVal, userString);
    }

    void GUI_CALL Gui::setStringCB(const void* pVal, void* pUserData)
    {
        StringCB* pCB = (StringCB*)pUserData;
        pCB->pfnSetStringCB(pVal, pCB->pUserData);
    }

    void Gui::initialize(uint32_t windowWidth, uint32_t windowHeight, const RenderContext::SharedPtr& pRenderContext)
    {
        if(sInitialized == false)
        {
            sInitialized = true;
#ifdef FALCOR_GL
            TwInit(TW_OPENGL_CORE, nullptr);
#elif defined FALCOR_D3D11
            TwInit(TW_DIRECT3D11, getD3D11Device());
#endif
            TwDefine(" TW_HELP visible=false ");
            TwWindowSize(windowWidth, windowHeight);
            TwCopyStdStringToClientFunc(copyStdStringToClient);
            sWindowSize.x = (float)windowWidth;
            sWindowSize.y = (float)windowHeight;
            Gui::spRenderContext = pRenderContext;
        }
    }

    Gui::UniquePtr Gui::create(const std::string& caption, bool isVisible, float refreshRate)
    {
        if(sInitialized == false)
        {
            Logger::log(Logger::Level::Error, "Gui::Initialize() has to be called before creating Gui object.");
        }

        UniquePtr pGui = UniquePtr(new Gui);
        pGui->mpTwBar = TwNewBar(caption.c_str());
        pGui->setVisibility(isVisible);
        pGui->setRefreshRate(refreshRate);
        return pGui;
    }

    Gui::~Gui()
    {
        for(auto& a : mStringCB)
        {
            safe_delete(a);
        }

        TwDeleteBar(mpTwBar);
    }

    void Gui::displayTwError(const std::string& prefix)
    {
        std::string error(TwGetLastError());
        Logger::log(Logger::Level::Error, prefix + "\n" + error);
    }

    void Gui::getSize(uint32_t size[2]) const
    {
        TwGetParam(mpTwBar, nullptr, "size", TW_PARAM_INT32, 2, size);
    }

    void Gui::getPosition(uint32_t position[2]) const
    {
        TwGetParam(mpTwBar, nullptr, "position", TW_PARAM_INT32, 2, position);
    }

    void Gui::setSize(uint32_t width, uint32_t height)
    {
        uint32_t size[] = {width, height};
        TwSetParam(mpTwBar, nullptr, "size", TW_PARAM_INT32, 2, size);
    }

    void Gui::setPosition(uint32_t x, uint32_t y)
    {
        uint32_t topLeft[] = {x, y};
        TwSetParam(mpTwBar, nullptr, "position", TW_PARAM_INT32, 2, topLeft);
    }

    void Gui::setRefreshRate(float rate)
    {
        TwSetParam(mpTwBar, nullptr, "refresh", TW_PARAM_FLOAT, 1, &rate);
    }

    void Gui::drawAll()
    {
        spRenderContext->pushState();
        // DISABLED_FOR_D3D12
//        spRenderContext->setRasterizerState(nullptr);
        TwDraw();
        spRenderContext->popState();
    }

    void Gui::setGlobalHelpMessage(const std::string& msg)
    {
        std::string TwMsg = std::string(" GLOBAL help='") + msg + "' ";
        TwDefine(TwMsg.c_str());
    }

    void Gui::setGlobalFontScaling(float scale)
    {
        TwDefine((" GLOBAL fontscaling="+std::to_string(scale)+" ").c_str());
    }

    __forceinline std::string getGroupParamString(const std::string& group)
    {
        std::string tmp;
        if(group.size() != 0)
        {
            tmp = " group=\"" + group + "\"";
        }
        return tmp;
    }

    __forceinline std::string getTitleParamString(const std::string& title)
    {
        std::string tmp;
        if(title.size() != 0)
        {
            tmp = " label=\"" + title + "\"";
        }
        return tmp;
    }

    void Gui::addSeparator(const std::string& group)
    {
        static uint32_t i = 0;
        i++;
        std::string name = "Sep" + std::to_string(i);
        int res = TwAddSeparator(mpTwBar, name.c_str(), getGroupParamString(group).c_str());
        if(res == 0)
        {
            displayTwError("Error when adding separator \"" + name + "\"");
        }
    }

    void Gui::addButton(const std::string& name, Gui::ButtonCallback callback, void* pUserData, const std::string& group)
    {
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddButton(mpTwBar, (group + '/' + name).c_str(), callback, pUserData, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating button \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addCheckBox(const std::string& name, bool* pVar, const std::string& group)
    {
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarRW(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_BOOL8, pVar, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating checkbox \"" + name + "\"");
        }
        collapseGroup(group);
    }


    void Gui::addCheckBoxWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group)
    {
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarCB(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_BOOL8, setCallback, getCallback, pUserData, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating checkbox \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addTextBox(const std::string& name, std::string* pVar, const std::string& group)
    {
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarRW(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_STDSTRING, pVar, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating text box \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addTextBoxWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group)
    {
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        StringCB* pCB = new StringCB;
        pCB->pfnGetStringCB = getCallback;
        pCB->pfnSetStringCB = setCallback;
        pCB->pUserData = pUserData;
        mStringCB.push_back(pCB);

        int res = TwAddVarCB(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_STDSTRING, &Gui::setStringCB, &Gui::getStringCB, pCB, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating text box \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addDir3FVar(const std::string& name, glm::vec3* pVar, const std::string& group)
    {
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarRW(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_DIR3F, pVar, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating Dir3Var \"" + name + "\"");
        }
        collapseGroup(group);
    }

	void Gui::addDir3FVarWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group)
    {
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarCB(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_DIR3F, setCallback, getCallback, pUserData, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating Dir3Var \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addRgbColor(const std::string& name, glm::vec3* pVar, const std::string& group)
    {
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarRW(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_COLOR3F, pVar, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating RgbColor \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addRgbColorWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group)
    {
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarCB(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_COLOR3F, setCallback, getCallback, pUserData, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating RgbColor \"" + name + "\"");
        }
        collapseGroup(group);
    }

    template<uint32_t TwType, typename VarType>
    void Gui::addFloatVarInternal(const std::string& name, VarType* pVar, const std::string& group, VarType min, VarType max, VarType Step)
    {
        std::string param = " min=" + std::to_string(min) + " max=" + std::to_string(max) + " step=" + std::to_string(Step) + getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarRW(mpTwBar, (group + '/' + name).c_str(), (ETwType)TwType, pVar, param.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating float var \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addFloatVar(const std::string& name, float* pVar, const std::string& group, float min, float max, float step)
    {
        addFloatVarInternal<TW_TYPE_FLOAT, float>(name, pVar, group, min, max, step);
    }

    void Gui::addDoubleVar(const std::string& name, double* pVar, const std::string& group, double min, double max, double step)
    {
        addFloatVarInternal<TW_TYPE_DOUBLE, double>(name, pVar, group, min, max, step);
    }

    template<uint32_t TwType, typename VarType>
    void Gui::addFloatVarWithCallbackInternal(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group, VarType min, VarType max, VarType step)
    {
        std::string param = " min=" + std::to_string(min) + " max=" + std::to_string(max) + " step=" + std::to_string(step) + getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarCB(mpTwBar, (group + '/' + name).c_str(), (ETwType)TwType, setCallback, getCallback, pUserData, param.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating float var \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addFloatVarWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group, float min, float max, float step)
    {
        addFloatVarWithCallbackInternal<TW_TYPE_FLOAT, float>(name, setCallback, getCallback, pUserData, group, min, max, step);
    }

    void Gui::addDoubleVarWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group, double min, double max, double step)
    {
        addFloatVarWithCallbackInternal<TW_TYPE_DOUBLE, double>(name, setCallback, getCallback, pUserData, group, min, max, step);
    }

    void Gui::addIntVar(const std::string& name, int32_t* pVar, const std::string& group, int min, int max, int step)
    {
        std::string param = " min=" + std::to_string(min) + " max=" + std::to_string(max) + " step=" + std::to_string(step) + getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarRW(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_INT32, pVar, param.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating int var \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addIntVarWithCallback(const std::string& name, SetVarCallback setCallback, GetVarCallback getCallback, void* pUserData, const std::string& group, int min, int max, int step)
    {
        std::string param = " min=" + std::to_string(min) + " max=" + std::to_string(max) + " step=" + std::to_string(step) + getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarCB(mpTwBar, (group + '/' + name).c_str(), TW_TYPE_INT32, setCallback, getCallback, pUserData, param.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating int var \"" + name + "\"");
        }
        collapseGroup(group);

    }

    std::vector<TwEnumVal> ConvertDropdownList(const Gui::dropdown_list& guiList)
    {
        std::vector<TwEnumVal> twList;
        for(const auto& GuiVal : guiList)
        {
            TwEnumVal TwVal;
            TwVal.Label = GuiVal.label.c_str();
            TwVal.Value = GuiVal.value;
            twList.push_back(TwVal);
        }

        return twList;
    }

    void Gui::addDropdown(const std::string& name, const dropdown_list& values, void* pVar, const std::string& group)
    {
        auto wwList = ConvertDropdownList(values);
        TwType enumType = TwDefineEnum(name.c_str(), wwList.data(), uint32_t(wwList.size()));
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarRW(mpTwBar, (group + '/' + name).c_str(), enumType, pVar, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating dropdown \"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::addDropdownWithCallback(const std::string& name, const dropdown_list& values, TwSetVarCallback setCallback, TwGetVarCallback getCallback, void* pUserData, const std::string& group)
    {
        auto twList = ConvertDropdownList(values);
        TwType enumType = TwDefineEnum((group + '/' + name).c_str(), &twList[0], uint32_t(twList.size()));
        std::string params = getGroupParamString(group) + getTitleParamString(name);
        int res = TwAddVarCB(mpTwBar, (group + '/' + name).c_str(), enumType, setCallback, getCallback, pUserData, params.c_str());
        if(res == 0)
        {
            displayTwError("Error when creating dropdown with callback\"" + name + "\"");
        }
        collapseGroup(group);
    }

    void Gui::setDropdownValues(const std::string& varName, const std::string& group, const dropdown_list& Values)
    {
        auto twList = ConvertDropdownList(Values);
        TwDefineEnum((group + '/' + varName).c_str(), &twList[0], uint32_t(twList.size()));
    }

    void Gui::nestGroups(const std::string& parent, const std::string& child)
    {
        TwSetParam(mpTwBar, child.c_str(), "group", TW_PARAM_CSTRING, 1, parent.c_str());
        collapseGroup(parent);
    }

    void Gui::collapseGroup(const std::string& group)
    {
        if(group.size())
        {
            uint32_t opened = 0;
            TwSetParam(mpTwBar, group.c_str(), "opened", TW_PARAM_INT32, 1, &opened);
        }
    }

    void Gui::expandGroup(const std::string& group)
    {
        if(group.size())
        {
            uint32_t opened = 1;
            TwSetParam(mpTwBar, group.c_str(), "opened", TW_PARAM_INT32, 1, &opened);
        }
    }

    void Gui::setVarTitle(const std::string& varName, const std::string& title)
    {
        TwSetParam(mpTwBar, varName.c_str(), "label", TW_PARAM_CSTRING, 1, title.c_str());
    }

    void Gui::setVisibility(bool visible)
    {
        int v = visible ? 1 : 0;
        TwSetParam(mpTwBar, nullptr, "visible", TW_PARAM_INT32, 1, &v);
    }

    void Gui::setVarVisibility(const std::string& name, const std::string& group, bool visible)
    {
        TwSetParam(mpTwBar, (group + '/' + name).c_str(), "visible", TW_PARAM_CSTRING, 1, visible ? "true" : "false");
    }

    void Gui::setGroupVisibility(const std::string& group, bool visible)
    {
        TwSetParam(mpTwBar, group.c_str(), "visible", TW_PARAM_CSTRING, 1, visible ? "true" : "false");
    }

    void Gui::setVarReadOnly( const std::string& varName, const std::string& group, bool readOnly )
    {
        TwSetParam(mpTwBar, (group + '/' + varName).c_str(), "readonly", TW_PARAM_CSTRING, 1, readOnly ? "true" : "false");
    }

    void Gui::removeVar(const std::string& name, const std::string& group)
    {
        TwRemoveVar(mpTwBar, (group + '/' + name).c_str());
    }

    void Gui::removeGroup(const std::string& group)
    {
        TwRemoveVar(mpTwBar, (group).c_str());
    }

    void Gui::refresh() const
    {
        TwRefreshBar(mpTwBar);
    }

    void Gui::setVarRange(const std::string& varName, const std::string& group, float min, float max)
    {
        int res = TwSetParam(mpTwBar, (group + '/' + varName).c_str(), "min", TW_PARAM_FLOAT, 1, &min);
        if(res == 0)
        {
            displayTwError("Error when setting min value for variable \"" + varName + "\"");
        }
        res = TwSetParam(mpTwBar, (group + '/' + varName).c_str(), "max", TW_PARAM_FLOAT, 1, &max);
        if(res == 0)
        {
            displayTwError("Error when setting max value for variable \"" + varName + "\"");
        }
    }

    void Gui::setVarRange(const std::string& varName, const std::string& group, int32_t min, int32_t max)
    {
        int res = TwSetParam(mpTwBar, (group + '/' + varName).c_str(), "min", TW_PARAM_INT32, 1, &min);
        if(res == 0)
        {
            displayTwError("Error when setting min value for variable \"" + varName + "\"");
        }
        res = TwSetParam(mpTwBar, (group + '/' + varName).c_str(), "max", TW_PARAM_INT32, 1, &max);
        if(res == 0)
        {
            displayTwError("Error when setting max value for variable \"" + varName + "\"");
        }
    }

    bool Gui::keyboardCallback(const KeyboardEvent& keyEvent)
    {
        if(keyEvent.type == KeyboardEvent::Type::KeyPressed)
        {
            int32_t atbKey = getAtbKey(keyEvent.key);
            if(atbKey != 0)
            {
                int32_t atbMod = getAtbModifiers(keyEvent.mods);
                return (TwKeyPressed(atbKey, atbMod) == 1);
            }
        }
        return false; 
    }

    bool Gui::mouseCallback(const MouseEvent& mouseEvent)
    {
        switch(mouseEvent.type)
        {
        case MouseEvent::Type::Move:
        {
            auto pos = mouseEvent.pos * sWindowSize;
            return (TwMouseMotion((int)pos.x, (int)pos.y) == 1);
        }
        case MouseEvent::Type::Wheel:
            return (TwMouseWheel((int)mouseEvent.wheelDelta.y) == 1);
        case MouseEvent::Type::LeftButtonDown:
        case MouseEvent::Type::RightButtonDown:
        case MouseEvent::Type::MiddleButtonDown:
            return (TwMouseButton(TW_MOUSE_PRESSED, getAtbMouseButton(mouseEvent.type)) == 1);

        case MouseEvent::Type::LeftButtonUp:
        case MouseEvent::Type::RightButtonUp:
        case MouseEvent::Type::MiddleButtonUp:
            return (TwMouseButton(TW_MOUSE_RELEASED, getAtbMouseButton(mouseEvent.type)) == 1);
        default:
            should_not_get_here();
            return false;
        }
    }

    void Gui::windowSizeCallback(uint32_t width, uint32_t height)
    {
        sWindowSize.x = (float)width;
        sWindowSize.y = (float)height;
        TwWindowSize(width, height);
    }

}
