param(
    [switch]$WithAAX,
    [string]$IsccPath
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
$artifactRoot = Join-Path $projectRoot "build\AureaPiano_artefacts\Release"
$standalone = Join-Path $artifactRoot "Standalone\Aurea Piano.exe"
$vst3 = Join-Path $artifactRoot "VST3\Aurea Piano.vst3"
$signedAax = Join-Path $projectRoot "release\signed\Aurea Piano.aaxplugin"

if (-not (Test-Path -LiteralPath $standalone -PathType Leaf)) {
    throw "Falta el ejecutable standalone. Compila primero AureaPiano_Standalone: $standalone"
}
if (-not (Test-Path -LiteralPath $vst3 -PathType Container)) {
    throw "Falta el paquete VST3. Compila primero AureaPiano_VST3: $vst3"
}
if ($WithAAX -and -not (Test-Path -LiteralPath $signedAax -PathType Container)) {
    throw "Falta el paquete AAX firmado en: $signedAax. Un AAX sin firmar no funcionará en Pro Tools normal."
}

if (-not $IsccPath) {
    $found = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($found) { $IsccPath = $found.Source }
}
if (-not $IsccPath) {
    foreach ($candidate in @(
        (Join-Path $env:ProgramFiles "Inno Setup 7\ISCC.exe"),
        (Join-Path ${env:ProgramFiles(x86)} "Inno Setup 7\ISCC.exe"),
        (Join-Path ${env:ProgramFiles(x86)} "Inno Setup 6\ISCC.exe")
    )) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            $IsccPath = $candidate
            break
        }
    }
}
if (-not $IsccPath -or -not (Test-Path -LiteralPath $IsccPath -PathType Leaf)) {
    throw "Falta Inno Setup (ISCC.exe). Instálalo desde https://jrsoftware.org/inno-setup/downloads o pasa -IsccPath."
}

$script = Join-Path $projectRoot "Installer\AureaPiano.iss"
if ($WithAAX) {
    & $IsccPath "-dINCLUDE_AAX=1" $script
} else {
    & $IsccPath $script
}
if ($LASTEXITCODE -ne 0) { throw "Falló la creación del instalador." }

$name = if ($WithAAX) { "AureaPiano-0.1.0-win64-protools-setup.exe" } else { "AureaPiano-0.1.0-win64-setup.exe" }
$output = Join-Path $projectRoot "dist\$name"
if (-not (Test-Path -LiteralPath $output -PathType Leaf)) {
    throw "Inno Setup terminó sin crear el archivo esperado: $output"
}
Write-Host "Instalador creado: $output"
