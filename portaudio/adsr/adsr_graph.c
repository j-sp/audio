#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include "audio.h"
#include "adsr.h"

static int adsr_test_callback (const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData) {

}

int main(int argc, char *argv[]) {

    /* audio_setup(adsr_test_callback); */

    FILE *fp;
    double attack, decay, sustain, sustain_level, release, time_step;
    const int number_of_points = 1000;
    int i;
    double level, t = 0.0;
  
    if (argc != 6) {
        fprintf(stderr, "Wrong number of arguments.\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "adsr_graph attack decay sustain sustain_level release\n");
        return 0;
    }

    attack = atof(argv[1]);
    decay = atof(argv[2]);
    sustain = atof(argv[3]);
    sustain_level = atof(argv[4]);
    release = atof(argv[5]);

    time_step = (attack + decay + sustain + release) / 1000.0;

    fp = fopen("diag.txt", "w");

    for(i=0; i<number_of_points; i++) {
        level = adsr(t, attack, decay, sustain, sustain_level, release);
        fprintf(fp, "%15.8f %10.8f\n", t, level);
        t += time_step;
    }

    fclose(fp);
}