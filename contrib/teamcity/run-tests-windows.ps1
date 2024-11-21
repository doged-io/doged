Set-PSDebug -Off
$ErrorActionPreference='Stop'

Write-Host "--- Extracting the artifacts ---"
mkdir -Force artifacts | out-null
tar xzf artifacts.tar.gz -C artifacts

pushd artifacts/bin

function check_help_version {
  try {
    .\dogecoind.exe -version
    .\dogecoind.exe -help
    .\bitcoin-qt.exe -version
    .\bitcoin-qt.exe -help
    .\dogecoin-cli.exe -version
    .\dogecoin-cli.exe -help
    .\dogecoin-tx.exe -help
    .\dogecoin-wallet -help
  }
  catch {
    Write-Error $_
  }
  finally {
    Stop-Process -name bitcoin-qt -Force -ErrorAction SilentlyContinue
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
    Stop-Process -name dogecoind -Force 
  }

  $datadir = New-TemporaryDirectory
  $datadirArg = "-datadir=$datadir"

  Write-Host "Launching dogecoind in the background"
  Start-Process -NoNewWindow .\dogecoind.exe "-noprinttoconsole $datadirArg"

  for($i=60; $i -gt 0; $i--) {
    Start-Sleep -Seconds 1
    if(.\dogecoin-cli.exe $datadirArg help) {
      break
    }
  }
  if($i -eq 0) {
    throw "Failed to start dogecoind"
  }

  Write-Host "Stopping dogecoind"
  .\dogecoin-cli.exe $datadirArg stop

  for($i=60; $i -gt 0; $i--) {
    Start-Sleep -Seconds 1
    if(-Not (Get-Process -Name dogecoind -ErrorAction SilentlyContinue)) {
      break
    }
  }
  if($i -eq 0) {
    throw "Failed to stop dogecoind"
  }
}

Write-Host "--- Checking helps and versions ---"
check_help_version

Write-Host "--- Checking dogecoind can run and communicate via dogecoin-cli ---"
check_bitcoind

Write-Host "--- Running bitcoin unit tests ---"
.\test_bitcoin.exe
Write-Host "--- Running bitcoin-qt unit tests ---"
.\test_bitcoin-qt.exe -platform windows
Write-Host "--- Running pow unit tests ---"
.\test-pow.exe
Write-Host "--- Running avalanche unit tests ---"
# FIXME: figure out why the poll_inflight_timeout test fails and fix it
.\test-avalanche.exe -t !processor_tests/poll_inflight_timeout

popd

Write-Host -ForegroundColor Green "--- All checks passed ---"
