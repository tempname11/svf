if not exist .tmp mkdir .tmp

.build\svfc.exe cpp > src\meta.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe binary > src\meta.blob 
if %errorlevel% neq 0 exit /b %errorlevel%

.build\embed.exe src/meta.blob src\meta.inc
if %errorlevel% neq 0 exit /b %errorlevel%

.build\test_file.exe
if %errorlevel% neq 0 exit /b %errorlevel%

@echo.
@echo --------
@echo Success!
@echo --------