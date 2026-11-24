param(
    [string]$Configuration = "Release",
    [switch]$WithAAX,
    [string]$Generator = "Visual Studio 18 2026"
)
$ErrorActionPreference = "Stop"
$aaxOption = if ($WithAAX) { "ON" } else { "OFF" }
cmake -S . -B build -G $Generator -A x64 "-DAUREA_BUILD_AAX=$aaxOption"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build --config $Configuration
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir build -C $Configuration --output-on-failure
exit $LASTEXITCODE
