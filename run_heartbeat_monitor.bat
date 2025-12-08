@echo off
cd /d "%~dp0"
python heartbeat_monitor.py %*
pause
