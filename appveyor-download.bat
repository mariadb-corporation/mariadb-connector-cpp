@echo off
set archive=https://archive.mariadb.org//mariadb-%DB%/winx64-packages/mariadb-%DB%-winx64.msi
set last=https://archive.mariadb.org//mariadb-%DB%/winx64-packages/mariadb-%DB%-winx64.msi

echo Downloading from %archive%
curl -fLsS -o server.msi %archive%

if %ERRORLEVEL% == 0 goto end

echo Downloading from %last%
curl -fLsS -o server.msi %last%
if %ERRORLEVEL% == 0  goto end

echo Failure Reason Given is %errorlevel%
exit /b %errorlevel%

:end
echo "File found".
