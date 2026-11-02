<#
.SYNOPSIS
Stops the local 7.72 test server started by tools\run-testserver.ps1 (issue #18).

.DESCRIPTION
Sends Ctrl+C to the server's console (the shutdown it documents: it stops the login server,
logs all players out and saves, closes the database) and waits for it to exit. The PID comes
from build\testserver\server.pid, or else from the running tibia-server.exe in -ServerDir;
a process whose path is not -ServerDir\tibia-server.exe is never touched. The Ctrl+C is sent
by a separate PowerShell process that detaches from its own console, attaches to the
server's and ignores the event itself. With -Force, a server that has not exited after
-TimeoutSeconds is killed (no save).

Exit code 0 when no server is running afterwards, 1 otherwise.

.EXAMPLE
powershell -NoProfile -ExecutionPolicy Bypass -File tools\stop-testserver.ps1
#>
param(
	[string]$ServerDir = 'C:\Tibia\gameserver',
	[int]$TimeoutSeconds = 60,
	[switch]$Force
)

$ErrorActionPreference = 'Stop'
$StateDir = Join-Path (Split-Path -Parent $PSScriptRoot) 'build\testserver'
$PidFile = Join-Path $StateDir 'server.pid'
$Exe = [System.IO.Path]::GetFullPath((Join-Path $ServerDir 'tibia-server.exe'))

$proc = $null
if (Test-Path -LiteralPath $PidFile) {
	$id = [int](Get-Content -LiteralPath $PidFile -TotalCount 1)
	$proc = Get-Process -Id $id -ErrorAction SilentlyContinue
	if ($proc -and $proc.Path -ne $Exe) { $proc = $null }
}
if (-not $proc) {
	$proc = Get-Process -Name 'tibia-server' -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $Exe } |
		Select-Object -First 1
}
if (-not $proc) {
	Write-Host 'stop-testserver: no server running'
	Remove-Item -LiteralPath $PidFile -ErrorAction SilentlyContinue
	exit 0
}

$helper = @'
using System;
using System.Runtime.InteropServices;
public static class CtrlC {
	[DllImport("kernel32.dll")] static extern bool FreeConsole();
	[DllImport("kernel32.dll")] static extern bool AttachConsole(uint pid);
	[DllImport("kernel32.dll")] static extern bool SetConsoleCtrlHandler(IntPtr handler, bool add);
	[DllImport("kernel32.dll")] static extern bool GenerateConsoleCtrlEvent(uint ev, uint group);
	public static int Send(uint pid) {
		FreeConsole();
		if (!AttachConsole(pid)) return Marshal.GetLastWin32Error();
		SetConsoleCtrlHandler(IntPtr.Zero, true);
		bool ok = GenerateConsoleCtrlEvent(0, 0);
		FreeConsole();
		return ok ? 0 : -1;
	}
}
'@
$script = "Add-Type -TypeDefinition @'`n$helper`n'@`nexit [CtrlC]::Send($($proc.Id))"
$encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($script))
$shell = (Get-Process -Id $PID).Path
$sender = Start-Process -FilePath $shell -ArgumentList '-NoProfile', '-NonInteractive', '-EncodedCommand', $encoded `
	-WindowStyle Hidden -PassThru -Wait
Write-Host "stop-testserver: sent Ctrl+C to PID $($proc.Id) (sender exit code $($sender.ExitCode))"

if ($proc.WaitForExit($TimeoutSeconds * 1000)) {
	Write-Host "stop-testserver: the server exited"
	Remove-Item -LiteralPath $PidFile -ErrorAction SilentlyContinue
	exit 0
}
if ($Force) {
	Stop-Process -Id $proc.Id -Force
	Write-Host "stop-testserver: killed PID $($proc.Id) after $TimeoutSeconds s"
	Remove-Item -LiteralPath $PidFile -ErrorAction SilentlyContinue
	exit 0
}
Write-Host "stop-testserver: PID $($proc.Id) still running after $TimeoutSeconds s (use -Force to kill it)"
exit 1
