Set-PSDebug -Off
$ErrorActionPreference='Stop'

Write-Host "--- Extracting the artifacts ---"
mkdir -Force artifacts | out-null
tar xzf artifacts.tar.gz -C artifacts

pushd artifacts/bin

function check_help_version {
  try {
    .\doged.exe -version
    .\doged.exe -help
    .\doge-qt.exe -version
    .\doge-qt.exe -help
    .\doge-cli.exe -version
    .\doge-cli.exe -help
    .\doge-tx.exe -help
    .\doge-wallet -help
  }
  catch {
    Write-Error $_
  }
  finally {
    Stop-Process -name doge-qt -Force -ErrorAction SilentlyContinue
  }
}

function New-TemporaryDirectory {
  $parent = [System.IO.Path]::GetTempPath()
  [string] $name = [System.Guid]::NewGuid()
  $tempDir = New-Item -ItemType Directory -Path (Join-Path $parent $name)
  return $tempDir.FullName
}

function check_bitcoind {
  trap {
    Stop-Process -name doged -Force 
  }

  $datadir = New-TemporaryDirectory
  $datadirArg = "-datadir=$datadir"

  Write-Host "Launching doged in the background"
  Start-Process -NoNewWindow .\doged.exe "-noprinttoconsole $datadirArg"

  for($i=60; $i -gt 0; $i--) {
    Start-Sleep -Seconds 1
    if(.\doge-cli.exe $datadirArg help) {
      break
    }
  }
  if($i -eq 0) {
    throw "Failed to start doged"
  }

  Write-Host "Stopping doged"
  .\doge-cli.exe $datadirArg stop

  for($i=60; $i -gt 0; $i--) {
    Start-Sleep -Seconds 1
    if(-Not (Get-Process -Name doged -ErrorAction SilentlyContinue)) {
      break
    }
  }
  if($i -eq 0) {
    throw "Failed to stop doged"
  }
}

Write-Host "--- Checking helps and versions ---"
check_help_version

Write-Host "--- Checking doged can run and communicate via doge-cli ---"
check_bitcoind

Write-Host "--- Running bitcoin unit tests ---"
.\test_bitcoin.exe
Write-Host "--- Running doge-qt unit tests ---"
.\test_doge-qt.exe -platform windows
Write-Host "--- Running pow unit tests ---"
.\test-pow.exe
Write-Host "--- Running avalanche unit tests ---"
# FIXME: figure out why the poll_inflight_timeout test fails and fix it
.\test-avalanche.exe -t !processor_tests/poll_inflight_timeout

popd

Write-Host -ForegroundColor Green "--- All checks passed ---"
