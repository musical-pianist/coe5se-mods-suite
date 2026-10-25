@echo off
rem Checks that every DLL and dylib in this mod was built from the code in its src folder:
rem glu32.dll, coe5se_loader.dll, coe5se.c5plugin.dll, coe5se.dylib, coe5se_loader.dylib, coe5se.c5plugin.dylib.
rem It downloads the two compilers they were built with (llvm-mingw for the DLLs, zig for the dylibs,
rem each one pinned archive checked by its SHA-256), builds src into a temporary folder, and compares
rem each rebuilt file with the one in this folder: OK means byte-identical. Nothing is installed, and
rem the temporary folder is deleted at the end. mac-verify.command does the same on a Mac, with the
rem same compilers' Mac builds, which give the same bytes. Needs Windows 10 (1803) or later, for
rem curl.exe and tar.exe.
rem Run it by double-clicking, or from a command prompt: windows-verify.cmd [nopause]
setlocal EnableExtensions
set "MOD=%~dp0"
set "TC_URL=https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/llvm-mingw-20260922-ucrt-x86_64.zip"
set "TC_SHA256=e3ad77d117a4bea19a7a3b333341824d79a5a371004a10e25b8504e7b3047666"
set "TC_DIR=llvm-mingw-20260922-ucrt-x86_64"
set "ZIG_URL_1=https://ziglang.freetls.fastly.net/zig-x86_64-windows-0.16.0.zip"
set "ZIG_URL_2=https://ziglang.org/download/0.16.0/zig-x86_64-windows-0.16.0.zip"
set "ZIG_SHA256=68659eb5f1e4eb1437a722f1dd889c5a322c9954607f5edcf337bc3684a75a7e"
set "ZIG_DIR=zig-x86_64-windows-0.16.0"
set "FLAGS=-shared -O2 -Wall -Wextra -s -Wl,--no-insert-timestamp"
set "MAC_FLAGS=-target aarch64-macos.11.0 -dynamiclib -O2 -Wall -Wextra -fstack-protector -g0 -s"
set "WORK=%TEMP%\coe5_verify_%RANDOM%%RANDOM%"
set "OUT=%WORK%\out"
set "BUILD_FAILED="
set "FAILED="
set "PAUSE_AT_END="
rem A double-click runs "cmd.exe /c ...": pause at the end then, so the window stays open. The
rem command line is read with delayed expansion, so quotes and parentheses in it (a path under
rem "Program Files (x86)") can't break the parse.
if /i "%~1"=="nopause" goto :start
setlocal EnableDelayedExpansion
set "CMDLINE=!CMDCMDLINE!"
if /i not "!CMDLINE:/c =!"=="!CMDLINE!" (endlocal & set "PAUSE_AT_END=1") else endlocal

:start

echo CoE5SE build check
echo Rebuilds the 6 DLLs and dylibs in this folder from the source in src/ and compares them byte
echo for byte. SECURITY.md explains each step.
echo.
mkdir "%OUT%" || goto :error
echo Step 1 of 4: download the two pinned compilers and check their SHA-256
echo   llvm-mingw 20260922 (builds the Windows DLLs)
call :fetch llvm-mingw "%TC_URL%" "%WORK%\tc.zip" %TC_SHA256% || goto :error
echo   zig 0.16.0 (builds the Mac dylibs)
call :fetch zig "%ZIG_URL_1%" "%WORK%\zig.zip" %ZIG_SHA256% || call :fetch zig "%ZIG_URL_2%" "%WORK%\zig.zip" %ZIG_SHA256% || goto :error
tar.exe -xf "%WORK%\tc.zip" -C "%WORK%" || goto :error
tar.exe -xf "%WORK%\zig.zip" -C "%WORK%" || goto :error
set "CC=%WORK%\%TC_DIR%\bin\x86_64-w64-mingw32-clang.exe"
set "WINDRES=%WORK%\%TC_DIR%\bin\x86_64-w64-mingw32-windres.exe"
set "ZIG=%WORK%\%ZIG_DIR%\zig.exe"
rem zig keeps its cache in the temporary folder too, so nothing stays behind.
set "ZIG_GLOBAL_CACHE_DIR=%WORK%\zig-cache"
set "ZIG_LOCAL_CACHE_DIR=%WORK%\zig-cache"

echo.
echo Step 2 of 4: the compilers' own version lines
call :firstline "%CC%" --version
for /f "delims=" %%V in ('call "%ZIG%" version') do set "ZIG_REPORTED=%%V"
for /f "delims=" %%V in ('call "%ZIG%" cc --version') do if not defined ZIG_CLANG set "ZIG_CLANG=%%V"
echo     zig %ZIG_REPORTED%, %ZIG_CLANG%

echo.
echo Step 3 of 4: rebuild each file from src/ into the temporary folder %WORK%
pushd "%MOD%src" || goto :error
echo   glu32.dll
echo     ^> "%CC%" %FLAGS% -o "%OUT%\glu32.dll" frozen_stub/glu32_stub.c frozen_stub/glu32.def -lbcrypt -luser32
"%CC%" %FLAGS% -o "%OUT%\glu32.dll" frozen_stub/glu32_stub.c frozen_stub/glu32.def -lbcrypt -luser32
if errorlevel 1 set "BUILD_FAILED=1"
echo   coe5se_loader.dll
echo     ^> "%WINDRES%" -O coff -o "%WORK%\coe5se_loader.rc.o" coe5se_loader.rc
"%WINDRES%" -O coff -o "%WORK%\coe5se_loader.rc.o" coe5se_loader.rc
if errorlevel 1 set "BUILD_FAILED=1"
echo     ^> "%CC%" %FLAGS% -o "%OUT%\coe5se_loader.dll" coe5se_loader.c -lbcrypt -luser32 "%WORK%\coe5se_loader.rc.o"
"%CC%" %FLAGS% -o "%OUT%\coe5se_loader.dll" coe5se_loader.c -lbcrypt -luser32 "%WORK%\coe5se_loader.rc.o"
if errorlevel 1 set "BUILD_FAILED=1"
echo   coe5se.c5plugin.dll
echo     ^> "%WINDRES%" -O coff -o "%WORK%\coe5se.c5plugin.rc.o" coe5se.c5plugin.rc
"%WINDRES%" -O coff -o "%WORK%\coe5se.c5plugin.rc.o" coe5se.c5plugin.rc
if errorlevel 1 set "BUILD_FAILED=1"
echo     ^> "%CC%" %FLAGS% -o "%OUT%\coe5se.c5plugin.dll" coe5se_plugin.c fanpatch/fix_feeblemind.c fanpatch/fix_item_reset.c fanpatch/fix_token_sprites.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -I. -Ilua -lbcrypt "%WORK%\coe5se.c5plugin.rc.o"
"%CC%" %FLAGS% -o "%OUT%\coe5se.c5plugin.dll" coe5se_plugin.c fanpatch/fix_feeblemind.c fanpatch/fix_item_reset.c fanpatch/fix_token_sprites.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -I. -Ilua -lbcrypt "%WORK%\coe5se.c5plugin.rc.o"
if errorlevel 1 set "BUILD_FAILED=1"
echo   coe5se.dylib
echo     ^> "%ZIG%" cc %MAC_FLAGS% -install_name coe5se.dylib -o "%OUT%\coe5se.dylib" frozen_stub/mac_stub.c -isystem frozen_stub/mac_sdk frozen_stub/mac_sdk/CoreFoundation.tbd
"%ZIG%" cc %MAC_FLAGS% -install_name coe5se.dylib -o "%OUT%\coe5se.dylib" frozen_stub/mac_stub.c -isystem frozen_stub/mac_sdk frozen_stub/mac_sdk/CoreFoundation.tbd
if errorlevel 1 set "BUILD_FAILED=1"
echo   coe5se_loader.dylib
echo     ^> "%ZIG%" cc %MAC_FLAGS% -install_name coe5se_loader.dylib -o "%OUT%\coe5se_loader.dylib" coe5se_loader.c -isystem mac_sdk mac_sdk/CoreFoundation.tbd
"%ZIG%" cc %MAC_FLAGS% -install_name coe5se_loader.dylib -o "%OUT%\coe5se_loader.dylib" coe5se_loader.c -isystem mac_sdk mac_sdk/CoreFoundation.tbd
if errorlevel 1 set "BUILD_FAILED=1"
echo   coe5se.c5plugin.dylib
echo     ^> "%ZIG%" cc %MAC_FLAGS% -install_name coe5se.c5plugin.dylib -o "%OUT%\coe5se.c5plugin.dylib" coe5se_plugin.c fanpatch/fix_feeblemind.c fanpatch/fix_item_reset.c fanpatch/fix_token_sprites.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -I. -Ilua -isystem mac_sdk mac_sdk/CoreFoundation.tbd
"%ZIG%" cc %MAC_FLAGS% -install_name coe5se.c5plugin.dylib -o "%OUT%\coe5se.c5plugin.dylib" coe5se_plugin.c fanpatch/fix_feeblemind.c fanpatch/fix_item_reset.c fanpatch/fix_token_sprites.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -I. -Ilua -isystem mac_sdk mac_sdk/CoreFoundation.tbd
if errorlevel 1 set "BUILD_FAILED=1"
popd
if defined BUILD_FAILED goto :error

echo.
echo Step 4 of 4: compare each shipped file with its rebuild and with SHA256.txt
for %%D in (glu32.dll coe5se_loader.dll coe5se.c5plugin.dll coe5se.dylib coe5se_loader.dylib coe5se.c5plugin.dylib) do call :compare %%D
goto :done

rem Compares the shipped file %1 with its rebuild and with its line in SHA256.txt: OK only when all
rem three name the same SHA-256.
:compare
set "HASH_FILE=%MOD%%1"
call :hash SHIPPED
set "HASH_FILE=%OUT%\%1"
call :hash REBUILT
set "LISTED=no"
rem /l /b, not /x: SHA256.txt has LF line ends, and findstr's /x expects CRLF.
if defined SHIPPED findstr /l /b /c:"%SHIPPED%  %1" "%MOD%SHA256.txt" >nul 2>nul && set "LISTED=yes"
if defined SHIPPED if /i "%SHIPPED%"=="%REBUILT%" if "%LISTED%"=="yes" (
  echo   OK    %1  %REBUILT%
  exit /b
)
set "FAILED=1"
echo   FAIL  %1
echo         shipped:            %SHIPPED%
echo         rebuilt:            %REBUILT%
echo         listed in SHA256.txt: %LISTED%
exit /b

rem Prints the first line the command %* writes, indented.
:firstline
for /f "delims=" %%V in ('call %*') do (
  echo     %%V
  exit /b
)
exit /b

rem Downloads the URL %2 into the file %3 and checks it against the SHA-256 %4; %1 names the
rem compiler. Exits 1 when the download fails or gives another file.
:fetch
echo     from:      %~2
echo     expected:  %4
set "DL_URL=%~2"
set "DL_FILE=%~3"
where curl.exe >nul 2>nul
if errorlevel 1 (
  powershell -NoProfile -Command "$ProgressPreference='SilentlyContinue'; Invoke-WebRequest -UseBasicParsing -Uri $env:DL_URL -OutFile $env:DL_FILE" || (echo     download failed& exit /b 1)
) else (
  curl.exe -fL --retry 3 -sS -o "%DL_FILE%" "%DL_URL%" || (echo     download failed& exit /b 1)
)
set "HASH_FILE=%DL_FILE%"
call :hash DL_HASH
if /i not "%DL_HASH%"=="%4" (
  echo     got:       %DL_HASH%  MISMATCH, not used
  exit /b 1
)
echo     got:       %DL_HASH%  OK
exit /b 0

rem Sets the variable named %1 to the SHA-256 of the file in HASH_FILE (lowercase, as SHA256.txt
rem and mac-verify.command print it), or to nothing if it's missing. The path goes through the
rem environment, so spaces, parentheses, and quotes in it are safe.
:hash
set "%1="
for /f "usebackq delims=" %%H in (`powershell -NoProfile -Command "if (Test-Path -LiteralPath $env:HASH_FILE) { (Get-FileHash -Algorithm SHA256 -LiteralPath $env:HASH_FILE).Hash.ToLower() }"`) do set "%1=%%H"
exit /b

:error
set "FAILED=1"
echo Verification could not finish.

:done
cd /d "%MOD%"
rmdir /s /q "%WORK%" 2>nul
echo.
if defined FAILED (echo Result: FAIL) else (echo Result: OK, every DLL and dylib matches its source.)
echo The temporary folder is deleted. Nothing was installed.
if defined PAUSE_AT_END pause
if defined FAILED exit /b 1
exit /b 0
