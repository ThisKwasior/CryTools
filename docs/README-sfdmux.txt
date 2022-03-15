===========SFDMUX BETA===========

========USAGE========

        sfdmux.exe <output file> <mpeg file> <adx file 1> <adx file 2> <...>

        OUTPUT  - Output file.

        MPEG    - MPEG-PS Video. Audio will be ignored.
				  
        ADX     - Standard CRI ADPCM audio.
				  Multiple files are supported.

========ENCODING TUTORIAL========

        We'll be using FFmpeg for this.

        For fast and dirty SFD we need to reencode the source
        to mpeg1video and adpcm_adx.
        Here's as follows:

                ffmpeg -i video.webm -an -c:v mpeg1video video.mpeg

        Resulting video will have no audio streams and bitrate suggested by FFmpeg.
        Change bitrate to your needs.
        Also some games can be picky about resolutions, change that too if
        the video skips or game crashes.

        I also recommend 2pass encoding:

                ffmpeg -i vid.webm -an -c:v mpeg1video -b:v 8M -pass 1 -f mpeg NUL
                ffmpeg -i vid.webm -an -c:v mpeg1video -b:v 8M -pass 2 -f mpeg video.mpeg

        For Linux, change NUL to /dev/null

        Now for audio:

                ffmpeg -i video.webm -vn audio.adx

        Resulting ADX will be of nice quality, but for greater control over this
        I recommend VGAudio.

                https://github.com/Thealexbarney/VGAudio

        After all of this we can finally mux.

                sfdmux.exe movie.sfd video.mpeg audio.adx

========DISCLAIMER========

        This program is only intended for muxing audio and video.
        This means it does not encode anything.
        Quality of the video/audio is dependent on you only.

        Program is provided as-is.
        Don't sue me over fried PCs or something.

========CREDITS========

        Programmer      - Kwasior
                          https://github.com/ThisKwasior
                          https://twitter.com/ThisKwasior