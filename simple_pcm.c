/******************************************************************************/
/* SPDX-FileCopyrightText: 2020 Javier Serrano <javi@orellut.net>             */
/* SPDX-License-Identifier: GPL-3.0-or-later                                  */
/* This is a simplified and modified version of the example code at           */
/* https://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html */
/* Work in progress                                                           */
/******************************************************************************/

#include <stdio.h>
#include <alsa/asoundlib.h>
#include <inttypes.h>
#include <math.h>

static char *sound_device = "default"; /* playback device */
static snd_pcm_format_t sample_format = SND_PCM_FORMAT_S16_LE; /* sample format */
static unsigned int nb_channels = 2; /* number of channels, 2 for stereo */
static unsigned int sample_rate = 44100; /* sample rate in Hz */
static unsigned int buffer_time = 500000; /* ring buffer length in us */
static unsigned int period_time = 100000; /* period time in us */
static double sine_freq = 1000; /* sinusoidal wave frequency in Hz */
static unsigned int playback_duration = 5; /* duration of playback in seconds */

static snd_pcm_sframes_t buffer_size; /* size of buffer size in samples (tbc) */
static snd_pcm_sframes_t period_size; /* size of period in samples (tbc) */

static int set_hwparams(snd_pcm_t *handle,
            snd_pcm_hw_params_t *params,
            snd_pcm_access_t access)
{
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;

    /* choose all parameters */
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0) {
        printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
        return err;
    }
    /* Restrict a configuration space to contain only real hardware rates */
    /* '1' in the last argument means 'enable' rate resampling */
    err = snd_pcm_hw_params_set_rate_resample(handle, params, 1);
    if (err < 0) {
        printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, params, access);
    if (err < 0) {
        printf("Access type not available for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* set the sample format */
    err = snd_pcm_hw_params_set_format(handle, params, sample_format);
    if (err < 0) {
        printf("Sample format not available for playback: %s\n", snd_strerror(err));
        return err;
    }
    printf("Sample format: %s (%u bits per sample)\n",snd_pcm_format_name(sample_format),
        snd_pcm_format_physical_width(sample_format));
    /* set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, params, 2);
    if (err < 0) {
        printf("Channels count (%u) not available for playbacks: %s\n", 2, snd_strerror(err));
        return err;
    }
    /* set the stream rate */
    rrate = sample_rate;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
    if (err < 0) {
        printf("Rate %uHz not available for playback: %s\n", sample_rate, snd_strerror(err));
        return err;
    }
    if (rrate != sample_rate) {
        printf("Rate doesn't match (requested %uHz, got %iHz)\n", sample_rate, rrate);
        return -EINVAL;
    }
    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
    if (err < 0) {
        printf("Unable to set buffer time %u for playback: %s\n", buffer_time, snd_strerror(err));
        return err;
    }
    printf("Buffer time set to %u microseconds\n", (unsigned int)buffer_time);
    err = snd_pcm_hw_params_get_buffer_size(params, &size);
    if (err < 0) {
        printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
        return err;
    }
    buffer_size = size;
    printf("Buffer size set to %u frames (1 frame is 2 samples)\n", (unsigned int)buffer_size);
    /* set the period time */
    err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
    if (err < 0) {
        printf("Unable to set period time %u for playback: %s\n", period_time, snd_strerror(err));
        return err;
    }
   printf("Period time set to %u microseconds\n", (unsigned int)period_time);
    err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
    if (err < 0) {
        printf("Unable to get period size for playback: %s\n", snd_strerror(err));
        return err;
    }
    period_size = size;
    printf("Period size set to %u frames (1 frame is 2 samples)\n", (unsigned int)period_size);   
    /* write the parameters to device */
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
        return err;
    }
    return 0;
}

static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams)
{
    int err;

    /* get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* start the transfer when the buffer is almost full: */
    /* (buffer_size / avail_min) * avail_min */
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
    if (err < 0) {
        printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* allow the transfer when at least period_size samples can be processed */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
    if (err < 0) {
        printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* write the parameters to the playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
        return err;
    }
    return 0;
}

static int xrun_recovery(snd_pcm_t *handle, int err)
{
    printf("Stream recovery\n");
    if (err == -EPIPE) {    /* under-run */
        err = snd_pcm_prepare(handle);
        if (err < 0)
            printf("Can't recover from underrun, snd_pcm_prepare failed: %s\n", snd_strerror(err));
        return 0;
    } else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume(handle)) == -EAGAIN)
            sleep(1);   /* wait until the suspend flag is released */
        if (err < 0) {
            err = snd_pcm_prepare(handle);
            if (err < 0)
                printf("Can't recover from suspend, snd_pcm_prepare failed: %s\n", snd_strerror(err));
        }
        return 0;
    }
    return err;
}

static void generate_sine(snd_pcm_sframes_t _period_size, unsigned int _nb_channels, 
                          int16_t *_samples, double *_phase) {
    static double max_phase = 2. * M_PI;
    double phase = *_phase;
    double step = max_phase*sine_freq/(double)sample_rate;
    unsigned int i = 0;
    int16_t res;

    unsigned int chn;
    int format_bits = snd_pcm_format_width(sample_format);
    unsigned int maxval = (1 << (format_bits - 1)) - 1;

    while (i < _period_size) {
        res = sin(phase) * maxval;
        _samples[2*i] = res;
        _samples[2*i+1] = res;
        phase += step;
        if (phase >= max_phase)
            phase -= max_phase;
        i++;
    }
    *_phase = phase;
}

static int playback(snd_pcm_t *handle,
                 int16_t *samples)
{
    double phase = 0;
    int16_t *ptr;
    int err, cptr;
    int iterations = playback_duration * 1000000 / period_time;
    while (iterations > 0) {
        generate_sine(period_size, nb_channels, samples, &phase);
        ptr = samples;
        cptr = period_size;
        while (cptr > 0) {
            err = snd_pcm_writei(handle, ptr, cptr);
            if (err == -EAGAIN)
                continue;
            if (err < 0) {
                if (xrun_recovery(handle, err) < 0) {
                    printf("Write error: %s\n", snd_strerror(err));
                    exit(EXIT_FAILURE);
                }
                break;  /* skip one period */
            }
            ptr += err * nb_channels;
            cptr -= err;
        }
        iterations--;
    }
}

int main(void) {

    int err;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    int16_t *samples;

    /* Allocate memory for hardware and software parameters */
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    printf("Playback device is %s\n", sound_device);
    printf("Stream parameters are %uHz, %u channels\n", 
            sample_rate, nb_channels);
    printf("Sine wave frequency is %.4fHz\n", sine_freq);

    /* Open PCM device. '0' in the last argument means standard */
    /* (blocking on write and read) mode */
    if ((err = snd_pcm_open(&handle, sound_device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("Playback open error: %s\n", snd_strerror(err));
        return err;
    }

    /* Set hardware parameters for the playback */
    /* We use interleaved mode, for left and right channels in the stereo stream */
    if ((err = set_hwparams(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)) {
        printf("Setting of hardware parameters failed: %s\n", snd_strerror(err));
        return err;
    }

    /* Set soft parameters for the playback */
    if ((err = set_swparams(handle, swparams)) < 0) {
        printf("Setting of soft parameters failed: %s\n", snd_strerror(err));
        return err;
    }

    /* Set aside memory for samples */
    samples = malloc((period_size * nb_channels * snd_pcm_format_physical_width(sample_format)) / 8);
    if (samples == NULL) {
        printf("Not enough memory\n");
        return -1;
    }

    playback(handle, samples);
   
    free(samples);
    snd_pcm_close(handle);
    return 0;

}