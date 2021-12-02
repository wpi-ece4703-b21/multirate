#include "xlaudio.h"

// This is an interpolation filter with 41 taps
// and four samples per symbol. It has a sinc(x) shape.
// Note that every fourth coefficient is zero, as we would
// expect for such an interpolation filter
#define NUMCOEF 41
#define RATE    4

float32_t coef[NUMCOEF] = {
    0.0000,
    0.0474,
    0.0707,
    0.0530,
   -0.0000,
   -0.0600,
   -0.0909,
   -0.0693,
    0.0000,
    0.0818,
    0.1273,
    0.1000,
   -0.0000,
   -0.1286,
   -0.2122,
   -0.1801,
    0.0000,
    0.3001,
    0.6366,
    0.9003,
    1.0000,
    0.9003,
    0.6366,
    0.3001,
    0.0000,
   -0.1801,
   -0.2122,
   -0.1286,
   -0.0000,
    0.1000,
    0.1273,
    0.0818,
    0.0000,
   -0.0693,
   -0.0909,
   -0.0600,
   -0.0000,
    0.0530,
    0.0707,
    0.0474,
    0.0000};

static int phase = 0;

// traditional implementation
float32_t taps[NUMCOEF];
uint16_t processSample(uint16_t x) {
    float32_t input = xlaudio_adc14_to_f32(x);

    // the input is 'upsampled' to RATE, by inserting N-1 zeroes every N samples
    phase = (phase + 1) % RATE;
    if (phase == 0)
        taps[0] = input;
    else
        taps[0] = 0.0;

    float32_t q = 0.0;
    uint16_t i;
    for (i = 0; i<NUMCOEF; i++)
        q += taps[i] * coef[i];

    for (i = NUMCOEF-1; i>0; i--)
        taps[i] = taps[i-1];

    return xlaudio_f32_to_dac14(0.1 * q);
}

float32_t symboltaps[NUMCOEF/RATE + 1];

uint16_t processSampleMultirate(uint16_t x) {
    float32_t input = xlaudio_adc14_to_f32(x);

    phase = (phase + 1) % RATE;

    uint16_t i;
    if (phase == 0) {
        symboltaps[0] = input;
        for (i=NUMCOEF/RATE; i>0; i--)
            symboltaps[i] = symboltaps[i-1];
    }

    float32_t q = 0.0;
    uint16_t limit = (phase == 0) ? NUMCOEF/RATE + 1 : NUMCOEF/RATE;
    for (i=0; i<limit; i++)
        q += symboltaps[i] * coef[i * RATE + phase + 1];

    return xlaudio_f32_to_dac14(0.1 * q);
}


#include <stdio.h>

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    int c = xlaudio_measurePerfSample(processSample);
    printf("Cycles Direct %d\n", c);

    c = xlaudio_measurePerfSample(processSampleMultirate);
    printf("Cycles Multirate %d\n", c);

    xlaudio_init_intr(FS_16000_HZ, XLAUDIO_J1_2_IN, processSampleMultirate);
    xlaudio_run();

    return 1;
}
