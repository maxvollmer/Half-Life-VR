# <img src="../art/game_icon.png" alt="HLVR Game Icon" width="50"/> Half-Life: VR

A highly immersive Virtual Reality mod for the classic Half-Life from 1998.

---

[Back to Main README](README.md)

---

### Developer Guide

#### Prerequisites

- Visual Studio 2022
   - .NET Framework 4.7.2
   - C++17
- Python 3
- NuGet Package Manager
- Steam with SteamVR
- Valid installation of Half-Life or Half-Life: VR
- Possibly something else I forgot to list.

---
#### Build The Mod

All source code lives in [src](../src). 

1. Open [shared.props](../src/shared.props) and make sure `HLVRInstallDirectory` points to the directory where you have Half-Life: VR installed.
    - If you don't have Half-Life: VR installed, see [Create a Half-Life: VR folder from Half-Life](#create-a-half-life-vr-folder-from-half-life) below.
    - Visual Studio will automagically copy the generated files into your Half-Life: VR installation. The debugger will also correctly launch Half-Life: VR out of the box.

2. Open [HLVR.sln](../src/HLVR.sln) with Visual Studio.

3. To build `HLVRConfig` you need to ensure all NuGet packages are installed. Please refer to NuGet documentation if you don't know how.

4. After building the game DLLs, Visual Studio runs `DLLIncludeFixer.py`. For this you need Python. Without this script, Half-Life: VR will use the wrong Steam API DLL and the game will crash.

If there are issues or if you have questions, join my Discord Server [Max Makes Mods](https://discord.gg/jujwEGf62K) to ask for support!

---
#### Create a Half-Life: VR folder from Half-Life

Half-Life: VR is a heavily modified version of Half-Life. Building the source in this repo creates `client.dll`, `hl.dll`, and `HLVRConfig.exe`. For the full mod you still need several original game files as well as certain game files modified for VR.

If you don't have Half-Life: VR installed, e.g. if you are following this guide before the Steam release, you need to create your own Half-Life: VR folder. For this you need a valid installation of Half-Life. You can create the Half-Life: VR folder manually, or you can use a script provided in this repository.

##### 1. Using the CreateHLVRFromHL.py script

  - Run [CreateHLVRFromHL.py](../src/CreateHLVRFromHL.py) and provide the path to your Half-Life installation like so:  
    `python CreateHLVRFromHL.py -h "Path/To/SteamLibrary/steamapps/common/Half-Life"`
  - The script will create a new `Half-Life VR` folder in the same location where your `Half-Life` folder is.
  - Some things to consider:
    - You must run the script from the `src` folder in a complete clone of this repo.
    - Your Half-Life installation must be a valid installation from Steam.
    - No `Half-Life VR` folder must exist yet.
    - You must have NuGet packages for `HLVRConfig` installed.
  - If the script encounters any problems, it will output an error message with a detailed explanation.

##### 2. Creating the folder manually

1. Create a copy of your `Half-Life` folder and name it `Half-Life VR`.

2. Copy the [sound](../art/sound), [textures](../art/textures), [sprites](../art/sprites), and [models](../art/models) folders into `Half-Life VR/valve`.
    - If you are prompted, confirm to override any files.

3. Copy the [actions](../game/actions) and [resource](../game/resource) folders into `Half-Life VR/valve`.
    - If you are prompted, confirm to override any files.

4. Copy third-party libraries directly into `Half-Life VR`:
    - CEGUI and dependencies:
      - [CEGUIBase-0.dll](../src/cegui/bin/CEGUIBase-0.dll)
      - [CEGUICommonDialogs-0.dll](../src/cegui/bin/CEGUICommonDialogs-0.dll)
      - [CEGUICoreWindowRendererSet.dll](../src/cegui/bin/CEGUICoreWindowRendererSet.dll)
      - [CEGUIExpatParser.dll](../src/cegui/bin/CEGUIExpatParser.dll)
      - [CEGUIOpenGLRenderer-0.dll](../src/cegui/bin/CEGUIOpenGLRenderer-0.dll)
      - [CEGUISILLYImageCodec.dll](../src/cegui/bin/CEGUISILLYImageCodec.dll)
      - [freetype.dll](../src/cegui/bin/freetype.dll)
      - [glew.dll](../src/cegui/bin/glew.dll)
      - [glfw.dll](../src/cegui/bin/glfw.dll)
      - [jpeg.dll](../src/cegui/bin/jpeg.dll)
      - [libpng.dll](../src/cegui/bin/libpng.dll)
      - [pcre.dll](../src/cegui/bin/pcre.dll)
      - [SILLY.dll](../src/cegui/bin/SILLY.dll)
      - [zlib.dll](../src/cegui/bin/zlib.dll)
    - OpenVR (SteamVR):
      - [openvr_api.dll](../src/cl_dll/openvr/openvr_api.dll)
    - Client DLLS for 3D Sound:
      - [EasyHook32.dll](../src/cl_dll/EasyHook/bin/EasyHook32.dll)
      - [fmod.dll](../src/cl_dll/fmod/lib/x86/fmod.dll)
      - [fmodL.dll](../src/cl_dll/fmod/lib/x86/fmodL.dll)
    - HLVRConfig NuGet dependencies (you can find these in your NuGet packages folder after installing the packages):
      - Microsoft.Experimental.Collections.dll
      - Newtonsoft.Json.dll
      - MarkdownSharp.dll
      - HtmlToXamlConverter.dll

5. If it exists, delete the folder `valve_hd`.

Now follow the [Build The Mod](#build-the-mod) instructions above to build `client.dll`, `hl.dll`, and `HLVRConfig.exe`.

---
That should be it. If you run into problems or have questions, join my Discord Server [Max Makes Mods](https://discord.gg/jujwEGf62K)!
