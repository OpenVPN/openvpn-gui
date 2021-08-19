# Build openvpn-gui with MSVC

$CWD = Get-Location

$CMAKE="C:\Program Files\CMake\bin\cmake.exe"
$CMAKE_TOOLCHAIN_FILE="C:\users\vagrant\buildbot\windows-server-2019-static-msbuild\vcpkg\scripts\buildsystems\vcpkg.cmake"

# Enable running this script from anywhere
cd $PSScriptRoot

# Architecture names taken from here:
#
# https://docs.microsoft.com/en-us/cpp/build/cmake-presets-vs?view=msvc-160
"x64", "arm64", "Win32" | ForEach  {
	$platform = $_
    Write-Host "Building openvpn-gui ${platform}"
    if ( ( Test-Path "build_${platform}" ) -eq $False )
	{
        mkdir build_$platform
	}
    cd build_$platform
    & "$CMAKE" -A $platform -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" ..
    & "$CMAKE" --build . --config Release
    cd $PSScriptRoot
}

cd $CWD