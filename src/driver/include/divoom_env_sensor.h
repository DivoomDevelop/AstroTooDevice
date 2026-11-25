#ifndef __divoom_env_sensor_h_
#define __divoom_env_sensor_h_

typedef struct __divoom_env_sensor_info_
{
    float temperature;
    float humidity;
}divoom_env_sensor_info,*Pdivoom_env_sensor_info;

void divoom_env_senoser_init(void);


#endif

