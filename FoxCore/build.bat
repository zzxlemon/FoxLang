@echo off
cd /d "%~dp0"
:: 设置编码为GBK以匹配终端中文输出
chcp 936 > nul

if "%~1"=="clean" goto CLEAN

echo ====================================
echo BUILDING....
echo ====================================

if not exist "build" mkdir "build"
if not exist "builds" mkdir "builds"
del /q "builds\*.o" 2>nul

setlocal enabledelayedexpansion

:: 自动扫描所有子目录并生成头文件包含路径
set "include_paths="
for /r %%d in (.) do (
    if exist "%%d\*.h" set "include_paths=!include_paths! -I"%%d""
    if exist "%%d\*.hpp" set "include_paths=!include_paths! -I"%%d""
)

echo [1/2] 正在编译源文件...
set "obj_files="
set "has_cpp=0"

:: 循环编译当前目录及子目录下的所有 .cpp 文件
for /r %%f in (*.cpp) do (
    set "has_cpp=1"
    echo -^> 正在编译: %%~nxf ...
    g++ -c "%%f" %include_paths% -o "builds\%%~nxf.o" -finput-charset=UTF-8 -fexec-charset=GBK
    if !errorlevel! neq 0 (
        echo -----------------------------------
        echo [错误] 文件 %%~nxf 编译失败。
        goto END
    )
    set "obj_files=!obj_files! "builds\%%~nxf.o""
)

if "%has_cpp%"=="0" (
    echo [错误] 当前目录及子目录下没有找到任何 .cpp 文件。
    goto END
)

echo -----------------------------------
echo [2/2] 正在链接并生成可执行程序...
g++ %obj_files% -o build/fox.exe -lws2_32

if %errorlevel% equ 0 (
    echo -----------------------------------
    echo [成功] 编译并链接成功！
) else (
    echo ------------------------------------
    echo [错误] 链接失败，请检查上面的代码报错信息。
)
goto END

:CLEAN
if exist "builds" rd /s /q "builds"
if exist "build" rd /s /q "build"
echo -----------------------------------
echo [成功] 清理临时文件完成。
goto END

:END
echo ---------------END-----------------
