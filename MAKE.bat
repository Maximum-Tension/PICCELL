:::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::::::
:: BAT - MAKE                                    ;       Maximum Tension      ::
::,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,;,,,,,,,,,,,,,,,,,,,,,,,,,,,,::
::                                               ;     -__            __-     ::
:: Teoman Deniz                                  ; :    :!1!-_    _-!1!:    : ::
:: maximum-tension.com                           ; ::                      :: ::
::                                               ; :!:    : :: : :  :  ::::!: ::
:: +.....................+.....................+ ;  :!:: :!:!1:!:!::1:::!!!:  ::
:: : C - Maximum Tension : Create - 2026/11/18 : ;  ::!::!!1001010!:!11!!::   ::
:: :---------------------:---------------------: ;  :!1!!11000000000011!!:    ::
:: : License - GPL-3.0   : Update - 2026/11/21 : ;   ::::!!!1!!1!!!1!!!::     ::
:: +.....................+.....................+ ;      ::::!::!:::!::::      ::
::...............................................;............................::

:: MAKE.bat          PICCELL.exe with the gcc in PATH (64-bit or 32-bit)
:: MAKE.bat 32       PICCELL32.exe with the 32-bit gcc in C:\mingw32\bin
:: MAKE.bat clean    deletes what MAKE.bat made

@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
CD /D "%~dp0"

SET OUT=PICCELL.exe
SET BUILD=build

IF /I "%~1"=="clean" (
	IF EXIST build RMDIR /S /Q build
	IF EXIST build32 RMDIR /S /Q build32
	IF EXIST PICCELL.exe DEL /Q PICCELL.exe
	IF EXIST PICCELL32.exe DEL /Q PICCELL32.exe
	ECHO Cleaned.
	EXIT /B 0
)

IF "%~1"=="32" (
	IF NOT EXIST "C:\mingw32\bin\gcc.exe" (
		ECHO C:\mingw32\bin\gcc.exe was not found. Put a 32-bit MinGW-w64 there.
		EXIT /B 1
	)
	SET "PATH=C:\mingw32\bin;!PATH!"
	SET OUT=PICCELL32.exe
	SET BUILD=build32
)

WHERE gcc >NUL 2>NUL
IF ERRORLEVEL 1 (
	ECHO gcc was not found. Install MinGW-w64 and put its "bin" folder in PATH.
	EXIT /B 1
)
WHERE windres >NUL 2>NUL
IF ERRORLEVEL 1 (
	ECHO windres was not found. It comes with MinGW-w64, next to gcc.
	EXIT /B 1
)
IF NOT EXIST res\reader.bin (
	ECHO res\reader.bin was not found: the handwriting reader is missing.
	EXIT /B 1
)

:: THE WEBVIEW2 LOADER IN THE RESOURCES MUST BE AS WIDE AS THE EXE
SET ARCH=
FOR /F "delims=" %%M IN ('gcc -dumpmachine') DO SET MACHINE=%%M
ECHO !MACHINE! | FINDSTR /I "x86_64" >NUL
IF NOT ERRORLEVEL 1 SET ARCH=-DPICCELL_X64

IF NOT EXIST %BUILD% MKDIR %BUILD%

SET CFLAGS=-std=c99 -O2 -Wall -Wextra -Wno-misleading-indentation -Isrc
SET CFLAGS=%CFLAGS% -isystem webview -DWEBVIEW2_EMBED_LOADER=1
SET CFLAGS=%CFLAGS% -DWEBVIEW2_MEMORY_MODULE=1
SET LIBS=-lole32 -loleaut32 -lwindowscodecs -luuid -lshell32 -luser32
SET LIBS=%LIBS% -lcomdlg32 -lcomctl32 -lgdi32 -lwinhttp -lbcrypt

ECHO [1/3] Resources (!MACHINE!)
windres --codepage=65001 %ARCH% -I res -I src -i res\piccell.rc -O coff -o %BUILD%\piccell.res.o
IF ERRORLEVEL 1 GOTO FAIL

ECHO [2/3] Compiling
SET OBJECTS=
FOR %%F IN (src\*.c) DO (
	ECHO        %%~nxF
	gcc %CFLAGS% -c "%%F" -o "%BUILD%\%%~nF.o"
	IF ERRORLEVEL 1 GOTO FAIL
	SET OBJECTS=!OBJECTS! "%BUILD%\%%~nF.o"
)
ECHO        MemoryModule.c
gcc -O2 -w -c webview\MemoryModule.c -o %BUILD%\MemoryModule.o
IF ERRORLEVEL 1 GOTO FAIL

ECHO [3/3] Linking
gcc !OBJECTS! %BUILD%\MemoryModule.o %BUILD%\piccell.res.o -o %OUT% -municode -mwindows -static -s %LIBS%
IF ERRORLEVEL 1 GOTO FAIL

ECHO.
ECHO Done: %OUT%
EXIT /B 0

:FAIL
ECHO.
ECHO Build failed.
EXIT /B 1
