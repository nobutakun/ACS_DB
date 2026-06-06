@echo off
REM ============================================
REM Deploy to Raspberry Pi 4
REM Run from Windows (requires password/ssh key)
REM ============================================

set PROJECT_ROOT=C:\Users\CAS\Work-Project\ACS_DB

echo === Deploying Access Control Database to Pi4 ===

REM Create directory structure first
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/database/schema"
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/database/migrations"
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/database/seeds"
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/management/user_mgmt"
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/management/credential_mgmt"
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/management/door_reader_mgmt"
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/services/accessd"
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/scripts"
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/logs"
ssh nobuta@Cv.local "mkdir -p /home/nobuta/accessd/config"

echo.
echo Copying schema files...
scp "%PROJECT_ROOT%\database\schema\*.sql" nobuta@Cv.local:/home/nobuta/accessd/database/schema/

echo.
echo Copying init scripts...
scp "%PROJECT_ROOT%\database\init_db.sh" nobuta@Cv.local:/home/nobuta/accessd/database/
scp "%PROJECT_ROOT%\database\init_db.sql" nobuta@Cv.local:/home/nobuta/accessd/database/
scp "%PROJECT_ROOT%\scripts\install.sh" nobuta@Cv.local:/home/nobuta/accessd/scripts/

echo.
echo === Deployment complete ===
echo.
echo On Pi4, run:
echo   cd /home/nobuta/accessd/database
echo   bash init_db.sh
pause
