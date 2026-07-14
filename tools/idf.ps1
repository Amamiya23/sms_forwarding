[CmdletBinding()]
param(
    [ValidateSet('build', 'buildflash', 'all', 'flash', 'monitor', 'reconfigure', 'clean', 'fullclean')]
    [string]$Action = 'build',
    [string]$Port = 'COM5',
    [string]$IdfPath = $env:IDF_PATH,
    [string]$IdfToolsPath = $env:IDF_TOOLS_PATH,
    [string]$IdfPythonEnvPath = $env:IDF_PYTHON_ENV_PATH,
    [string]$BuildDir = '',
    [int]$Jobs = 0
)

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot 'build\idf'
} elseif (-not [System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot $BuildDir
}
$SdkConfig = Join-Path $RepoRoot 'build\sdkconfig'

$eimConfigCandidates = @('C:\Espressif\tools\eim_idf.json')
if (-not [string]::IsNullOrWhiteSpace($IdfToolsPath)) {
    $eimConfigCandidates = @((Join-Path $IdfToolsPath 'eim_idf.json')) + $eimConfigCandidates
}

$eimInstall = $null
foreach ($configPath in $eimConfigCandidates | Select-Object -Unique) {
    if (-not (Test-Path -LiteralPath $configPath)) { continue }
    try {
        $eimConfig = Get-Content -Raw -LiteralPath $configPath | ConvertFrom-Json
        if (-not [string]::IsNullOrWhiteSpace($IdfPath)) {
            $eimInstall = $eimConfig.idfInstalled | Where-Object { $_.path -eq $IdfPath } | Select-Object -First 1
        }
        if (-not $eimInstall) {
            $eimInstall = $eimConfig.idfInstalled | Where-Object { $_.id -eq $eimConfig.idfSelectedId } | Select-Object -First 1
        }
        if ($eimInstall) { break }
    } catch {
        Write-Warning "Ignoring invalid EIM metadata ${configPath}: $($_.Exception.Message)"
    }
}

if ($eimInstall) {
    if ([string]::IsNullOrWhiteSpace($IdfPath)) { $IdfPath = $eimInstall.path }
    if ([string]::IsNullOrWhiteSpace($IdfToolsPath)) { $IdfToolsPath = $eimInstall.idfToolsPath }
    if ([string]::IsNullOrWhiteSpace($IdfPythonEnvPath) -and $eimInstall.python) {
        $IdfPythonEnvPath = Split-Path -Parent (Split-Path -Parent $eimInstall.python)
    }
}

if ([string]::IsNullOrWhiteSpace($IdfPath)) {
    $IdfPath = 'E:\Espressif\esp-idf-v6.0.2'
}
if ([string]::IsNullOrWhiteSpace($IdfToolsPath)) {
    $IdfToolsPath = 'E:\Espressif\.espressif'
}

$ExportScript = Join-Path $IdfPath 'export.ps1'
if (-not (Test-Path -LiteralPath $ExportScript)) {
    throw "ESP-IDF export script not found: $ExportScript"
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$useEimActivation = $eimInstall -and
    (Test-Path -LiteralPath $eimInstall.activationScript) -and
    ($eimInstall.path -eq $IdfPath)
if ($useEimActivation) {
    . $eimInstall.activationScript
} else {
    $env:IDF_TOOLS_PATH = $IdfToolsPath
    if (-not [string]::IsNullOrWhiteSpace($IdfPythonEnvPath)) {
        $env:IDF_PYTHON_ENV_PATH = $IdfPythonEnvPath
    }
    . $ExportScript
}

$IdfArgs = @('-B', $BuildDir, '-D', "SDKCONFIG=$SdkConfig")

if ($Jobs -le 0) {
    $Jobs = [int]$env:NUMBER_OF_PROCESSORS
    if ($Jobs -le 0) { $Jobs = 4 }
}

function Reset-StaleBuildDirectory {
    $cachePath = Join-Path $BuildDir 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath)) { return }

    $generatorLine = Get-Content -LiteralPath $cachePath |
        Where-Object { $_ -like 'CMAKE_GENERATOR:INTERNAL=*' } |
        Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($generatorLine)) { return }

    $generator = $generatorLine.Substring('CMAKE_GENERATOR:INTERNAL='.Length)
    if ($generator -eq 'Ninja') { return }

    Write-Warning "Build directory uses CMake generator '$generator'; removing stale build artifacts."
    Remove-Item -LiteralPath $BuildDir -Recurse -Force
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
}

function Invoke-Idf {
    param([Parameter(Mandatory = $true)][string[]]$CommandArgs)

    idf.py @CommandArgs
    if ($LASTEXITCODE -ne 0) {
        throw "idf.py $($CommandArgs -join ' ') failed with exit code $LASTEXITCODE."
    }
}

function Invoke-Ninja {
    ninja -C $BuildDir -j $Jobs
    if ($LASTEXITCODE -ne 0) {
        throw "ninja failed with exit code $LASTEXITCODE."
    }
}

if ($Action -in @('build', 'buildflash', 'all', 'reconfigure')) {
    Reset-StaleBuildDirectory
}

switch ($Action) {
    'build' {
        Invoke-Idf ($IdfArgs + @('reconfigure'))
        Invoke-Ninja
    }
    'buildflash' {
        Invoke-Idf ($IdfArgs + @('reconfigure'))
        Invoke-Ninja
        Invoke-Idf ($IdfArgs + @('-p', $Port, 'flash'))
    }
    'all' {
        Invoke-Idf ($IdfArgs + @('reconfigure'))
        Invoke-Ninja
        Invoke-Idf ($IdfArgs + @('-p', $Port, 'flash', 'monitor'))
    }
    'flash' {
        Invoke-Idf ($IdfArgs + @('-p', $Port, 'flash'))
    }
    'monitor' {
        Invoke-Idf ($IdfArgs + @('-p', $Port, 'monitor'))
    }
    'reconfigure' {
        Invoke-Idf ($IdfArgs + @('reconfigure'))
    }
    'clean' {
        Invoke-Idf ($IdfArgs + @('clean'))
    }
    'fullclean' {
        Invoke-Idf ($IdfArgs + @('fullclean'))
    }
}
