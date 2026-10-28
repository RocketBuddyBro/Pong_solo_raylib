
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

# Confirm the Pong source exists.
$Source = Join-Path $Root "src\main.c"

if (!(Test-Path $Source)) {
    throw "Missing src\main.c. Put the Pong source in that location."
}

# Reuse the raylib installer toolchain when available.
$Toolchain = "C:\raylib\w64devkit\bin"

if (Test-Path (Join-Path $Toolchain "gcc.exe")) {
    $env:Path = "$Toolchain;$env:Path"
}

$Gcc = Get-Command gcc.exe -ErrorAction SilentlyContinue
$Make = Get-Command make.exe -ErrorAction SilentlyContinue

if (!$Make) {
    $Make = Get-Command mingw32-make.exe -ErrorAction SilentlyContinue
}

if (!$Gcc -or !$Make) {
    throw @"
A MinGW-W64 compiler and make are required.
Install raylib with its Windows toolchain, or install W64DevKit.
The script automatically detects C:\raylib\w64devkit\bin.
"@
}

# Bootstrap the official raylib-quickstart files on first run.
$BuildDir = Join-Path $Root "build"
$Premake = Join-Path $BuildDir "premake5.exe"

if (!(Test-Path $Premake)) {
    if (Test-Path $BuildDir) {
        throw "An incomplete build folder exists. Inspect or remove it, then rerun this script."
    }

    Write-Host "Downloading raylib-quickstart build setup..." -ForegroundColor Cyan

    $Temp = Join-Path $env:TEMP (
        "pong-quickstart-" + [guid]::NewGuid().ToString("N")
    )

    New-Item -ItemType Directory -Path $Temp | Out-Null

    try {
        $Zip = Join-Path $Temp "quickstart.zip"

        Invoke-WebRequest `
            -Uri "https://github.com/raylib-extras/raylib-quickstart/archive/refs/heads/main.zip" `
            -OutFile $Zip `
            -UseBasicParsing

        Expand-Archive -LiteralPath $Zip -DestinationPath $Temp -Force

        $Template = Join-Path $Temp "raylib-quickstart-main"

        if (!(Test-Path $Template)) {
            throw "Could not locate the extracted quickstart template."
        }

        $Items = @(
            ".vscode",
            "build",
            "include",
            "resources",
            ".clangd",
            "build-MinGW-W64.bat"
        )

        foreach ($Item in $Items) {
            $From = Join-Path $Template $Item
            $To = Join-Path $Root $Item

            if ((Test-Path $From) -and !(Test-Path $To)) {
                Copy-Item -LiteralPath $From -Destination $Root -Recurse -Force
            }
        }
    }
    finally {
        Remove-Item $Temp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# Pin the downloaded raylib dependency to release 6.2.
$LuaPath = Join-Path $BuildDir "premake5.lua"

if (!(Test-Path $LuaPath)) {
    throw "Missing build\premake5.lua."
}

$Lua = Get-Content $LuaPath -Raw

$MasterUrl = "https://github.com/raysan5/raylib/archive/refs/heads/master.zip"
$ReleaseUrl = "https://github.com/raysan5/raylib/archive/refs/tags/6.2.zip"

if ($Lua.Contains($MasterUrl)) {
    $Lua = $Lua.Replace("raylib-master", "raylib-6.2")
    $Lua = $Lua.Replace($MasterUrl, $ReleaseUrl)
}
elseif (
    !$Lua.Contains($ReleaseUrl) -or
    !$Lua.Contains("raylib-6.2")
) {
    throw "The quickstart download configuration changed. Check build\premake5.lua."
}

[System.IO.File]::WriteAllText(
    $LuaPath,
    $Lua,
    [System.Text.UTF8Encoding]::new($false)
)

# Ignore downloaded build configuration, dependencies and executables.
$IgnorePath = Join-Path $Root ".gitignore"
$Marker = "# Local raylib setup downloaded by run.ps1"

$Rules = @"
$Marker
/.vscode/
/.clangd
/build/
/include/
/resources/
/build-MinGW-W64.bat
/bin/
/Makefile
/*.make
"@

if (!(Test-Path $IgnorePath)) {
    [System.IO.File]::WriteAllText(
        $IgnorePath,
        $Rules + "`r`n",
        [System.Text.UTF8Encoding]::new($false)
    )
}
else {
    $Ignore = Get-Content $IgnorePath -Raw

    if (!$Ignore.Contains($Marker)) {
        [System.IO.File]::AppendAllText(
            $IgnorePath,
            "`r`n" + $Rules + "`r`n",
            [System.Text.UTF8Encoding]::new($false)
        )
    }
}

# Generate the platform Makefile.
Write-Host "Configuring the Pong project..." -ForegroundColor Cyan

Push-Location $BuildDir
try {
    & $Premake gmake

    if ($LASTEXITCODE -ne 0) {
        throw "Premake configuration failed."
    }
}
finally {
    Pop-Location
}

# Build the actual game.
Write-Host "Building Pong..." -ForegroundColor Cyan

& $Make.Source

if ($LASTEXITCODE -ne 0) {
    throw "Compilation failed. Review the compiler output above."
}

# Launch the executable matching the repository folder name.
$ProjectName = Split-Path $Root -Leaf
$Exe = Join-Path $Root "bin\Debug\$ProjectName.exe"

if (!(Test-Path $Exe)) {
    $Found = Get-ChildItem (Join-Path $Root "bin") `
        -Filter "$ProjectName.exe" -Recurse -File `
        -ErrorAction SilentlyContinue |
        Select-Object -First 1

    if ($Found) {
        $Exe = $Found.FullName
    }
}

if (!(Test-Path $Exe)) {
    throw "Build finished, but the expected game executable was not found."
}

Write-Host "Launching $ProjectName..." -ForegroundColor Green
Start-Process -FilePath $Exe -WorkingDirectory $Root
