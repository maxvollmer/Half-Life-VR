
# GLProxy

Single source file function call interceptor for OpenGL. This project provides
a basic skeleton for building more sophisticated OpenGL interceptors and detours.

The template project, `opengl_proxy.cpp` provides wrapper functions for the
"classic OpenGL", AKA fixed function OpenGL, plus WGL on Windows and a few log utilities.
As is stands, it can be used for basic interception of old OpenGL applications and GL-based games.

Currently only building for Windows. Tested on Windows 7, Visual Studio 2015.

## How it works

The default DLL search strategy on Windows is:

- Application directory
- System directories

So we can take advantage of that and override system libraries, like `opengl32.dll`,
by simply placing a new DLL with the same name and interface into the application directory.

GLProxy provides a template DLL project that defines a thin wrapper over OpenGL functions
that in itself just forwards the calls to the actual OpenGL library. When compiled, it will
output a proxy `opengl32.dll` that you can place in the install directory of an application
and the app will reference it instead of the "real" OpenGL when run.

This opens the door for a lot of interesting things, from just logging every GL call
made by an application, to saving draw calls to a file for later playback.

This project is just a base skeleton for anyone interested in writing their own interceptors,
hence why it is small and entirely contained inside a single source file (`opengl_proxy.cpp`).
It provides wrappers for most of the old fixed function OpenGL + WGL. The wrapper just forwards
each call to the real OpenGL DLL, but it will also keep a global call count for each function
and attempt to write a log file when the process terminates with the counts for each function.

For more robust uses and more mature projects, I suggest checking:

- [GLIntercept](https://github.com/dtrebilco/glintercept)
- [APITrace](https://github.com/apitrace/apitrace)
- [RenderDoc](https://github.com/baldurk/renderdoc)

## License

This project's source code is released under the [MIT License](http://opensource.org/licenses/MIT).

