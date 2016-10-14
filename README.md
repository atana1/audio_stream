# audio_stream
Reads audio stream from an audio input file and displays time-domain, frequency domain, peaks in frequency domain data.


## Instruction to Run

 gcc read_audio.c -o read_audio fft.h fft.c -Ilibav/include -Llibav/lib -lavformat -lavcodec
-lavutil  -lz -lm -lpthread

 ./read_audio inputf:wqile


## Note

Use FFmpeg to generate audio with lower bit rate

 ffmpeg -i inputfile -ab 100k outputfile
