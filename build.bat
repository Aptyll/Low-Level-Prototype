@echo off
echo Building WASM game...

cd /d "%~dp0"

REM Set up Emscripten environment
call emsdk\emsdk_env.bat

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Compile C++ to WASM
emcc src/cpp/main.cpp ^
    src/cpp/engine/renderer.cpp ^
    src/cpp/engine/game.cpp ^
    -s USE_WEBGL2=1 ^
    -s MIN_WEBGL_VERSION=2 ^
    -s MAX_WEBGL_VERSION=2 ^
    -s WASM=1 ^
    -s EXPORTED_FUNCTIONS=_init,_start_game_loop,_resize_renderer,_malloc,_free ^
    -s EXPORTED_RUNTIME_METHODS=ccall,cwrap ^
    -s MODULARIZE=1 ^
    -s EXPORT_NAME=Module ^
    -O2 ^
    -o build/game.js

echo Build complete! Output: build/game.js
