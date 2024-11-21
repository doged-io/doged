Set-PSDebug -Off
$ErrorActionPreference='Stop'

Write-Host "--- Extracting the artifacts ---"
mkdir -Force artifacts | out-null
tar xzf artifacts.tar.gz -C artifacts

pushd artifacts/bin

function check_help_version {
  try {
    .\dogecashd.exe -version
    .\dogecashd.exe -help
    .\bitcoin-qt.exe -version
    .\bitcoin-qt.exe -help
    .\dogecash-cli.exe -version
    .\dogecash-cli.exe -help
    .\bitcoin-tx.exe -help
    .\bitcoin-wallet -help
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

function check_dogecashd {
  trap {
    Stop-Process -name dogecashd -Force 
  }

  $datadir = New-TemporaryDirectory
  $datadirArg = "-datadir=$datadir"

  Write-Host "Launching dogecashd in the background"
  Start-Process -NoNewWindow .\dogecashd.exe "-noprinttoconsole $datadirArg"

  for($i=60; $i -gt 0; $i--) {
    Start-Sleep -Seconds 1
    if(.\dogecash-cli.exe $datadirArg help) {
      break
    }
  }
  if($i -eq 0) {
    throw "Failed to start dogecashd"
  }

  Write-Host "Stopping dogecashd"
  .\dogecash-cli.exe $datadirArg stop

  for($i=60; $i -gt 0; $i--) {
    Start-Sleep -Seconds 1
    if(-Not (Get-Process -Name dogecashd -ErrorAction SilentlyContinue)) {
      break
    }
  }
  if($i -eq 0) {
    throw "Failed to stop dogecashd"
  }
}

Write-Host "--- Checking helps and versions ---"
check_help_version

Write-Host "--- Checking dogecashd can run and communicate via dogecash-cli ---"
check_dogecashd

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
