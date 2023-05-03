if not exist .tmp mkdir .tmp

.build\svfc.exe cpp meta\meta.txt > meta\meta.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe binary meta\meta.txt > meta\meta.blob 
if %errorlevel% neq 0 exit /b %errorlevel%

.build\embed.exe meta\meta.blob meta\meta.inc
if %errorlevel% neq 0 exit /b %errorlevel%

.build\test_file.exe
if %errorlevel% neq 0 exit /b %errorlevel%

@echo.
@echo --------
@echo Success!
@echo --------