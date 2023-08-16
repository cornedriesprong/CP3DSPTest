//
//  KarplusVoice.cpp
//  
//
//  Created by Corné Driesprong on 02/06/2023.
//

#import "KarplusVoice.h"

#include <random>

// this is the number of samples we need to represent a full period
// of the lowest possible MIDI frequency
#define MAX_BUFFER_SIZE (8192)

KarplusVoice::KarplusVoice() {
    // allocate noise buffer
    // WARNING: don't call on the audio thread
    mBuffer = new double[MAX_BUFFER_SIZE];
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
        mBuffer[i] = 0.0;
}

void KarplusVoice::pluck(double freq,
                         double sampleRate,
                         double damping,
                         double tone,
                         double excitation) {
   
    // check parameters
    assert(freq >= 0 && freq <= 441000);
    assert(sampleRate >= 8000 && sampleRate <= 192000);
    assert(damping >= 0.0 && damping <= 1.0);
    assert(tone >= 0.0 && tone <= 1.0);
    assert(excitation >= 0.0 && excitation <= 1.0);
    
    // convert frequency to period in samples
    mPeriod = calculatePeriodInSamples(freq, sampleRate);
    assert(mPeriod <= MAX_BUFFER_SIZE);
    
    // pitch tracking: higher pitches should ring out longer
    double pitchTracking = fmax(5.0, (double)mPeriod / 7.);
    
    // determine window length, bigger window = more smoothing
    double scaledDamping = pow((damping * 0.75) + 0.25, 2);
    mWindowLength = std::max(2, (int)round(scaledDamping * pitchTracking));
    mBufferPos = 0;
    
    // mix triangle and noise
    double pluckLength = mPeriod * excitation;
    // fill noise buffer with -1.0 and 1.0 values
    for (int i = 0; i < mPeriod; i++) {
        // pad buffer with silence after pluck length
        if (i > pluckLength) {
            mBuffer[i] = 0.;
            continue;
        }
        // generate and mix triangle wave
        double triangle = generateTriangleWave(i);
        mBuffer[i] += triangle * tone;
       
        // mix noise
        mBuffer[i] += ((rand() % 2 == 0) ? -1.0 : 1.0) * (1.0 - tone);
    }
}

double KarplusVoice::process() {
    // if buffer is empty, do nothing
    if (mPeriod == 0) return 0.0;
    
    // loop around noise buffer
    mBufferPos = (mBufferPos + 1) % mPeriod;
    
    // smooth signal using simple moving average
    double sum = 0;
    for (int i = 0; i < mWindowLength; i++) {
        int index = (mBufferPos + i) % mPeriod;
        sum += mBuffer[index];
    }
    mBuffer[mBufferPos] = sum / mWindowLength;
    
    return mBuffer[mBufferPos];
}

int mPeriod = 0;
int mWindowLength = 0;
int mBufferPos = 0;
double *mBuffer;

// generate a triangle wave of the frequency given by mPeriod
double KarplusVoice::generateTriangleWave(int sample) {
    double phase = (double)sample / mPeriod;
    double triangle = 0;
    if (phase < 0.25) {
        triangle = 4 * phase;
    } else if (phase < 0.75) {
        triangle = 2 - 4 * phase;
    } else {
        triangle = -4 + 4 * phase;
    }
    return triangle;
}

// convert frequency to period
int KarplusVoice::calculatePeriodInSamples(double frequency, double sampleRate) {
    return static_cast<int>(sampleRate / frequency);
}
