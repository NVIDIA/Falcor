Falcor
============

Falcor is an OpenGL-based rendering framework.
Its features include:
* Abstract many common graphics operations, such as shader compilation, model loading and scene rendering.
* VR support using OpenVR.
* Common rendering effects such as shadows and post-processing effects.

Prerequisites
------------------------
* Visual Studio 2013.
* GPU with OpenGL 4.4 support and the following extensions:
	* `GL_ARB_Bindless_texture`
	* `GL_NV_uniform_buffer_unified_memory`
	* `GL_EXT_texture_filter_anisotropic`
	* `GL_NV_shader_buffer_load`
	If you are using an NVIDIA GPU, Kepler or newer is required.
	
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

Falcor will also look for a directory called "Data/" under those directories.<br/>
To search for a data file, call `findFileInDataDirectories()`.<br/>
To add a new data directory, call `addDataDirectory()`.

Falcor will also look for an optional environment variable named `FALCOR_MEDIA_FOLDERS`. It is a semicolon-separated list of folders Falcor will look in when searching for media files. If you have a common 3D models repository, it's a good practice to set this environment variable to point to these locations.

Deployment
----------
The best practice is to create a directory called "Data/" next to your project file and place all your data files there (shaders/models).  If that directory exists, Falcor will copy it to the output directory, making the output directory self-contained (you can zip only the output directory and it should work).  If not, you will have to copy the data files yourself.
