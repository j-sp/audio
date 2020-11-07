/******************************************************************************/
/* SPDX-FileCopyrightText: 2020 Javier Serrano <javi@orellut.net>             */
/* SPDX-License-Identifier: GPL-3.0-or-later                                  */
/* A simple program to reproduce a classic result from "The Synthesis of      */
/* Complex Audio Spectra by Means of Frequency Modulation", by John M.        */
/* Chowning, first published in 1973. Specifically, calling this program like */
/* this:                                                                      */
/* ./fm_test 0.6 440 440 5                                                    */
/* reproduces a brass-like tone using simple frequency modulation as          */
/* described in page 7 of the paper                                           */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <math.h>
#include "adsr.h"

#define SAMPLE_RATE_IN_HZ   (44100)
#define FRAMES_PER_BUFFER (1024)


typedef struct {
    double t;
    double time_step;
    double phase;
    double phase_step;
    double freq;
    double mod_freq;
    double mod_index;
    double attack;
    double decay;
    double sustain;
    double sustain_level;
    double release;
} pa_data;

static int fm_test_callback (const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData) {
    
    pa_data *data = (pa_data*) userData;
    (void) inputBuffer; /* Prevent unused argument warning. */
    float *out = (float*) outputBuffer;
    int i;
    float sample;
    double inst_freq;
    double adsr_val;


    for (i=0; i<framesPerBuffer; i++) {
        adsr_val = adsr(data->t, data->attack, data->decay, data->sustain, data->sustain_level, data->release);
        inst_freq = data->freq + data->mod_index * adsr_val * data->mod_freq * 
                    sin(2 * M_PI * data->mod_freq * data->t); 
        sample = adsr_val * sin(data->phase);
        *out++ = sample; /*left */
        *out++ = sample; /* right */
        data->t += data->time_step;
        data->phase_step = 2 * M_PI * inst_freq * data->time_step;
        data->phase += data->phase_step;
        while (data->phase > 2*M_PI)
            data->phase -= 2*M_PI;
        while (data->phase < 0)
            data->phase += 2*M_PI;
    }

    return 0;
}

int main(int argc, char *argv[]) {

    /* audio_setup(adsr_test_callback); */

    double frequency, mod_frequency, mod_index,
           attack, decay, sustain, sustain_level, release, duration;
    int i;
    PaStream *stream;
    PaError err;
    pa_data data;
  
    if (argc != 5) {
        fprintf(stderr, "Wrong number of arguments.\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "fm_test duration frequency mod_frequency mod_index\n");
        return 0;
    }

    duration = atof(argv[1]);
    frequency = atof(argv[2]);
    mod_frequency = atof(argv[3]);
    mod_index = atof(argv[4]);
    attack = duration / 6;
    decay = attack;
    sustain = duration / 2;
    sustain_level = 0.5;
    release = attack;

    data.attack = attack;
    data.decay = decay;
    data.sustain = sustain;
    data.sustain_level = sustain_level;
    data.release = release;
    data.phase = 0.0;
    data.t = 0.0;
    data.freq = frequency;
    data.mod_freq = mod_frequency;
    data.mod_index = mod_index;
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
                                fm_test_callback,
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