@echo off

set code=%cd%\src
set glfw_include_path=%cd%\thirdparty\glfw-3.4\include
set glew_include_path=%cd%\thirdparty\glew-2.1.0\include
set thirdparty_include_path=%cd%\thirdparty
set opts=-nologo /MDd -diagnostics:column -Zi -Fesidescroller.exe /I%glfw_include_path% /I%glew_include_path% /I%thirdparty_include_path% /EHsc -DDEBUG
set glfw_lib_path=%cd%\thirdparty\glfw-3.4\build\src\Debug
set glew_lib_path=%cd%\thirdparty\glew-2.1.0\lib\Debug\x64

pushd build
cl %opts% %code%\platform.cpp /link user32.lib shell32.lib opengl32.lib gdi32.lib /LIBPATH:%glew_lib_path% glew32sd.lib /LIBPATH:%glfw_lib_path% glfw3.lib 
popd
