param(
    [ValidateSet('release', 'debug', 'release-vr')]
    [string]$Preset = 'release',

    [switch]$RunTests,

    [switch]$NoPause
)

$ErrorActionPreference = 'Stop'

$ProjectRoot = if ($PSScriptRoot) {
    $PSScriptRoot
} else {
    (Get-Location).Path
}

$BuildDir = Join-Path $ProjectRoot "build\$Preset"
$StageRoot = Join-Path $ProjectRoot 'dist'
$OutputRoot = Join-Path $ProjectRoot 'Output'
$PluginOutput = Join-Path $OutputRoot 'Data\SKSE\Plugins'
$ExitCode = 0

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Arguments
    )

    Write-Host ''
    Write-Host "> $FilePath $($Arguments -join ' ')"
    & $FilePath @Arguments

    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE."
    }
}

function Assert-PathInsideProject {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $ProjectFullPath = [System.IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
    $TargetFullPath = [System.IO.Path]::GetFullPath($Path)

    if (-not $TargetFullPath.StartsWith($ProjectFullPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to operate outside the project directory: $TargetFullPath"
    }
}

try {
    Set-Location -LiteralPath $ProjectRoot
    Assert-PathInsideProject -Path $BuildDir
    Assert-PathInsideProject -Path $StageRoot
    Assert-PathInsideProject -Path $OutputRoot
    Assert-PathInsideProject -Path $PluginOutput

    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw 'CMake was not found in PATH.'
    }

    if (-not $env:VCPKG_ROOT) {
        $VcpkgCandidates = @(
            'C:\vcpkg',
            (Join-Path $ProjectRoot 'vcpkg')
        )

        foreach ($Candidate in $VcpkgCandidates) {
            $ToolchainPath = Join-Path $Candidate 'scripts\buildsystems\vcpkg.cmake'
            if (Test-Path -LiteralPath $ToolchainPath) {
                $env:VCPKG_ROOT = $Candidate
                Write-Host "Using VCPKG_ROOT: $env:VCPKG_ROOT"
                break
            }
        }
    }

    $CachePath = Join-Path $BuildDir 'CMakeCache.txt'

    if (-not $env:VCPKG_ROOT) {
        Write-Warning 'VCPKG_ROOT is not set. Configure may fail unless vcpkg is available another way.'
    }

    if (Test-Path -LiteralPath $CachePath) {
        Invoke-NativeCommand cmake --fresh --preset $Preset
    } else {
        Invoke-NativeCommand cmake --preset $Preset
    }

    Invoke-NativeCommand cmake --build --preset $Preset --target stage_mod

    if ($RunTests) {
        Invoke-NativeCommand cmake --build --preset $Preset --target DialogueHUDFadeTests
        Invoke-NativeCommand ctest --preset $Preset
    }

    if (-not (Test-Path -LiteralPath $StageRoot)) {
        throw "Stage directory was not created: $StageRoot"
    }

    if (Test-Path -LiteralPath $OutputRoot) {
        Remove-Item -LiteralPath $OutputRoot -Recurse -Force
    }

    New-Item -ItemType Directory -Path $PluginOutput -Force | Out-Null
    Get-ChildItem -LiteralPath $StageRoot | Copy-Item -Destination $OutputRoot -Recurse -Force

    $PluginDll = Join-Path $PluginOutput 'DialogueHUDFade.dll'
    $PluginIni = Join-Path $PluginOutput 'DialogueHUDFade.ini'

    if (-not (Test-Path -LiteralPath $PluginDll)) {
        throw "Plugin DLL was not copied to: $PluginDll"
    }

    if (-not (Test-Path -LiteralPath $PluginIni)) {
        throw "Plugin INI was not copied to: $PluginIni"
    }

    Write-Host ''
    Write-Host 'Build finished successfully.'
    Write-Host "Output package: $OutputRoot"
    Write-Host 'Skyrim plugin path: Output\Data\SKSE\Plugins'
} catch {
    $ExitCode = 1
    Write-Host ''
    Write-Host 'Build failed.'
    Write-Host $_.Exception.Message
} finally {
    if (-not $NoPause) {
        Write-Host ''
        Read-Host 'Press Enter to close this window'
    }

    exit $ExitCode
}
