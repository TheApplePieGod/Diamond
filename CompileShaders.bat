if not exist "build\shaders\" mkdir "build\shaders\"
del /q build\shaders\*.spv
cd src/shaders
for %%i in (*) do glslc.exe %%i -o ../../build/shaders/%%i.spv