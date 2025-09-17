#include <math.h>
#include "correctHumidity.hpp"

float correctHumidity(float RH_measured, float T_dht, float T_env, float humidityCorrection) {
    if (isnan(RH_measured) || isnan(T_dht) || isnan(T_env) || RH_measured <= 0.0) {
        return NAN;
    }

    float A = 17.62;
    float B = 243.12;

    if (T_env < 0.0) {
        A = 22.46;
        B = 272.62;
    }

    float gamma_dht = (A * T_dht) / (B + T_dht);
    float gamma_env = (A * T_env) / (B + T_env);

    float factor = pow(10, gamma_dht - gamma_env);
    if (factor > humidityCorrection) factor = humidityCorrection;
    float RH_corrected = RH_measured * factor;

    if (RH_corrected < 0.0) RH_corrected = 0.0;
    if (RH_corrected > 100.0) RH_corrected = 100.0;

    return RH_corrected;
}
