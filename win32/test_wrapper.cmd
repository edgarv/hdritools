::  test_wrapper <PATH_PREPEND> <test_command> [arg1 arg2 ... arg7]
::    PATH_PREPEND - semicolon-separated list to prepend to the PATH
::                 - environment variable e.g. "C:\something;C:\another dir"
:: Helper batch script to launch the tests from the Visual Studio IDE,
:: modifying the path so that the tests can load the adecuate DLLs.
:: Use "NONE" as the value to PATH_PREPEND

@echo off

setlocal ENABLEDELAYEDEXPANSION

:: Check arguments
if "%~1"=="" goto BadParams
if "%~2"=="" goto BadParams
goto Continue

:BadParams
echo Error: insufficient arguments. Usage:
echo   %0 ^<PATH_PREPEND^> ^<test_command^> [arg1 arg2 ... arg7]
exit /b 1

:Continue

:: Workaround to preserve the path separator inside %1 using delayed expansion
set CMAKE_PATH_SEPARATOR=;

:: Extend the PATH
if not "%~1"=="NONE" set PATH=%~1;%PATH%

:: Execute the command followed by up to 7 arguments with quotation marks
%2 %3 %4 %5 %6 %7 %8 %9

:: Don't call endlocal to preserve the errorlevel, we are leaving anyway
exit /b %errorlevel%
