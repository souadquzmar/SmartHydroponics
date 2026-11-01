#ifndef PH_SENSOR_H
#define PH_SENSOR_H
void initPHSensor();
void updatePHSensor();
float getPH(); // NAN until calibrated and valid
#endif
