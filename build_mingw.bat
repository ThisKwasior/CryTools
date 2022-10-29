echo OFF

set INCLUDES= -I./include
set ARGS_DEBUG= -flto -g3 -std=c99
set ARGS_RELEASE= -flto -O3 -s -std=c99
set ARGS= %ARGS_DEBUG%

mkdir bin

echo Building sfdmux
gcc "./src/sfdmux.c" -o "./bin/sfdmux.exe" %ARGS% %INCLUDES% ^
	./lib/adx.c ./lib/m1v.c ./lib/mpeg.c ./lib/pathutils.c ^
	./lib/sfd.c ./lib/io_common.c ./lib/common.c ./lib/fd.c
	
echo Building test_adx
gcc "./src/test_adx.c" -o "./bin/test_adx.exe" %ARGS% %INCLUDES% ^
	./lib/adx.c ./lib/common.c
	
echo Building test_m1v
gcc "./src/test_m1v.c" -o "./bin/test_m1v.exe" %ARGS% %INCLUDES% ^
	./lib/m1v.c ./lib/common.c ./lib/io_common.c

echo Building test_mpeg
gcc "./src/test_mpeg.c" -o "./bin/test_mpeg.exe" %ARGS% %INCLUDES% ^
	./lib/mpeg.c ./lib/common.c
	
echo Building csc_video_conv
gcc "./src/csc_video_conv.c" -o "./bin/csc_video_conv.exe" %ARGS% %INCLUDES% ^
	./lib/mpeg.c ./lib/sfd.c ./lib/plane_converters.c ./lib/common.c ./lib/m1v.c ./lib/pathutils.c

echo Building sfdinfo
gcc "./src/sfdinfo.c" -o "./bin/sfdinfo.exe" %ARGS% %INCLUDES% ^
	./lib/mpeg.c ./lib/sfd.c ./lib/common.c ./lib/fd.c ^
	./lib/m1v.c

pause