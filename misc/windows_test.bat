if not exist .tmp mkdir .tmp

.build\svfc.exe cpp meta\schema.txt > meta\schema.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe binary meta\schema.txt > meta\schema.blob 
if %errorlevel% neq 0 exit /b %errorlevel%

.build\embed.exe meta\schema.blob meta\schema.inc
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe cpp example\a0\schema.txt > example\a0\schema.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe binary example\a0\schema.txt > example\a0\schema.blob
if %errorlevel% neq 0 exit /b %errorlevel%

.build\embed.exe example\a0\schema.blob example\a0\schema.inc
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe cpp example\a1\schema.txt > example\a1\schema.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

.build\svfc.exe binary example\a1\schema.txt > example\a1\schema.blob
if %errorlevel% neq 0 exit /b %errorlevel%

.build\embed.exe example\a1\schema.blob example\a1\schema.inc
if %errorlevel% neq 0 exit /b %errorlevel%

.build\test_file.exe
if %errorlevel% neq 0 exit /b %errorlevel%

@echo.
@echo --------
@echo Success!
@echo --------