if not exist .tmp mkdir .tmp

.build\svfc.exe cpp meta\meta.txt > meta\meta.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe binary meta\meta.txt > meta\meta.blob 
if %errorlevel% neq 0 exit /b %errorlevel%

.build\embed.exe meta\meta.blob meta\meta.inc
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe cpp example\schema_a\schema_a.txt > example\schema_a\schema_a.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe binary example\schema_a\schema_a.txt > example\schema_a\schema_a.blob
if %errorlevel% neq 0 exit /b %errorlevel%

.build\embed.exe example\schema_a\schema_a.blob example\schema_a\schema_a.inc
if %errorlevel% neq 0 exit /b %errorlevel%

.build\test_file.exe
if %errorlevel% neq 0 exit /b %errorlevel%

@echo.
@echo --------
@echo Success!
@echo --------