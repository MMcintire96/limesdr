//
//  signals.hpp
//  L-SDR
//
//  Created by bill on 8/26/20.
//  Copyright © 2020 Bill McIntire. All rights reserved.
//

#ifndef signals_hpp
#define signals_hpp

#include <complex>

class BaseClass_Demod
{
public:
    virtual float process(std::complex<float> &sample) = 0;
};

class FM_Demod : public BaseClass_Demod
{
public:
    float process(std::complex<float> &sample);
    
private:
    float demodOut = 0;
    float filterOut = 0;
    std::complex<float> lastSample = {0, 0};
};

class OOK_Demod : public BaseClass_Demod
{
public:
    float process(std::complex<float> &sample);
    
private:
    float demodOut = 0;
    float filterOut = 0;
    std::complex<float> lastSample = {0, 0};
};


#endif /* signals_hpp */
