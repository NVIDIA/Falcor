Falcor21
============

Falcor is an OpenGL-based rendering framework.
Its features include:
* Abstract many common graphics operations, such as shader compilation, model loading and scene rendering.
* VR support using OpenVR.
* Common rendering effects such as shadows and post-processing effects.

There is a limited D3D11 support. The material system is implemented in GLSL so won't compile when using HLSL. We are working on a cross-compiler to solve this issue.

D3D12 is coming soon.

Prerequisites
------------------------
* Visual Studio 2013.
* NVIDIA® Geforce™ 900 series or better GPU.
The project uses extentions which are supported only by NVIDIA GPUs.

Creating a New Project
------------------------
- If you haven't done so already, create a visual-studio solution and project for your code. Falcor only supports 64-bit builds, so make sure you have a 64-bit build configuration.
- Add Falcor.props to your project (Property Manager -> Right click your project -> Add existing property sheet).
- Add Falcor.vcxproj to your solution.
- Add a reference to Falcor in your project (Solution Explorer -> Right click your project -> Properties -> Common Properties -> References -> Add new reference -> Choose Falcor).

Optional Modules
----------------
To create CUDA applications you will need to install the CUDA Toolkit 7.5.  
For VR applications development, you will need the SDK which came with your headset (SteamVR for Vive, Oculus SDK, etc.).

*Sample* Class
-------------------
This is the bootstrapper class of the application. Your class should inherit from it and override its protected methods which serve as the callback functions.  
A good place to start would be the ModelViewer sample.


Build Configurations
--------------------
Falcor has two build configurations:
* `Debug` - This configuration will create an OpenGL debug context. By default, file logging and message boxes are enabled, and there is a lot of runtime error checking.
* `Release` - This configuration creates a regular, non-debug context. Logging and most runtime error checks are disabled. Use this configuration to measure performance.

Falcor Configuration
--------------------
`FalcorConfig.h` contains some flags which control Falcor's behavior.
* `_LOG_ENABLED` - Enable/disable log messages. By default, this is `false` for release build and `true` for debug builds.
* `_PROFILING_ENABLED` - Enable/Disable the internal CPU/GPU profiler. By default, this is `false`.

Data Files
--------------------
Data files include shader files, textures and models.  
By default, Falcor looks for data files in the current locations:
- The working directory. In some cases this is not the same as the executable directory. For example, if you launch the application from Visual Studio, by default the working directory is the directory containing the project file.
- The executable directory.
- An optional environment variable named `FALCOR_MEDIA_FOLDERS`. It is a semicolon-separated list of folders.
- Any directory that was added to the data directories list by calling `addDataDirectory()`.
- A directory called "Data/" under any of the above directories.

To search for a data file, call `findFileInDataDirectories()`.

Deployment
----------
The best practice is to create a directory called "Data/" next to your **project** file and place all your data files there (shaders/models).  If that directory exists, Falcor will copy it to the output directory, making the output directory self-contained (you can zip only the output directory and it should work).  If not, you will have to copy the data files yourself.

Citation
--------
If you use Falcor in a research project leading to a publication, please cite the project.
The BibTex entry is

@Misc{Benty16,  
   author =      {Nir Benty},  
   title =       {The {Falcor} Rendering Framework},  
   year =        {2016},  
   month =       {08},  
   url =         {https://github.com/NVIDIA/Falcor},  
   note=         {\url{https://github.com/NVIDIA/Falcor}}  
}
