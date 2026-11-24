param([switch]$Run)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    if (!$env:VULKAN_SDK) { throw 'Install the LunarG Vulkan SDK and set VULKAN_SDK.' }
    New-Item -ItemType Directory -Force -Path build | Out-Null
    foreach ($stage in 'vert','frag') {
        & "$env:VULKAN_SDK/Bin/glslc.exe" "-mfmt=c" "shaders/map.$stage" -o "build/$stage.inc"
        if ($LASTEXITCODE) { throw "Failed to compile $stage shader" }
    }
    $compiler = (Get-Command g++).Source
    $targetArgs = @()
    if (Get-Command clang++ -ErrorAction SilentlyContinue) {
        $mingw = Split-Path (Split-Path $compiler)
        $compiler = (Get-Command clang++).Source
        $targetArgs = @('--target=x86_64-w64-windows-gnu', "--sysroot=$mingw", '-fuse-ld=lld')
    }
    & $compiler @targetArgs -std=c++20 -O2 -Wall -Wextra -Wno-missing-field-initializers -municode -mwindows -static -D_WIN32_WINNT=0x0A00 -DUNICODE -D_UNICODE -DNOMINMAX "-I$env:VULKAN_SDK/Include" -Ibuild src/main.cpp src/model.cpp src/canvas.cpp src/vulkan.cpp "$env:VULKAN_SDK/Lib/vulkan-1.lib" -pthread -lgdi32 -lcomdlg32 -lshell32 -o build/FieldAtlas.exe
    if ($LASTEXITCODE) { throw 'C++ build failed' }
    if ($Run) { Start-Process -FilePath (Resolve-Path build/FieldAtlas.exe) }
} finally { Pop-Location }
