echo OFF
set PROGRAMS= test_mpeg test_adx sfdmux
set INCLUDES= -I./include
set LIBS= ./lib/mpeg.c ./lib/utils.c ./lib/adx.c
set ARGS= -Qn -O2 -s

mkdir bin

for %%a in (%PROGRAMS%) do (
	echo Building %%a 
	gcc "./src/%%a.c" -o "./bin/%%a.exe" %ARGS% %INCLUDES% %LIBS%
)

pause