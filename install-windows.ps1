param(
    [switch]$SkipBuild,
    [switch]$Launch,
    [string]$Generator = "Visual Studio 18 2026"
)

# Herramienta para desarrolladores. No es el instalador final para usuarios.
# El AAX de Pro Tools requiere una firma externa antes de su distribución.
$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
$artifacts = Join-Path $projectRoot "build\AureaPiano_artefacts\Release"
$vstSource = Join-Path $artifacts "VST3\Aurea Piano.vst3"
$appSource = Join-Path $artifacts "Standalone\Aurea Piano.exe"

if (-not $SkipBuild) {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "Falta CMake. Para Visual Studio 2026 instala CMake 4.2 o superior y Visual Studio Community 2026 con Desktop development with C++."
    }
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "Falta Git. Se necesita para descargar JUCE en la primera compilación."
    }
    & (Join-Path $projectRoot "build-windows.ps1") -Configuration Release -Generator $Generator
    if ($LASTEXITCODE -ne 0) { throw "La compilación o la prueba del motor falló." }
}

if (-not (Test-Path -LiteralPath $vstSource)) {
    throw "No encuentro el VST3 compilado: $vstSource"
}
if (-not (Test-Path -LiteralPath $appSource -PathType Leaf)) {
    throw "No encuentro la aplicación compilada: $appSource"
}

# Ruta VST3 de usuario definida por Steinberg: no requiere privilegios de admin.
$vstFolder = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3"
$vstDestination = Join-Path $vstFolder "Aurea Piano.vst3"
$appFolder = Join-Path $env:LOCALAPPDATA "Programs\Aurea Piano"
New-Item -ItemType Directory -Path $vstFolder, $appFolder -Force | Out-Null

if (Test-Path -LiteralPath $vstSource -PathType Container) {
    New-Item -ItemType Directory -Path $vstDestination -Force | Out-Null
    Get-ChildItem -LiteralPath $vstSource -Force |
        Copy-Item -Destination $vstDestination -Recurse -Force
} else {
    Copy-Item -LiteralPath $vstSource -Destination $vstDestination -Force
}
Copy-Item -LiteralPath $appSource -Destination (Join-Path $appFolder "Aurea Piano.exe") -Force

Write-Host "VST3 instalado en: $vstDestination"
Write-Host "Aplicación instalada en: $(Join-Path $appFolder 'Aurea Piano.exe')"
Write-Host "Abre de nuevo tu DAW y vuelve a analizar los plugins VST3 si no aparece."
Write-Host "Pro Tools 12 requiere AAX: esta instalación VST3 no aparecerá allí."
if ($Launch) {
    Start-Process -FilePath (Join-Path $appFolder "Aurea Piano.exe")
}
