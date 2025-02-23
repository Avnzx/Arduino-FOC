#include "pid.h"

PIDController::PIDController(float P, float I, float D, float ramp, float limit)
    : P(P)
    , I(I)
    , D(D)
    , output_ramp(ramp)    // output derivative limit [volts/second]
    , limit(limit)         // output supply limit     [volts]
    , error_prev(0.0f)
    , output_prev(0.0f)
    , integral_prev(0.0f)
{
    timestamp_prev = _micros();
}

// PID controller function
float PIDController::operator() (float error){
    // calculate the time from the last call
    unsigned long timestamp_now = _micros();
    float Ts = (timestamp_now - timestamp_prev) * 1e-6f;
    // quick fix for strange cases (micros overflow)
    if(Ts <= 0 || Ts > 0.5f) Ts = 1e-3f;
    float output = calc_pid(error, Ts);
    timestamp_prev = timestamp_now;

    return output;
}

float PIDController::operator() (float error, float Ts) {

    return calc_pid(error, Ts);
}

float PIDController::operator() (float error, float Ts, float Ts_inv) {

    return calc_pid(error, Ts, Ts_inv);
}

float PIDController::calc_pid (float error, float Ts) {
    return calc_pid(error, Ts, 1.0f/Ts);
}

float PIDController::calc_pid (float error, float Ts, float Ts_inv) {
        // u(s) = (P + I/s + Ds)e(s)
    // Discrete implementations
    // proportional part
    // u_p  = P *e(k)
    float proportional = P * error;
    // Tustin transform of the integral part
    // u_ik = u_ik_1  + I*Ts/2*(ek + ek_1)
    float integral = integral_prev + I*Ts*0.5f*(error + error_prev);
    // float integral = integral_prev + I*Ts*proportional;
    // antiwindup - limit the output
    integral = _constrain(integral, -limit, limit);
    // Discrete derivation
    // u_dk = D(ek - ek_1)/Ts
    float derivative = D*(error - error_prev)*Ts_inv;

    // sum all the components
    float output = proportional + integral + derivative;
    // antiwindup - limit the output variable
    output = _constrain(output, -limit, limit);

    // if output ramp defined
    if(output_ramp > 0){
        // limit the acceleration by ramping the output
        float output_rate = (output - output_prev)*Ts_inv;
        if (output_rate > output_ramp)
            output = output_prev + output_ramp*Ts;
        else if (output_rate < -output_ramp)
            output = output_prev - output_ramp*Ts;
    }
    // saving for the next pass
    integral_prev = integral;
    output_prev = output;
    error_prev = error;
    return output;
}

void PIDController::reset(){
    integral_prev = 0.0f;
    output_prev = 0.0f;
    error_prev = 0.0f;
}
