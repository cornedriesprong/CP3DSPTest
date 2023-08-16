//
//  main.cpp
//  CP3DSPTest
//
//  Created by Corné Driesprong on 02/06/2023.
//

#include "portaudio.h"
#include "KarplusVoice.h"
#include <CoreFoundation/CoreFoundation.h>

#define NUM_SECONDS (60)
#define SAMPLE_RATE (44100.0)
#define FRAMES_PER_BUFFER (256)
#define VOICE_COUNT (4)

typedef struct
{
    KarplusVoice *karplusVoices[VOICE_COUNT];
}
paTestData;

/* This routine will be called by the PortAudio engine when audio is needed.
 ** It may called at interrupt level on some machines so don't do anything
 ** that could mess up the system like calling malloc() or free().
 */
static int patestCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {
    
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;

    (void)timeInfo; /* Prevent unused variable warnings. */
    (void)statusFlags;
    (void)inputBuffer;
    
    for(unsigned long i = 0; i < framesPerBuffer; i++) {
        float mix = 0;

        for(int j = 0; j < VOICE_COUNT; j++) {
            KarplusVoice *karplusVoice = data->karplusVoices[j];
            mix += karplusVoice->process();
        }
        *out++ = (float)mix / VOICE_COUNT;
        *out++ = (float)mix / VOICE_COUNT;
    }
    
    return paContinue;
}

int main(void)
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    
    double freqs[4] = {261.63, 329.63, 392.00, 493.88};
    
    for (int i = 0; i < VOICE_COUNT; i++) {
        data.karplusVoices[i] = new KarplusVoice();
        
        double frequency = freqs[i] / 2;
        double damping = 0.0;
        double tone = 0.0;
        double excitation = 0.2;
        data.karplusVoices[i]->pluck(frequency,
                                     SAMPLE_RATE,
                                     pow(damping, 2),
                                     tone,
                                     excitation);
    }
    
    err = Pa_Initialize();
    if (err != paNoError)
        goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice)
    {
        fprintf(stderr, "Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = 2;         /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        NULL, /* no input */
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff, /* we won't output out of range samples so don't bother clipping them */
        patestCallback,
        &data);
    if (err != paNoError)
        goto error;

    err = Pa_StartStream(stream);
    if (err != paNoError)
        goto error;

    printf("Play for %d seconds.\n", NUM_SECONDS);
    Pa_Sleep(NUM_SECONDS * 1000);

    err = Pa_StopStream(stream);
    if (err != paNoError)
        goto error;

    err = Pa_CloseStream(stream);
    if (err != paNoError)
        goto error;

    Pa_Terminate();
    printf("Test finished.\n");

    return err;
error:
    Pa_Terminate();
    fprintf(stderr, "An error occurred while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}
