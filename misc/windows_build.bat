cls
pushd .build

cl ^
  /std:c++20 ^
	/Zi ^
	/DCOMPILE_TIME_OPTION_BUILD_TYPE_DEVELOPMENT ^
	..\src\exe_test_file.cpp ^
	..\src\platform_windows.cpp ^
	/link ^
	/DEBUG:FULL ^
	/OUT:test_file.exe
if %errorlevel% neq 0 exit /b %errorlevel%

@echo.
@echo ---------------
@echo "test_file": OK
@echo ---------------

cl ^
  /std:c++20 ^
	/Zi ^
	/DCOMPILE_TIME_OPTION_BUILD_TYPE_DEVELOPMENT ^
	..\src\exe_svfc.cpp ^
	..\src\parsing.cpp ^
	..\src\typechecking.cpp ^
	..\src\output_binary.cpp ^
	..\src\output_cpp.cpp ^
	..\src\platform_windows.cpp ^
	/link ^
	/DEBUG:FULL ^
	/OUT:svfc.exe
if %errorlevel% neq 0 exit /b %errorlevel%

@echo.
@echo ----------
@echo "svfc": OK
@echo ----------

cl ^
  /std:c++20 ^
	/Zi ^
	/DCOMPILE_TIME_OPTION_BUILD_TYPE_DEVELOPMENT ^
	..\src\exe_embed.cpp ^
	/link ^
	/DEBUG:FULL ^
	/OUT:embed.exe
if %errorlevel% neq 0 exit /b %errorlevel%

@echo.
@echo -----------
@echo "embed": OK
@echo -----------

@echo.
@echo ---------
@echo All done!
@echo ---------
	
popd