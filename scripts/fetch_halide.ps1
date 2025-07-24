# PowerShell script to fetch and extract Halide for Windows

# Download Halide release for Windows
$url = "https://github.com/halide/Halide/releases/download/v19.0.0/Halide-19.0.0-x86-64-windows-5f17d6f8a35e7d374ef2e7e6b2d90061c0530333.zip"
$output = "halide.zip"
$tempDir = "thirdparty/halide_temp"
$destDir = "thirdparty/halide"

Write-Host "Downloading Halide for Windows..."
Invoke-WebRequest -Uri $url -OutFile $output

# Create directory structure
Write-Host "Creating directory structure..."
New-Item -ItemType Directory -Force -Path $tempDir
New-Item -ItemType Directory -Force -Path $destDir

# Extract the archive to temp directory
Write-Host "Extracting Halide to temp directory..."
Expand-Archive -Path $output -DestinationPath $tempDir -Force

# Move contents up one level
Write-Host "Moving files to $destDir..."
$extracted = Get-ChildItem -Path $tempDir | Where-Object { $_.PSIsContainer } | Select-Object -First 1
if ($extracted) {
    Move-Item -Path (Join-Path $extracted.FullName '*') -Destination $destDir -Force
    Remove-Item -Path $extracted.FullName -Recurse -Force
}

# Clean up
Write-Host "Cleaning up..."
Remove-Item $output
Remove-Item $tempDir -Recurse -Force

Write-Host "Halide installation complete!" 