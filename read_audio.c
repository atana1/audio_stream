#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "fft.h"

#define WINDOWSIZE 4


/* Structure to contain digital audio data */
typedef struct {
    double *data;
    int nsamples;
} AudioData;


/* returns maximum value from an array conditioned on start and end index */
double getMax(double *res_arr, int startIndex, int endIndex) {
    int i;
    double max = res_arr[startIndex];
    for (i = startIndex; i <= endIndex; i++) {
	    if (res_arr[i] > max) {
	        max = res_arr[i];
	    }
    }
    return max;
}


/* Stores peak frequency for each window(of chunkSize) in peaks array */
void getPeakPointInChunk(int chunkSize, double *res_arr, int size, double *peaks) {
    int endIndex, i = 0, peakIndex = 0;
    int startIndex = 0;
    double max;
    // get a chunk and find max value in it
    while (i < size) {
	    if (i % chunkSize-1 == 0) {
            max = getMax(res_arr, startIndex, i);
	        peaks[peakIndex++] = max;
	        startIndex = startIndex + chunkSize;
	    }
        i += 1;
    }
}


/* read audio stream from input file */
void readAudio (const char *url, int *sampleRate, AudioData *a)
{
    // Initialize FFmpeg
    av_register_all ();

    AVFormatContext *formatContext = NULL;
    AVCodecContext *codecContext = NULL;
    AVCodec *codec = NULL;
    AVPacket packet;
    AVFrame frame;

    ssize_t nSamples = 0;

    // open the file and autodetect the format
    if (avformat_open_input (&formatContext, url, NULL, NULL) < 0)
        printf("Can not open input file %s", url);

    if (avformat_find_stream_info (formatContext, NULL) < 0)
        printf("Can not find stream information");

    // find audio stream (between video/audio/subtitles/.. streams)
        int audioStreamId = av_find_best_stream (formatContext, AVMEDIA_TYPE_AUDIO,
                                            -1, -1, &codec, 0);
    if (audioStreamId < 0)
        printf("Can not find audio stream in the input file");

    codecContext = formatContext->streams[audioStreamId]->codec;

    // init the audio decoder
    if (avcodec_open2 (codecContext, codec, NULL) < 0)
        printf("Can not open audio decoder");

    // read all packets
    int i = 0;
    while (av_read_frame (formatContext, &packet) == 0)  {
        if (packet.stream_index == audioStreamId)  {
            avcodec_get_frame_defaults (&frame);
            int gotFrame = 0;
            if (avcodec_decode_audio4 (codecContext,&frame,&gotFrame,&packet) < 0){
                printf("Error decoding audio");
            }
            if (gotFrame)  {
                // audio frame has been decoded
                int size = av_samples_get_buffer_size (NULL,
                                                   codecContext->channels,
                                                   frame.nb_samples,
                                                   codecContext->sample_fmt,
                                                   1);
                if (size < 0)  {
                    printf("av_samples_get_buffer_size invalid value");
                }

	            // printf("%d\n", *frame.data[0]);
                a->data[i] = (double)*frame.data[0];
                i += 1;
                nSamples += frame.nb_samples;
            }
        }
        av_free_packet (&packet);
    }
    a->nsamples = i;

    if (nSamples < 1)
        printf("Decoded audio data is empty");

    int sampleSize = av_get_bytes_per_sample (codecContext->sample_fmt);
    if (sampleSize < 1)
        printf("Invalid sample format");

    // optional, return value with sample rate
    if (sampleRate) *sampleRate = codecContext->sample_rate;

    if (codecContext) avcodec_close (codecContext);

    avformat_close_input (&formatContext);
}


int main(int argc, char **argv) {

    int i, k, size, chunkSize, chunkSampleSize, resSize, sample_rate;
    double *fft_res;
    long m;
    char *s = NULL;

    if (argc < 2) {
        printf ("Usage: ./read_audio <inputFile>\n");
        return 1;
    }

    s = argv[1];
    AudioData ad;

    readAudio (s, &sample_rate, &ad);
    size = ad.nsamples;
    printf("size is %d\n", size);
    printf("sample rate is %d\n", sample_rate);
    printf("Time Domain data:\n");
    for (i = 0; i < size; i++) { printf("%f\n", ad.data[i]);}

    //m = log2(chunkSize);
    m = 2;
    chunkSize = WINDOWSIZE;
    chunkSampleSize = size/chunkSize;
    double *img_part = malloc(sizeof(double)* chunkSize);

    // initialize imaginary part with zeros
    for (i = 0; i < chunkSize; i++) { img_part[i] = 0; }
    resSize = chunkSize * chunkSampleSize;
    fft_res = malloc(sizeof(double) * resSize);

    double *temp = malloc(sizeof(double)*chunkSize);

    // FFT transform for windowed time domain data
    // window is of size chunkSize
    k = 0;
    while (k < resSize) {
        //copy data
        for (i = 0; i < chunkSize; i++) {
            temp[i] = ad.data[i+k];
        }
        FFT(1, m, temp, img_part);

        for (i = 0; i < chunkSize; i++) {
	    fft_res[i+k] = temp[i];
        }
        k = k + 4;
    }

    printf("Frequency Domain data:\n");
    for (i = 0; i < resSize; i++) { printf("%f\n", fft_res[i]); }

    int pSize = resSize/chunkSize;
    double *peaks = malloc(sizeof(double)*pSize);
    getPeakPointInChunk(chunkSize, fft_res, resSize, peaks);

    // peaks in frequency domain window
    printf("All peaks are:\n");
    for (i = 0; i < pSize; i++) { printf("%f\n", peaks[i]); }
    return 0;
}
