#pragma once

#ifdef __cplusplus
extern "C" {
#endif


int init_sensor(void);
int read_sensor(int** data_array_ptr);


#ifdef __cplusplus
}
#endif
