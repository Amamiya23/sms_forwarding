<#
  .SYNOPSIS
  一键编译 + 烧录 (+ 串口监控) 便捷脚本。

  用法:
    .\tools\go.ps1                       自动探测串口，编译 + 烧录 + 监控
    .\tools\go.ps1 -Port COM7            指定串口
    .\tools\go.ps1 -NoMonitor            只编译 + 烧录，不进监控
    .\tools\go.ps1 -BuildDir build\idf-local
#>
[CmdletBinding()]
param(
    [string]$Port = '',
    [string]$BuildDir = '',
    [string]$IdfPath = $env:IDF_PATH,
    [string]$IdfToolsPath = $env:IDF_TOOLS_PATH,
    [string]$IdfPythonEnvPath = $env:IDF_PYTHON_ENV_PATH,
    [int]$Jobs = 0,
    [switch]$NoMonitor
)

$ErrorActionPreference = 'Stop'
$IdfScript = Join-Path $PSScriptRoot 'idf.ps1'
if (-not (Test-Path -LiteralPath $IdfScript)) {
    throw "找不到依赖脚本: $IdfScript"
}

function Get-EspSerialPort {
    $seen = @{}
    try {
        foreach ($d in @(Get-PnpDevice -Class Ports -Present -ErrorAction SilentlyContinue)) {
            if ($d.FriendlyName -match 'USB|CH340|CH341|CP210|JTAG|ESP|UART|Bridge' -and $d.FriendlyName -match '\((COM\d+)\)') {
                $com = $Matches[1]
                if (-not $seen.ContainsKey($com)) {
                    $seen[$com] = $true
                    [pscustomobject]@{ DeviceID = $com; Description = $d.FriendlyName }
                }
            }
        }
    } catch {}
    if ($seen.Count -eq 0) {
        try {
            $cim = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
                Where-Object { $_.DeviceID -and $_.Description -match 'USB|CH340|CH341|CP210|JTAG|ESP|UART|Bridge' }
            foreach ($c in $cim) {
                $com = $c.DeviceID
                if (-not $seen.ContainsKey($com)) {
                    $seen[$com] = $true
                    [pscustomobject]@{ DeviceID = $com; Description = $c.Description }
                }
            }
        } catch {}
    }
}

if ([string]::IsNullOrWhiteSpace($Port)) {
    $candidates = @(Get-EspSerialPort)
    if ($candidates.Count -eq 1) {
        $Port = $candidates[0].DeviceID
        Write-Host ("[go] 自动探测到串口: {0} ({1})" -f $Port, $candidates[0].Description) -ForegroundColor Cyan
    } elseif ($candidates.Count -gt 1) {
        Write-Host "[go] 发现多个串口，请用 -Port 指定:" -ForegroundColor Yellow
        foreach ($c in $candidates) {
            Write-Host ("  {0}  -  {1}" -f $c.DeviceID, $c.Description)
        }
        exit 1
    } else {
        Write-Host "[go] 未自动探测到串口，请用 -Port 指定，例如: .\tools\go.ps1 -Port COM5" -ForegroundColor Yellow
        Write-Host "[go] 查看可用串口: Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Description"
        exit 1
    }
}

$Action = if ($NoMonitor) { 'buildflash' } else { 'all' }

Write-Host ("[go] 执行: tools\idf.ps1 -Action {0} -Port {1}" -f $Action, $Port) -ForegroundColor Green
& $IdfScript -Action $Action -Port $Port -BuildDir $BuildDir -IdfPath $IdfPath `
    -IdfToolsPath $IdfToolsPath -IdfPythonEnvPath $IdfPythonEnvPath -Jobs $Jobs
