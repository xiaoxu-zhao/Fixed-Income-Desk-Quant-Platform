param(
    [string]$Python = "python"
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $Root

Remove-Item -Force -ErrorAction SilentlyContinue output/*.csv, output/*.md, output/dashboard.html
Remove-Item -Force -ErrorAction SilentlyContinue output/plots/*.png
New-Item -ItemType Directory -Force output/plots, data/market_events | Out-Null

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --build-config Release --output-on-failure

function Find-Executable {
    param([string]$Name)
    $candidates = @(
        "build/$Name.exe",
        "build/$Name",
        "build/Release/$Name.exe",
        "build/Release/$Name",
        "build/Debug/$Name.exe",
        "build/Debug/$Name"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }
    throw "Could not find executable: $Name"
}

& (Find-Executable "run_daily_desk_report") .
& (Find-Executable "run_intraday_simulation") .
& (Find-Executable "run_full_desk_day") .

$env:PYTHONHASHSEED = "0"
& $Python python/plot_curves.py .
& $Python python/plot_risk.py .
& $Python python/plot_pnl.py .
& $Python python/plot_execution.py .
& $Python python/generate_desk_summary.py .
& $Python python/dashboard.py --static

Write-Host "Full demo complete. Outputs are in $Root/output"

