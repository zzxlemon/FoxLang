@echo off
cd /d "%~dp0"

:: 设置编码
chcp 936 > nul

if "%~1"=="clean" goto CLEAN

echo ====================================
echo BUILDING....
echo ====================================

if not exist "build" mkdir "build"
if not exist "builds" mkdir "builds"

del /q "builds\*.o" 2>nul

setlocal EnableDelayedExpansion

:: =========================================================
:: 自动扫描所有头文件目录
:: =========================================================

set "include_paths="

for /r %%d in (.) do (
    if exist "%%d\*.h" (
        set "include_paths=!include_paths! -I""%%d"""
    )

    if exist "%%d\*.hpp" (
        set "include_paths=!include_paths! -I""%%d"""
    )
)

:: GLFW
set "glfw_include=-Inative\glfw\include"
set "glfw_lib=-Lnative\glfw\lib"

echo.
echo [1/2] 正在编译源文件...
echo.

set "obj_files="
set "has_cpp=0"

for /r %%f in (*.cpp) do (

    set "has_cpp=1"

    echo --> %%~nxf

    g++ -c "%%f" ^
        %include_paths% ^
        %glfw_include% ^
        -o "builds\%%~nf.o"

    if !errorlevel! neq 0 (
        echo.
        echo -----------------------------------
        echo [错误] %%~nxf 编译失败！
        goto END
    )

    set "obj_files=!obj_files! "builds\%%~nf.o""
)

if "%has_cpp%"=="0" (
    echo.
    echo [错误] 没有找到任何 cpp 文件！
    goto END
)

echo.
echo -----------------------------------
echo [2/2] 正在链接...
echo.

g++ ^
%obj_files% ^
%glfw_lib% ^
-lglfw3 ^
-lopengl32 ^
-lgdi32 ^
-luser32 ^
-lkernel32 ^
-lws2_32 ^
-o build\fox.exe

if %errorlevel% neq 0 (
    echo.
    echo -----------------------------------
    echo [错误] 链接失败！
    goto END
)

:: =========================================================
:: 自动复制 glfw3.dll（如果存在）
:: =========================================================

if exist "native\glfw\lib\glfw3.dll" (
    copy /Y "native\glfw\lib\glfw3.dll" "build\" >nul
)

echo.
echo -----------------------------------
echo [成功] 编译完成！
echo.

goto END

:CLEAN

if exist "builds" rd /s /q "builds"

if exist "build" rd /s /q "build"

echo.
echo -----------------------------------
echo [成功] 清理完成！
echo.

goto END

:END

echo --------------- END ----------------
pause