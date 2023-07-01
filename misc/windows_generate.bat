build\svfc.exe cpp meta\schema.txt > meta\schema.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

build\svfc.exe binary meta\schema.txt > meta\schema.blob 
if %errorlevel% neq 0 exit /b %errorlevel%

build\embed.exe meta\schema.blob meta\schema.inc
if %errorlevel% neq 0 exit /b %errorlevel%

build\svfc.exe cpp test\a0\schema.txt > test\a0\schema.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

build\svfc.exe binary test\a0\schema.txt > test\a0\schema.blob
if %errorlevel% neq 0 exit /b %errorlevel%

build\embed.exe test\a0\schema.blob test\a0\schema.inc
if %errorlevel% neq 0 exit /b %errorlevel%

build\svfc.exe cpp test\a1\schema.txt > test\a1\schema.hpp
if %errorlevel% neq 0 exit /b %errorlevel%

build\svfc.exe binary test\a1\schema.txt > test\a1\schema.blob
if %errorlevel% neq 0 exit /b %errorlevel%

build\embed.exe test\a1\schema.blob test\a1\schema.inc
if %errorlevel% neq 0 exit /b %errorlevel%

build\test_file.exe
if %errorlevel% neq 0 exit /b %errorlevel%

@echo.
@echo --------
@echo Success!
@echo --------