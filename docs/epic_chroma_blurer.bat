rem Converts YUV420 to YUV444 by scaling CrCb components
rem with Lanczos to size of Y component
rem and outputs PNG files

set FILE=e0000_converted.yuv
set WIDTH=640
set HEIGHT=480
set FPS=29.970
set YUV=yuv420p

mkdir out

ffmpeg -s %WIDTH%x%HEIGHT% -pix_fmt %YUV% -framerate %FPS% -i %FILE% ^
	   -filter_complex "extractplanes=y+u+v[y][u][v],[u]scale=w=%WIDTH%:h=%HEIGHT%:flags=lanczos[usc],[v]scale=w=%WIDTH%:h=%HEIGHT%:flags=lanczos[vsc],[y][usc][vsc]mergeplanes=0x001020:yuv444p[yuv]" ^
	   -map "[yuv]" out/%%05d.png
	   
pause