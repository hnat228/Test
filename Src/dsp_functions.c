
#include "dsp_functions.h"
#include "math.h"
#include "stdint.h"

static const uint32_t Fs = 48000;       // частота семплирования   
static const uint16_t Vs = 343;         // клрость звука м/с
static const double Pi = 3.1415;

static double coef3[3];
static double coef5[5];
static double a0, omega, gain_t, alfa, orderangle, Ax, gainlin, sn, cs, sqrtAlfa, AxPl, AxMi;
static double Boost_T, Boost_B, A_T, A_B, Knum_T, Kden_T, Knum_B, Kden_B, a10, a11, a20, a21, b10, b11, b20, b21; 

double gainliner(double gain)
{
  gainlin = pow(10.0, (gain / 20.0));
  return gainlin;
}

double sound_delay(double distance)
{
  double tmp = (uint16_t)((distance * Fs) / Vs);
  if(tmp == 0) tmp = 1; 
  return (tmp / 0x00800000);
}

double sound_delay_smpl(uint16_t samples)
{
  double tmp = samples;
  if(tmp == 0) tmp = 1;
  return (tmp / 0x00800000);
}

double *butt_1st_lowpass(uint16_t freq, int8_t gain)
{
  gain_t = pow(10.0, (gain / 20.0));
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega);
  a0 = sn + cs + 1;
  
  coef3[2] = (sn - cs - 1) / a0;      // A1
  coef3[0] = gain_t * sn / a0;        // B0        
  coef3[1] = gain_t * sn / a0;        // B1  

  return coef3; 
}

double *butt_1st_highpass(uint16_t freq, int8_t gain)
{
  gain_t = pow(10.0, (gain / 20.0));
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega);
  a0 = sn + cs + 1;
   
  coef3[2] = (sn - cs - 1) / a0;         // A1
  coef3[1] = gain_t * (1 + cs) / a0;     // B0        
  coef3[0] = - gain_t * (1 + cs) / a0;   // B1  

  return coef3;
}

double *butt_1st_highpass_5coef(uint16_t freq, int8_t gain)
{
  gain_t = pow(10.0, (gain / 20.0));
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega);
  a0 = sn + cs + 1;
  
  coef5[4] = (2 * cs) / a0;                     // A1
  coef5[3] = 0;                                 // A2
  coef5[1] = - gain_t * (1 + cs) / a0;          // B1
  coef5[2] = gain_t * (1 + cs) / a0;            // B0
  coef5[0] = 0;                                 // B2
  
  return coef5;
}

double *butt_2nd_lowpass(uint16_t freq, double gain)
{
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  alfa = sn / (2 * (1 / (pow(2, 0.5))));
  a0 = 1 + alfa;

  coef5[4] = -(-2 * cs) / a0;                            // A1
  coef5[3] = -(1 - alfa) / a0;                           // A2
  coef5[1] = ((1 - cs) / a0) * pow(10, (gain / 20.0));   // B1
  coef5[2] = coef5[1] / 2;                               // B0
  coef5[0] = coef5[0];                                   // B2

  return coef5;
}

double *butt_2nd_highpass(uint16_t freq, double gain)
{
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  alfa = sn / (2 * (1 / (pow(2, 0.5))));
  a0 = 1 + alfa;
  
  coef5[4] = (2 * cs) / a0;                                     // A1
  coef5[3] = -(1 - alfa) / a0;                                  // A2
  coef5[1] = (-(1 + cs) / a0) * pow(10, (gain / 20.0));         // B1
  coef5[2] = -coef5[1] / 2;                                     // B0
  coef5[0] = coef5[2];                                          // B2
  
  return coef5;
}

double *bess_2nd_lowpass(uint16_t freq, int8_t gain)
{
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  alfa = sn / (2 * (1 / (pow(3, 0.5))));
  a0 = 1 + alfa;
  
  coef5[3] = -(-2 * cs) / a0;                            // A1
  coef5[4] = -(1 - alfa) / a0;                           // A2
  coef5[1] = ((1 - cs) / a0) * pow(10, (gain / 20.0));   // B1
  coef5[0] = coef5[1] / 2;                               // B0
  coef5[2] = coef5[0];                                   // B2

  return coef5;
}

double *bess_2nd_highpass(uint16_t freq, int8_t gain)
{
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  alfa = sn / (2 * (1 / (pow(3, 0.5))));
  a0 = 1 + alfa;
  
  coef5[3] = -(-2 * cs) / a0;                            // A1
  coef5[4] = -(1 - alfa) / a0;                           // A2
  coef5[1] = (-(1 + cs) / a0) * pow(10, (gain / 20.0));  // B1
  coef5[0] = -coef5[1] / 2;                              // B0
  coef5[2] = coef5[0];                                   // B2

  return coef5;
}

double *butt_higherorder_lowpass(uint16_t freq, int8_t gain, uint8_t orderindex, uint8_t i)
{
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  orderangle = (Pi / orderindex) * (i + 0.5);
  alfa = sn / (2 * (1 / (2 * sin(orderangle))));
  a0 = 1 + alfa;
  
  coef5[3] = -(-2 * cs) / a0;                            // A1
  coef5[4] = -(1 - alfa) / a0;                           // A2
  coef5[1] = ((1 - cs) / a0) * pow(10, (gain / 20.0));   // B1
  coef5[0] = coef5[1] / 2;                               // B0
  coef5[2] = coef5[0];                                   // B2

  return coef5;
}

double *butt_higherorder_highpass(uint16_t freq, int8_t gain, uint8_t orderindex, uint8_t i)
{
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  orderangle = (Pi / orderindex) * (i + 0.5);
  alfa = sn / (2 * (1 / (2 * sin(orderangle))));
  a0 = 1 + alfa;
  
  coef5[4] = -(-2 * cs) / a0;                                    // A1
  coef5[3] = -(1 - alfa) / a0;                                  // A2
  coef5[1] = (-(1 + cs) / a0) * pow(10, (gain / 20.0));         // B1
  coef5[2] = -coef5[1] / 2;                                     // B0
  coef5[0] = coef5[2];                                          // B2
  
  return coef5;
}

double *parametric(double boost, uint16_t freq, double gain, double q)
{
  if(boost == 0.0)
  {
    gainlin = pow(10.0, (gain / 20.0));
    
    coef5[0] = 0;          // B0
    coef5[1] = 0;                // B1
    coef5[2] = gainlin;                // B2
    coef5[3] = 0;                // A1
    coef5[4] = 0;                // A2
  }                     
  else
  {  
    Ax = pow(10.0, (boost / 40.0));
    omega = ((2 * Pi * freq) / Fs);
    sn = sin(omega);
    cs = cos(omega);
    alfa = sn / (2 * q);    
    a0 = 1 + (alfa / Ax);
    gainlin = (pow(10.0, (gain / 20.0))) / a0;
    
    coef5[4] = -(-2 * cs) / a0;                  // A1
    coef5[3] = -(1 - (alfa / Ax)) / a0;          // A2
    coef5[2] = (1 + (alfa * Ax)) * gainlin;      // B0
    coef5[1] = -(2 * cs) * gainlin;              // B1
    coef5[0] = (1 - (alfa * Ax)) * gainlin;      // B2
  }
  
  return coef5;
}

double *general_lowpass(uint16_t freq, int8_t gain, double q)
{
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  alfa = sn / (2 * q);
  a0 = 1 + alfa;
  gainlin = pow(10, (gain / 20.0)) / a0;
  
  //coef5[3] = (2 * cs) / a0;              // A1
  //coef5[4] = -(1 - alfa) / a0;           // A2
  //coef5[0] = (1 - cs) * gainlin / 2;     // B0
  //coef5[1] = (1 - cs) * gainlin;         // B1
  //coef5[2] = (1 - cs) * gainlin / 2;     // B2
  
  //coef5[3] = (2 * cs) / a0;              // A1
  //coef5[4] = -(1 - alfa) / a0;           // A2
  //coef5[2] = (1 - cs) * gainlin / 2;     // B0
  //coef5[1] = (1 - cs) * gainlin;         // B1
  //coef5[0] = (1 - cs) * gainlin / 2;     // B2
  
  coef5[0] = (1 - cs) * gainlin / 2;     // B2
  coef5[1] = (1 - cs) * gainlin;         // B1
  coef5[2] = (1 - cs) * gainlin / 2;     // B0
  coef5[3] = -(1 - alfa) / a0;           // A2
  coef5[4] = (2 * cs) / a0;              // A1

  return coef5;
}

double *general_highpass(uint16_t freq, int8_t gain, double q)
{
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  alfa = sn / (2 * q);
  a0 = 1 + alfa;
  gainlin = pow(10, (gain / 20.0)) / a0;
  
  //coef5[3] = (2 * cs) / a0;              // A1
  //coef5[4] = -(1 - alfa) / a0;           // A2
  //coef5[0] = (1 + cs) * gainlin / 2;     // B0
  //coef5[1] = -(1 + cs) * gainlin;        // B1
  //coef5[2] = (1 + cs) * gainlin / 2;     // B2
  
  coef5[0] = (1 + cs) * gainlin / 2;     // B2
  coef5[1] = -(1 + cs) * gainlin;        // B1
  coef5[2] = (1 + cs) * gainlin / 2;     // B0
  coef5[3] = -(1 - alfa) / a0;           // A2
  coef5[4] = (2 * cs) / a0;              // A1

  return coef5;
}

double *shelving_lowpass(double boost, uint16_t freq, double gain, double q)
{
  Ax = pow(10, (boost / 40.0));
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  //alfa = (sn / 2) * sqrt(((1 / Ax + Ax) * ( 1 / slope - 1)) + 2);
  alfa = sn / (2 * q);
  sqrtAlfa = 2 * sqrt(Ax) * alfa;
  AxPl = Ax + 1;
  AxMi = Ax - 1;
  a0 = AxPl + AxMi * cs + sqrtAlfa;
  gainlin = pow(10, (gain / 20.0)) / a0;
  
  //coef5[3] = (2 * (AxMi + AxPl * cs)) / a0;                      // A1
  //coef5[4] = -(AxPl + AxMi * cs - sqrtAlfa) / a0;                // A2
  //coef5[0] = (Ax * (AxPl - AxMi * cs + sqrtAlfa)) * gainlin;     // B0
  //coef5[1] = 2 * Ax * (AxMi - AxPl * cs) * gainlin;              // B1
  //coef5[2] = Ax * (AxPl - AxMi * cs - sqrtAlfa) * gainlin;       // B2
  
  coef5[0] = Ax * (AxPl - AxMi * cs - sqrtAlfa) * gainlin;       // B2
  coef5[1] = 2 * Ax * (AxMi - AxPl * cs) * gainlin;              // B1
  coef5[2] = (Ax * (AxPl - AxMi * cs + sqrtAlfa)) * gainlin;     // B0
  coef5[3] = -(AxPl + AxMi * cs - sqrtAlfa) / a0;                // A2
  coef5[4] = (2 * (AxMi + AxPl * cs)) / a0;                      // A1

  return coef5;
}

double *shelving_highpass(double boost, uint16_t freq, double gain, double q)
{
  Ax = pow(10, (boost / 40.0));
  omega = ((2 * Pi * freq) / Fs);
  sn = sin(omega);
  cs = cos(omega); 
  //alfa = (sn / 2) * sqrt(((1 / Ax + Ax) * ( 1 / slope - 1)) + 2);
  alfa = sn / (2 * q);  
  sqrtAlfa = 2 * sqrt(Ax) * alfa;
  AxPl = Ax + 1;
  AxMi = Ax - 1;
  a0 = AxPl - AxMi * cs + sqrtAlfa;
  gainlin = pow(10, (gain / 20.0)) / a0;
  
  //coef5[3] = -(2 * (AxMi - AxPl * cs)) / a0;                     // A1
  //coef5[4] = -(AxPl - AxMi * cs - sqrtAlfa) / a0;                // A2
  //coef5[0] = (Ax * (AxPl + AxMi * cs + sqrtAlfa)) * gainlin;     // B0
  //coef5[1] = -2 * Ax * (AxMi + AxPl * cs) * gainlin;             // B1
  //coef5[2] = Ax * (AxPl + AxMi * cs - sqrtAlfa) * gainlin;       // B2
  
  coef5[0] = Ax * (AxPl + AxMi * cs - sqrtAlfa) * gainlin;       // B2
  coef5[1] = -2 * Ax * (AxMi + AxPl * cs) * gainlin;             // B1
  coef5[2] = (Ax * (AxPl + AxMi * cs + sqrtAlfa)) * gainlin;     // B0
  coef5[3] = -(AxPl - AxMi * cs - sqrtAlfa) / a0;                // A2
  coef5[4] = -(2 * (AxMi - AxPl * cs)) / a0;                     // A1
  
  
  
  

  return coef5;  
}

double *tone_control(int8_t low_boost, int8_t high_boost, uint16_t freq)
{
  Boost_T = pow(10, (high_boost / 20.0));
  Boost_B = pow(10, (low_boost / 20.0));
  
  A_T = A_B = tan((Pi * freq) / Fs);
  
  Knum_T = 2 / (1 + (1 / Boost_T));
  Kden_T = 2 / (1 + Boost_T);
  Knum_B = 2 / (1 + (1 / Boost_B));
  Kden_B = 2 / (1 + Boost_B);
  
  a10 = A_T + Kden_T;
  a11 = A_T - Kden_T;
  
  b10 = A_T + Knum_T;
  b11 = A_T - Knum_T;
  
  a20 = (A_B * Kden_B) + 1;
  a21 = (A_B * Kden_B) - 1;
  
  b20 = (A_B * Knum_B) - 1;
  b21 = (A_B * Knum_B) + 1;
  
  a0 = a10 * a20;

  //gainlin = pow(10, (gain / 20.0));
  
  coef5[3] = -((a10 * a21) + (a11 * a20)) / a0;         // A1
  coef5[4] = -((a11 * a21) / a0);                       // A2
  coef5[0] = (b10 * b20) / a0;                          // B0
  coef5[1] = ((b10 * b21) + (b11 * b20)) / a0;          // B1
  coef5[2] = (b11 * b21) / a0;                          // B2

  return coef5;  
}

double limiter_rms_tc(double millisecond)
{
  double dBps;
  double dspcoef;
  
  dBps = 20000/(millisecond * 2.302585092);
  dspcoef = (1.0 - (pow(10,(dBps/(10*Fs))))) *(-1.0);
  
  return dspcoef;
}

double limiter_hold(double millisecond)
{
  double dspcoef = Fs * (millisecond / 1000);
   
  return dspcoef;
}

double limiter_decay(double millisecond)
{
  double dBps;
  double dspcoef;
  
  dBps = 20000/(millisecond * 2.302585092);
  dspcoef = dBps / (96.0 * Fs);
  
  return dspcoef;
}




