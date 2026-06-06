@echo off
REM ============================================
REM Deploy Accessd Service to Pi4 (Windows)
REM ============================================

set PI_USER=nobuta
set PI_HOST=Cv.local
set PI_DIR=/home/nobuta/accessd
set PROJECT_ROOT=C:\Users\CAS\Work-Project\ACS_DB

echo === Deploying Accessd Service to Pi4 ===

REM Create directory structure
echo Creating directory structure...
ssh %PI_USER%@%PI_HOST% "mkdir -p %PI_DIR%/src/core %PI_DIR%/src/db %PI_DIR%/src/access %PI_DIR%/src/event %PI_DIR%/src/reader %PI_DIR%/src/api"
ssh %PI_USER%@%PI_HOST% "mkdir -p %PI_DIR%/include %PI_DIR%/config %PI_DIR%/scripts %PI_DIR%/test %PI_DIR%/docs %PI_DIR%/build"

REM Copy source files
echo.
echo Copying source files...
scp %PROJECT_ROOT%\src\core\*.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/core/
scp %PROJECT_ROOT%\include\*.hpp %PI_USER%@%PI_HOST%:%PI_DIR%/include/

REM Copy config
echo Copying config...
scp %PROJECT_ROOT%\config\accessd.conf %PI_USER%@%PI_HOST%:%PI_DIR%/config/

REM Copy CMakeLists
echo Copying CMakeLists.txt...
scp %PROJECT_ROOT%\CMakeLists.txt %PI_USER%@%PI_HOST%:%PI_DIR%/

REM Copy database
echo Copying database...
scp %PROJECT_ROOT%\database\schema\*.sql %PI_USER%@%PI_HOST%:%PI_DIR%/database/schema/
scp %PROJECT_ROOT%\database\init_db.sh %PI_USER%@%PI_HOST%:%PI_DIR%/database/
scp %PROJECT_ROOT%\database\init_db.sql %PI_USER%@%PI_HOST%:%PI_DIR%/database/

echo.
echo === Deployment complete ===
echo.
echo On Pi4, run:
echo   cd %PI_DIR%
echo   mkdir -p build ^&^& cd build
echo   cmake ..
echo   make
echo   ./accessd
pause
