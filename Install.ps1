$ErrorActionPreference = 'Stop'

$bundle = Join-Path $PSScriptRoot 'Side Fregna.vst3'
if (-not (Test-Path -LiteralPath $bundle)) {
    throw "Plugin bundle not found: $bundle. Extract the complete release ZIP before running this installer."
}

$vst3Folder = Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'
New-Item -ItemType Directory -Path $vst3Folder -Force | Out-Null
Copy-Item -LiteralPath $bundle -Destination $vst3Folder -Recurse -Force
Write-Host "Installed Side Fregna to $(Join-Path $vst3Folder 'Side Fregna.vst3')"
Write-Host 'Rescan VST3 plug-ins in your DAW.'
