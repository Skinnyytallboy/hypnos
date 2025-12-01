@echo off
setlocal enabledelayedexpansion

REM Switch to each local branch, pull, check for changes
for /f "tokens=*" %%b in ('git branch ^| findstr /v "^$"') do (
  echo Checking branch %%b...
  git checkout "%%b" >nul 2>&1
  git pull origin "%%b" 2>nul || git pull 2>nul
  git status --porcelain | findstr /r "^##.*behind\|^##.*ahead" >nul && (
    echo   ^> %%b has changes (ahead/behind remote)
  )
)

echo Done. Returning to previous branch...
git checkout -
