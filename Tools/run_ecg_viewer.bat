@echo off
setlocal

if "%~1"=="" (
    python "%~dp0ecg_serial_viewer.py"
) else (
    python "%~dp0ecg_serial_viewer.py" %*
)

endlocal
