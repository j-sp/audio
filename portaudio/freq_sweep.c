/******************************************************************************/
/* SPDX-FileCopyrightText: 2020 Javier Serrano <javi@orellut.net>             */
/* SPDX-License-Identifier: GPL-3.0-or-later                                  */
/* A simple frequency sweep program to test PortAudio                         */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>
#include <strings.h>

#define DURATION_IN_SECONDS   (10)
#define SAMPLE_RATE_IN_HZ   (44100)
#define SINE_START_FREQ_IN_HZ (1000)
#define SINE_STOP_FREQ_IN_HZ (1000)
#define FRAMES_PER_BUFFER (1024)

typedef struct {
    double phase;
    double frequency;
    double freq_step;
    PaTime *callback_invoked_time;
    PaTime *first_sample_dac_time;
    PaTime *callback_done_time;
    PaStream *stream;
    int counter;
} sine;


static int freq_sweep_callback (const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData) {


    /* Cast data passed through stream to our structure. */
    sine *wave = (sine*) userData;

    *(wave->callback_invoked_time) = timeInfo->currentTime;
    /* printf("Curent time: %15.10f\n", timeInfo->currentTime); */
    wave->callback_invoked_time++; 
    *(wave->first_sample_dac_time) = timeInfo->outputBufferDacTime;
    wave->first_sample_dac_time++;

    float *out = (float*) outputBuffer;
    double sample;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */
    double phase_step = 2*M_PI*(wave->frequency)/(double)SAMPLE_RATE_IN_HZ;


    /* fprintf(stderr,"fpb %d f %.1f\n", framesPerBuffer, wave->frequency); */
    for(i=0; i<framesPerBuffer; i++)
    {
        sample = sin(wave->phase);
        *out++ = sample;  /* left */
        *out++ = sample;  /* right */
        wave->phase += phase_step;
        if( wave->phase > 2*M_PI )
            wave->phase -= 2 * M_PI;
    }
    wave->frequency += wave->freq_step;
    *(wave->callback_done_time) = Pa_GetStreamTime(wave->stream); 
    wave->callback_done_time++;
    wave->counter++;
    return 0;
}

int main(int argc, char *argv[]) {

    PaStream *stream;
    PaError err;
    int i;
    PaTime *invoked_start, *first_start, *done_start;
    int numDevices;
    PaDeviceInfo *deviceInfo;
    PaDeviceIndex output_dev;
    float slack;

    PaStreamParameters outputParameters;
     
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

    waveform.frequency = (float) sine_start_freq;
    waveform.phase = 0.0;
    waveform.freq_step = (sine_stop_freq - sine_start_freq) / iterations;
    waveform.counter = 0;
    
    /* Being conservative in the size to account for the non-exact duration of sound */
    /* when using Pa_Sleep() (taking actually twice the memory we need in principle) */
    waveform.callback_invoked_time = malloc(2*iterations*sizeof(PaTime));
    waveform.first_sample_dac_time = malloc(2*iterations*sizeof(PaTime));
    waveform.callback_done_time = malloc(2*iterations*sizeof(PaTime));
    

    invoked_start = waveform.callback_invoked_time;
    first_start = waveform.first_sample_dac_time;
    done_start = waveform.callback_done_time;

    /* Initialize library before making any other calls. */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    
    numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 ) {
        printf( "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
        err = numDevices;
        goto error;
    }
    
    for(i=0; i<numDevices; i++) {
        printf("Device number %d: %s ", i, Pa_GetDeviceInfo(i)->name);
        printf("(Host API: %s)\n", Pa_GetHostApiInfo(Pa_GetDeviceInfo(i)->hostApi)->name);
    }
    printf("Please choose your device number (default: %d)\n", Pa_GetDefaultOutputDevice());
    scanf("%d", &output_dev);
    bzero( &outputParameters, sizeof( outputParameters ) ); 
    outputParameters.channelCount = 2;
    outputParameters.device = output_dev;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(output_dev)->defaultLowOutputLatency ;
    outputParameters.hostApiSpecificStreamInfo = NULL; 

err = Pa_OpenStream(
                &stream,
                NULL,
                &outputParameters,
                SAMPLE_RATE_IN_HZ,
                FRAMES_PER_BUFFER,
                paNoFlag, //flags that can be used to define dither, clip settings and more
                freq_sweep_callback, //your callback function
                &waveform); //data to be passed to callback. In C++, it is frequently (void *)this
//don't forget to check errors!

    /* Open an audio I/O stream. 
     err = Pa_OpenDefaultStream (&stream,
                                0,           no input channels 
                                2,           stereo output 
                                paFloat32,   32 bit floating point output 
                                SAMPLE_RATE_IN_HZ,
                                FRAMES_PER_BUFFER,
                                freq_sweep_callback,
                                &waveform); */
    if( err != paNoError ) goto error;

    waveform.stream = stream;
 
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
 
    /* Sleep for several seconds. */
    Pa_Sleep(duration*1000);
 
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    
    Pa_Terminate();
    printf("Test finished.\n");
 
    waveform.callback_invoked_time = invoked_start;
    waveform.first_sample_dac_time = first_start;
    waveform.callback_done_time = done_start;

    for(i=0; i<waveform.counter; i++) {
        slack = *first_start - *done_start;
        printf("Iteration %d:\t Invoke: %11.4f\t DAC: %11.4f\t Done: %11.4f\t Slack: %7.4f\n", i, (float)*(invoked_start), *first_start++, *done_start++, slack);
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