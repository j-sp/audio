/******************************************************************************/
/* SPDX-FileCopyrightText: 2020 Javier Serrano <javi@orellut.net>             */
/* SPDX-License-Identifier: GPL-3.0-or-later                                  */
/* A simple frequency sweep program to test PortAudio                         */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>

#define DURATION_IN_SECONDS   (10)
#define SAMPLE_RATE_IN_HZ   (44100)
#define SINE_START_FREQ_IN_HZ (1000)
#define SINE_STOP_FREQ_IN_HZ (1000)
#define FRAMES_PER_BUFFER (1024)

typedef struct {
    float phase;
    float frequency;
    float freq_step;
    PaTime *callback_invoked_time;
    PaTime *first_sample_dac_time;
    PaTime *callback_done_time;
    PaStream *stream;
} sine;


static int freq_sweep_callback (const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData) {


    /* Cast data passed through stream to our structure. */
    sine *wave = (sine*) userData;

    *(wave->callback_invoked_time) = timeInfo->currentTime;
    printf("Curent time: %15.10f\n", timeInfo->currentTime);
    wave->callback_invoked_time++; 
    *(wave->first_sample_dac_time++) = Pa_GetStreamTime(wave->stream); 

    float *out = (float*) outputBuffer;
    float sample;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */
    float phase_step = 2*M_PI*(wave->frequency)/(float)SAMPLE_RATE_IN_HZ;

    for(i=0; i<framesPerBuffer; i++)
    {
        sample = sin(wave->phase);
        *out++ = sample;  /* left */
        *out++ = sample;  /* right */
        wave->phase += phase_step;
    }
    wave->frequency += wave->freq_step;
    *(wave->callback_done_time++) = Pa_GetStreamTime(wave->stream); 
    return 0;
}

int main(int argc, char *argv[]) {

    PaStream *stream;
    PaError err;
    int i;
    PaTime *invoked_start, *first_start, *done_start;
     
    printf("PortAudio Test: output frequency swept sine wave.\n");
    /* Initialize our data for use by callback. */
    sine waveform;

    float sine_start_freq = (float) SINE_START_FREQ_IN_HZ;
    float sine_stop_freq = (float) SINE_STOP_FREQ_IN_HZ;
    unsigned int duration = DURATION_IN_SECONDS;
    
    if (argc==4) {
        duration = atoi(argv[1]);
        sine_start_freq = atof(argv[2]);
        sine_stop_freq = atof(argv[3]);
    }

    unsigned int iterations = (unsigned int)
                              ((float) duration / ((float) FRAMES_PER_BUFFER / (float) SAMPLE_RATE_IN_HZ));

    waveform.frequency = (float) SINE_START_FREQ_IN_HZ;
    waveform.phase = 0.0;
    waveform.freq_step = (sine_stop_freq - sine_start_freq) / iterations;
    
    waveform.callback_invoked_time = malloc(iterations*sizeof(PaTime));
    waveform.first_sample_dac_time = malloc(iterations*sizeof(PaTime));
    waveform.callback_done_time = malloc(iterations*sizeof(PaTime));
    waveform.stream = stream;

    invoked_start = waveform.callback_invoked_time;
    first_start = waveform.first_sample_dac_time;
    done_start = waveform.callback_done_time;

    /* Initialize library before making any other calls. */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    
    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream (&stream,
                                0,          /* no input channels */
                                2,          /* stereo output */
                                paFloat32,  /* 32 bit floating point output */
                                SAMPLE_RATE_IN_HZ,
                                FRAMES_PER_BUFFER,
                                freq_sweep_callback,
                                &waveform);
    if( err != paNoError ) goto error;
 
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
 
    /* Sleep for several seconds. */
    Pa_Sleep(duration*1000/2);
 
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    
    Pa_Terminate();
    printf("Test finished.\n");
 
    waveform.callback_invoked_time = invoked_start;
    waveform.first_sample_dac_time = first_start;
    waveform.callback_done_time = done_start;

    for(i=0; i<iterations; i++) {
        printf("Iteration: %d:\t %15.10f %15.10f %15.10f\n", i, (float)*(invoked_start), *first_start++, *done_start++);
        invoked_start++;
    }

    free(waveform.callback_invoked_time);
    free(waveform.first_sample_dac_time);
    free(waveform.callback_done_time);
    return err;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}