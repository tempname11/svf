cls
pushd .build

cl ^
  /std:c++20 ^
	/Zi ^
	/DCOMPILE_TIME_OPTION_BUILD_TYPE_DEVELOPMENT ^
	..\src\test_file.cpp ^
	..\src\platform_windows.cpp ^
	/link ^
	/DEBUG:FULL ^
	/OUT:test_file.exe
if %errorlevel% neq 0 exit /b %errorlevel%

cl ^
  /std:c++20 ^
	/Zi ^
	/DCOMPILE_TIME_OPTION_BUILD_TYPE_DEVELOPMENT ^
	..\src\svfc.cpp ^
	..\src\platform_windows.cpp ^
	/link ^
	/DEBUG:FULL ^
	/OUT:svfc.exe
if %errorlevel% neq 0 exit /b %errorlevel%
	
popd