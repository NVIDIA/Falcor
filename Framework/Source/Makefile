SRC  = Framework/Source/API/BlendState.cpp
SRC += Framework/Source/API/ComputeStateObject.cpp
SRC += Framework/Source/API/ConstantBuffer.cpp
SRC += Framework/Source/API/DepthStencilState.cpp
SRC += Framework/Source/API/FBO.cpp
SRC += Framework/Source/API/Formats.cpp
SRC += Framework/Source/API/GraphicsStateObject.cpp
SRC += Framework/Source/API/ProgramReflection.cpp
SRC += Framework/Source/API/ProgramVars.cpp
SRC += Framework/Source/API/ProgramVersion.cpp
SRC += Framework/Source/API/RenderContext.cpp
SRC += Framework/Source/API/Resource.cpp
SRC += Framework/Source/API/Sampler.cpp
SRC += Framework/Source/API/StructuredBuffer.cpp
SRC += Framework/Source/API/Texture.cpp
SRC += Framework/Source/API/TypedBuffer.cpp
SRC += Framework/Source/API/VAO.cpp
SRC += Framework/Source/API/VariablesBuffer.cpp

#Vulkan backend
SRC += Framework/Source/API/Vulkan/VKBuffer.cpp
SRC += Framework/Source/API/Vulkan/VKDevice.cpp
SRC += Framework/Source/API/Vulkan/VKGpuFence.cpp
SRC += Framework/Source/API/Vulkan/VKLowLevelContextData.cpp
SRC += Framework/Source/API/Vulkan/VKRenderContext.cpp


OBJ = $(SRC:.cpp=.o)
OUT = ./libfalcor.so
INCLUDES =  -I "./"  -I "./vulkan" -I "/home/nvidia/FALCOR/Falcor/Framework/Externals/GLM/" -I "/home/nvidia/FALCOR/Falcor/" -I "/home/nvidia/FALCOR/Falcor/wayland/" 
INCLUDES += -I "/home/nvidia/FALCOR/Falcor/Framework/Source/API/"
INCLUDES += -I "/home/nvidia/FALCOR/Falcor/Framework/Source/API/LowLevel"

CC = g++
CCC = g++
DEBUG = -g
CCFLAGS = -c $(DEBUG) -ggdb -std=c++11 -std=c++14 -DFALCOR_VK -DVK_USE_PLATFORM_WAYLAND_KHR -w
LDFLAGS = -lGL -lGLU -lglut -L/usr/lib/aarch64-linux-gnu/ /usr/lib/aarch64-linux-gnu/libvulkan.so.1
LIBS = -lGL -lGLU -lglut -L/usr/lib/aarch64-linux-gnu/ /usr/lib/aarch64-linux-gnu/libvulkan.so.1

LDFLAGS = -g
 
.SUFFIXES: .cpp
 
.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@
	
$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

clean:
	rm -f $(OBJ) $(OUT) Makefile.bak
