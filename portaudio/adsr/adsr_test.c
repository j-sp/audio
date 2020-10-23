#include "audio.h"
#include "adsr.h"

static int adsr_test_callback (const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData) {

}

int main(void) {

    audio_setup(adsr_test_callback);
    

}