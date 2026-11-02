<#
.SYNOPSIS
Starts the local 7.72 test server (issue #18, notes/testserver.md) in the background.

.DESCRIPTION
Runs tibia-server.exe from its own folder (default C:\Tibia\gameserver, where config.cfg,
tibia.db and tibia.pem are found relative to the working directory) in a hidden console of
its own, writes its output to build\testserver\server.log (stderr: server.err) and its PID
to build\testserver\server.pid, and waits until it listens on the login port (7171) and the
game port (7172). Stop it with tools\stop-testserver.ps1, which sends Ctrl+C so the server
saves and logs everyone out (an inherited "ignore Ctrl+C" flag is cleared before the start, so
this works from Git Bash too).

Exit code 0 when the server is up, 1 when it is already running, failed to start, exited,
or did not listen within -TimeoutSeconds.

.EXAMPLE
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run-testserver.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run-testserver.ps1 -Window
#>
param(
	[string]$ServerDir = 'C:\Tibia\gameserver',
	[int]$TimeoutSeconds = 300,
	# show the server's console (minimised) instead of hiding it
	[switch]$Window
)

$ErrorActionPreference = 'Stop'
$StateDir = Join-Path (Split-Path -Parent $PSScriptRoot) 'build\testserver'
$PidFile = Join-Path $StateDir 'server.pid'
$Exe = Join-Path $ServerDir 'tibia-server.exe'

if (-not (Test-Path -LiteralPath $Exe)) {
	Write-Error "run-testserver: $Exe not found"
	exit 1
}
$Exe = (Resolve-Path -LiteralPath $Exe).Path
$running = Get-Process -Name 'tibia-server' -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $Exe }
if ($running) {
	Write-Host "run-testserver: already running (PID $($running.Id -join ', '))"
	exit 1
}
foreach ($port in 7171, 7172) {
	# a listening socket has remote port 0 (-State Listen is not used: this Windows build reports no State)
	if (Get-NetTCPConnection -LocalPort $port -ErrorAction SilentlyContinue | Where-Object { $_.RemotePort -eq 0 }) {
		Write-Host "run-testserver: port $port is already in use"
		exit 1
	}
}

New-Item -ItemType Directory -Force -Path $StateDir | Out-Null

# A process that ignores Ctrl+C passes that to its children (SetConsoleCtrlHandler(NULL, TRUE) is
# inherited); started from Git Bash, for one, the server would then ignore stop-testserver.ps1's Ctrl+C.
# Clear the flag in this process before starting the server.
Add-Type -Namespace TestServer -Name Console -MemberDefinition @'
[DllImport("kernel32.dll")] public static extern bool SetConsoleCtrlHandler(IntPtr handler, bool add);
'@
[TestServer.Console]::SetConsoleCtrlHandler([IntPtr]::Zero, $false) | Out-Null

$log = Join-Path $StateDir 'server.log'
$err = Join-Path $StateDir 'server.err'
$style = if ($Window) { 'Minimized' } else { 'Hidden' }
$proc = Start-Process -FilePath $Exe -WorkingDirectory $ServerDir -WindowStyle $style `
	-RedirectStandardOutput $log -RedirectStandardError $err -PassThru
Set-Content -LiteralPath $PidFile -Value $proc.Id -Encoding ascii
Write-Host "run-testserver: started PID $($proc.Id), log $log"

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $deadline) {
	if ($proc.HasExited) {
		Write-Host "run-testserver: the server exited with code $($proc.ExitCode); see $log and $err"
		Remove-Item -LiteralPath $PidFile -ErrorAction SilentlyContinue
		exit 1
	}
	$listening = @(Get-NetTCPConnection -OwningProcess $proc.Id -ErrorAction SilentlyContinue |
		Where-Object { $_.RemotePort -eq 0 -and $_.LocalPort -in 7171, 7172 } |
		ForEach-Object { "$($_.LocalAddress):$($_.LocalPort)" })
	if (($listening -match ':7171$') -and ($listening -match ':7172$')) {
		Write-Host "run-testserver: listening on $($listening -join ', ')"
		exit 0
	}
	Start-Sleep -Milliseconds 500
}
Write-Host "run-testserver: not listening on 7171 and 7172 after $TimeoutSeconds s (still running, PID $($proc.Id)); see $log"
exit 1
