echo OFF
set PROGRAMS= test_mpeg test_adx sfdmux test_m1v
set INCLUDES= -I./include
set LIBS= ./lib/mpeg.c ./lib/utils.c ./lib/adx.c ./lib/sfd.c ./lib/m1v.c ./lib/common.c ./lib/io_common.c
set ARGS= -O2 -Wl,--gc-sections -std=c99

mkdir bin

for %%a in (%PROGRAMS%) do (
	echo Building %%a 
	gcc "./src/%%a.c" -o "./bin/%%a.exe" %ARGS% %INCLUDES% %LIBS%
)

pause