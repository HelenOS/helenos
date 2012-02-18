rem @echo off

set PCCDIR=
set PREFIX=
set usecl=
set doinstall=false

:parsecommandline
if '%1' == '/h' goto dispinfo
if '%1' == '/pcc' goto usepcc
if '%1' == '/cl' goto usecl
if '%1' == '/prefix' goto prefix
if '%1' == '/pccdir' goto pccdir
if '%1' == '/pccsrcdir' goto pccsrcdir
if '%1' == '/pcclibssrcdir' goto pcclibssrcdir
if '%1' == '/install' set doinstall=true
goto build

:dispinfo
echo build.bat [/h] { /pcc or /cl } [/prefix -dir-] [/pccdir -dir-] [/pccsrcdir -dir-] [/pcclibssrcdir -dir-] [/install]
goto end

:prefix
shift
set PREFIX=%1
shift
goto parsecommandline

:pccdir
shift
set PCCDIR=%1
shift
goto parsecommandline

:pccsrcdir
shift
set PCCSRCDIR=%1
shift
goto parsecommandline

:pcclibssrcdir
shift
set PCCLIBSSRCDIR=%1
shift
goto parsecommandline

:usecl
set CC=cl.exe -D__MSC__
set CFLAGS=/nologo /Zi /MT /W2
set CFLAGS2=/nologo /Zi /MD /Za /Wall /GS-
set OBJ=obj
set AR=lib.exe /nologo
set AR_OUT=/OUT:libpcc.a
set usecl=true
shift
goto parsecommandline

:usepcc
set CC=pcc.exe
set CFLAGS=-g
set CFLAGS2=-fno-stack-protector-all
set OBJ=o
set AR=ar.exe
set AR_OUT=r libpcc.a
set usecl=false
shift
goto parsecommandline

:build

if '%usecl%' == '' goto dispinfo

set PREFIX=###%PREFIX%###
set PREFIX=%PREFIX:"###=%
set PREFIX=%PREFIX:###"=%
set PREFIX=%PREFIX:###=%

set PCCDIR=###%PCCDIR%###
set PCCDIR=%PCCDIR:"###=%
set PCCDIR=%PCCDIR:###"=%
set PCCDIR=%PCCDIR:###=%

set PCCSRCDIR=###%PCCSRCDIR%###
set PCCSRCDIR=%PCCSRCDIR:"###=%
set PCCSRCDIR=%PCCSRCDIR:###"=%
set PCCSRCDIR=%PCCSRCDIR:###=%

set PCCLIBSSRCDIR=###%PCCLIBSSRCDIR%###
set PCCLIBSSRCDIR=%PCCLIBSSRCDIR:"###=%
set PCCLIBSSRCDIR=%PCCLIBSSRCDIR:###"=%
set PCCLIBSSRCDIR=%PCCLIBSSRCDIR:###=%

if not '%PCCDIR%' == '' goto pccdirset
set PCCDIR=C:\Program Files\pcc
:pccdirset

if not '%PCCSRCDIR%' == '' goto pccsrcdirset
set PCCSRCDIR=..\..
:pccsrcdirset

if not '%PCCLIBSSRCDIR%' == '' goto pcclibssrcdirset
set PCCLIBSSRCDIR=..\..\..\pcc-libs
:pcclibssrcdirset

if '%usecl%' == 'true' goto ccprefixed
set CC="%PCCDIR%\bin\%CC%"
set AR="%PCCDIR%\bin\%AR%"
:ccprefixed

set TARGOS=win32
set MACH=i386
set LIBEXECDIR=""

set MIPDIR=%PCCSRCDIR%\mip
set CPPDIR=%PCCSRCDIR%\cc\cpp
set CCOMDIR=%PCCSRCDIR%\cc\ccom
set CCDIR=%PCCSRCDIR%\cc\cc
set OSDIR=%PCCSRCDIR%\os\%TARGOS%
set MACHDIR=%PCCSRCDIR%\arch\%MACH%
set BISON_SIMPLE=%OSDIR%\bison.simple
set CPPFLAGS=-DWIN32 -DGCC_COMPAT -DPCC_DEBUG -DCPP_DEBUG -DTARGOS=%TARGOS% -Dos_%TARGOS% -Dmach_%MACH% -DLIBEXECDIR=%LIBEXECDIR% -D_CRT_SECURE_NO_WARNINGS

del *.obj *.o *.exe

%CC% -o pcc.exe %CPPFLAGS% %CFLAGS% -I%CCDIR% -I%OSDIR% -I%MACHDIR% -I%MIPDIR% %CCDIR%\cc.c %MIPDIR%\compat.c

bison -y -t -d --no-lines %CPPDIR%\cpy.y
rem flex %CPPDIR%\scanner.l
rem %CC% -o cpp.exe %CPPFLAGS% %CFLAGS% -I%CPPDIR% -I%OSDIR% -I%MACHDIR% -I%MIPDIR% -I. %CPPDIR%\cpp.c %MIPDIR%\compat.c y.tab.c lex.yy.c "C:\Program Files\UnxUtils\usr\local\lib\libfl.lib"
%CC% -o cpp.exe %CPPFLAGS% %CFLAGS% -I%CPPDIR% -I%OSDIR% -I%MACHDIR% -I%MIPDIR% -I. %CPPDIR%\cpp.c %CPPDIR%\token.c %MIPDIR%\compat.c y.tab.c "C:\Program Files\UnxUtils\usr\local\lib\libfl.lib"

%CC% -o mkext.exe -DMKEXT %CPPFLAGS% %CFLAGS% -I%CCOMDIR% -I%OSDIR% -I%MACHDIR% -I%MIPDIR% %MIPDIR%\mkext.c %MACHDIR%\table.c %MIPDIR%\common.c
mkext
bison -y -t -d --no-lines %CCOMDIR%\cgram.y
move y.tab.c cgram.c
move y.tab.h cgram.h
flex %CCOMDIR%\scan.l
move lex.yy.c scan.c

%CC% -o ccom.exe %CPPFLAGS% %CFLAGS% -I%CCOMDIR% -I%OSDIR% -I%MACHDIR% -I%MIPDIR% -I. %CCOMDIR%\main.c %MIPDIR%\compat.c scan.c cgram.c external.c %CCOMDIR%\optim.c %CCOMDIR%\pftn.c %CCOMDIR%\trees.c %CCOMDIR%\inline.c %CCOMDIR%\symtabs.c %CCOMDIR%\init.c %MACHDIR%\local.c %MACHDIR%\code.c %CCOMDIR%\stabs.c %CCOMDIR%\gcc_compat.c %MIPDIR%\match.c %MIPDIR%\reader.c %MIPDIR%\optim2.c %MIPDIR%\regs.c %MACHDIR%\local2.c %MACHDIR%\order.c %MACHDIR%\table.c %MIPDIR%\common.c "C:\Program Files\UnxUtils\usr\local\lib\libfl.lib"

if not '%PREFIX%' == '' goto prefixset
set PREFIX=C:\Program Files\pcc
:prefixset

set PCCDESTDIR=%PREFIX%
set LIBPCCDESTDIR=%PREFIX%\lib\i386-win32\0.9.9

set LIBPCCDIR=%PCCLIBSSRCDIR%\libpcc
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\_alloca.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\adddi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\anddi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\ashldi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\ashrdi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\cmpdi2.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\divdi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\fixdfdi.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\fixsfdi.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\fixunsdfdi.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\fixunssfdi.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\floatdidf.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\floatdisf.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\floatunsdidf.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\iordi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\lshldi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\lshrdi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\moddi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\muldi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\negdi2.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\notdi2.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\qdivrem.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\ssp.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\subdi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\ucmpdi2.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\udivdi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\umoddi3.c
%CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\xordi3.c

if '%usecl%' == 'false' %CC% -c %CPPFLAGS% %CFLAGS2% -I%LIBPCCDIR% %LIBPCCDIR%\_ftol.c
if '%usecl%' == 'true' ml /nologo -c %LIBPCCDIR%\_ftol.asm

%AR% %AR_OUT% _ftol.%OBJ% adddi3.%OBJ% anddi3.%OBJ% ashldi3.%OBJ% ashrdi3.%OBJ% cmpdi2.%OBJ% divdi3.%OBJ% fixdfdi.%OBJ% fixsfdi.%OBJ% fixunsdfdi.%OBJ% fixunssfdi.%OBJ% floatdidf.%OBJ% floatdisf.%OBJ% floatunsdidf.%OBJ% iordi3.%OBJ% lshldi3.%OBJ% lshrdi3.%OBJ% moddi3.%OBJ% muldi3.%OBJ% negdi2.%OBJ% notdi2.%OBJ% qdivrem.%OBJ% ssp.%OBJ% subdi3.%OBJ% ucmpdi2.%OBJ% udivdi3.%OBJ% umoddi3.%OBJ% xordi3.%OBJ%

if not '%doinstall%' == 'true' goto end

md "%PCCDESTDIR%"
md "%PCCDESTDIR%\bin"
md "%PCCDESTDIR%\libexec"
md "%PCCDESTDIR%\man"
md "%PCCDESTDIR%\man\man1"
md "%LIBPCCDESTDIR%\lib"
md "%LIBPCCDESTDIR%\include"

copy pcc.exe "%PCCDESTDIR%\bin"
copy cpp.exe "%PCCDESTDIR%\libexec"
copy ccom.exe "%PCCDESTDIR%\libexec"

copy libpcc.a "%LIBPCCDESTDIR%\lib"
copy "%LIBPCCDIR%\include\*.h" "%LIBPCCDESTDIR%\include"

copy "%CCDIR%\cc.1" "%PCCDESTDIR%\man\man1"
copy "%CPPDIR%\cpp.1" "%PCCDESTDIR%\man\man1"
copy "%CCOMDIR%\ccom.1" "%PCCDESTDIR%\man\man1"

:end
