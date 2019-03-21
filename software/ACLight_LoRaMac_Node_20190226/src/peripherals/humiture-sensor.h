#ifndef _TEMP_HUMI_SENSOR_H_
#define _TEMP_HUMI_SENSOR_H_

typedef struct 
{
  float temp;
  float humi;
}Humiture_t;

void HumitureSensorInit(void);
void ReadHumiture(void);

#endif
