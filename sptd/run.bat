@echo off
rem Launch the recompiled game. Extra args are passed through, e.g.:
rem   run.bat --sptd_launcher=false  (skip the launcher)
rem   run.bat --log_level=debug
cd /d "%~dp0out\build\sptd-relwithdebinfo"
start "" south_park_td.exe --game_data_root="%~dp0assets" --log_file=run.log %*
