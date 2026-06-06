@echo off
REM ============================================
REM Deploy Accessd Service to Pi4 (Complete)
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
ssh %PI_USER%@%PI_HOST% "mkdir -p %PI_DIR%/database/schema %PI_DIR%/database/migrations %PI_DIR%/database/seed"
ssh %PI_USER%@%PI_HOST% "mkdir -p %PI_DIR%/logs"

echo.
echo Copying source files...
REM Core
scp %PROJECT_ROOT%\src\core\config_loader.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/core/
scp %PROJECT_ROOT%\src\core\main.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/core/

REM Database
scp %PROJECT_ROOT%\src\db\db_connection.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/db/

REM Access
scp %PROJECT_ROOT%\src\access\credential_resolver.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/access/
scp %PROJECT_ROOT%\src\access\access_rule_evaluator.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/access/

REM Event
scp %PROJECT_ROOT%\src\event\decision_response.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/event/
scp %PROJECT_ROOT%\src\event\door_controller.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/event/
scp %PROJECT_ROOT%\src\event\event_logger.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/event/
scp %PROJECT_ROOT%\src\event\access_logger.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/event/

REM Reader
scp %PROJECT_ROOT%\src\reader\rs485.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/reader/
scp %PROJECT_ROOT%\src\reader\osdp_connection.cpp %PI_USER%@%PI_HOST%:%PI_DIR%/src/reader/

echo.
echo Copying headers...
scp %PROJECT_ROOT%\include\*.hpp %PI_USER%@%PI_HOST%:%PI_DIR%/include/

echo.
echo Copying config...
scp %PROJECT_ROOT%\config\accessd.conf %PI_USER%@%PI_HOST%:%PI_DIR%/config/

echo.
echo Copying build files...
scp %PROJECT_ROOT%\CMakeLists.txt %PI_USER%@%PI_HOST%:%PI_DIR%/

echo.
echo Copying database...
scp %PROJECT_ROOT%\database\schema\*.sql %PI_USER%@%PI_HOST%:%PI_DIR%/database/schema/
scp %PROJECT_ROOT%\database\init_db.sh %PI_USER%@%PI_HOST%:%PI_DIR%/database/
scp %PROJECT_ROOT%\database\init_db.sql %PI_USER%@%PI_HOST%:%PI_DIR%/database/

echo.
echo Copying scripts...
scp %PROJECT_ROOT%\scripts\build_on_pi.sh %PI_USER%@%PI_HOST%:%PI_DIR%/scripts/

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
