@echo off

echo Compiling...
echo.

if not exist out mkdir out
pushd out

REM C4201: nonstandard extension used: nameless struct/union
REM C4100 : unreferenced formal parameter

set CompilerFlags=-FC -GR- -EHa- -nologo -Zi -Od -Oi -WX -W4 -wd4201 -wd4100
cl %CompilerFlags% ..\whererue4.cpp Ole32.lib shell32.lib Kernel32.lib Advapi32.lib
popd

echo.
echo Done.
