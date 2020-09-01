//
//  signals.cpp
//
//  Created by bill on 8/26/20.
//

#include "signals.h"

//=============================================================
// FM Demod and lowpass filter

float FM_Demod::process(std::complex<float> &sample)
{
    // Apply input limiter
    std::complex<float> v;
    sample *= 1.0/(0.01 + std::abs(sample));    // limit amplitude to 1
    
    // FM demodulate
    v = sample * std::conj(lastSample);        // compute phase change vector
    lastSample = sample;                       // remember the this sample
    demodOut = std::imag(v);                   // 'Q' or imaginary part will contain the audio
    
    // lowpass filter the demod output
    filterOut = 0.98 * filterOut + 0.02 * demodOut;
    return filterOut;
}

//=============================================================
// OOK Demod and lowpass filter

float OOK_Demod::process(std::complex<float> &sample)
{
    // OOK demodulate
    demodOut = std::abs(sample);            // compute amplitude of the input
    
    // lowpass filter the demod output
    filterOut = 0.98 * filterOut + 0.02 * demodOut;
    return filterOut;
}

//=============================================================
// AM Demod and lowpass filter

float AM_Demod::process(std::complex<float> &sample)
{
    // OOK demodulate
    demodOut = std::abs(sample);            // compute amplitude of the input
    
    // remove carrier from the demodulated signal
    carrierLevel = 0.99999 * carrierLevel + 0.00001 * demodOut;
    demodOut -= carrierLevel;
    
    static int dec = 0;
    if (++dec > 100) {
        printf("%f %f\n", carrierLevel, demodOut);
        dec = 0;
    }

    // lowpass filter the demod output
    filterOut = 0.99 * filterOut + 0.01 * demodOut;
    
    return 10 * filterOut;
}
