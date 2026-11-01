#ifndef TDS_EC_H
#define TDS_EC_H
void initTDSEC();
void updateTDSEC();
float getEC(); // mS/cm; NAN if invalid
float getTDS(); // ppm (500-scale approximation); NAN if invalid
#endif
