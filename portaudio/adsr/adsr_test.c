#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <math.h>
#include "audio.h"
#include "adsr.h"

#define SAMPLE_RATE_IN_HZ   (44100)
#define FRAMES_PER_BUFFER (1024)


typedef struct {
    double t;
    double time_step;
    double phase;
    double phase_step;
    double attack;
    double decay;
    double sustain;
    double sustain_level;
    double release;
} pa_data;

static int adsr_test_callback (const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData) {
    
    pa_data *data = (pa_data*) userData;
    (void) inputBuffer; /* Prevent unused argument warning. */
    float *out = (float*) outputBuffer;
    int i;
    float sample;

    for (i=0; i<framesPerBuffer; i++) {
        sample = adsr(data->t, data->attack, data->decay, data->sustain, data->sustain_level, data->release) *
                sin(data->phase);
        *out++ = sample; /*left */
        *out++ = sample; /* right */
        data->t += data->time_step;
        data->phase += data->phase_step;
        if (data->phase > 2*M_PI)
            data->phase -= 2*M_PI;
    }

    return 0;
}

int main(int argc, char *argv[]) {

    /* audio_setup(adsr_test_callback); */

    double frequency, attack, decay, sustain, sustain_level, release, duration;
    int i;
    PaStream *stream;
    PaError err;
    pa_data data;
  
    if (argc != 7) {
        fprintf(stderr, "Wrong number of arguments.\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "adsr_test frequency attack decay sustain sustain_level release\n");
        return 0;
    }

    frequency = atof(argv[1]);
    attack = atof(argv[2]);
    decay = atof(argv[3]);
    sustain = atof(argv[4]);
    sustain_level = atof(argv[5]);
    release = atof(argv[6]);

    duration = attack + decay + sustain + release;

    data.attack = attack;
    data.decay = decay;
    data.sustain = sustain;
    data.sustain_level = sustain_level;
    data.release = release;
    data.phase = 0.0;
    data.t = 0.0;
    data.time_step = 1.0 / SAMPLE_RATE_IN_HZ;
    data.phase_step = 2 * M_PI * frequency * data.time_step;

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    err = Pa_OpenDefaultStream (&stream,
                                0,           /* no input channels */ 
                                2,           /* stereo output */
                                paFloat32,   /* 32 bit floating point output */ 
                                SAMPLE_RATE_IN_HZ,
                                FRAMES_PER_BUFFER,
                                adsr_test_callback,
                                &data);
    if( err != paNoError ) goto error;

    err = Pa_StartStream(stream);
    if(err != paNoError) goto error;
 
    /* Sleep for duration of ADSR envelope*/
    Pa_Sleep(duration*1000);
 
    err = Pa_StopStream(stream);
    if(err != paNoError) goto error;
    
    err = Pa_CloseStream(stream);
    if(err != paNoError) goto error;
    
    Pa_Terminate();
    printf("Test finished.\n");
    return err;
error:
    Pa_Terminate();
    fprintf(stderr, "An error occured while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}