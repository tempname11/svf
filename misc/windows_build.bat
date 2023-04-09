cls
pushd .build

cl ^
  /std:c++20 ^
	/Zi ^
	/DCOMPILE_TIME_OPTION_BUILD_TYPE_DEVELOPMENT ^
	..\src\svfc.cpp ^
	..\src\platform_windows.cpp ^
	/link ^
	/DEBUG:FULL ^
	
popd