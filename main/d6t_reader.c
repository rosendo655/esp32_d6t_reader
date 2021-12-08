#include "cmd_i2ctools.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t d6t_sensor_address = 0x0a;
uint8_t d6t_sensor_command = 0x4c;
uint8_t d6t_byte_count = 35;
uint8_t d6t_sector_count = 16;


int init_sensor(void)
{
    init_i2c(21,22);
    return 0;
}

int read_sensor(int** data_array_ptr)
{
    uint8_t * raw_data_array ;

    if(read_i2c(d6t_sensor_address,d6t_sensor_command,d6t_byte_count , &raw_data_array ))
    {
        //err
        printf("error on i2c reading...\n");
        return 1;
    }

    //int  t_PTAT = (raw_data_array[0]+(raw_data_array[1]<<8));
    *data_array_ptr = malloc(d6t_sector_count * sizeof(int));
    if(*data_array_ptr == 0)
    {
        //err
        printf("cant create data array...\n");
        return 1;
    }

    // for (int i = 0; i < d6t_sector_count; i++) {
    //     printf("%d " ,(int)(raw_data_array[(i*2+2)]+(raw_data_array[(i*2+3)]<<8)));
    // }
    // printf("\n");

    for (int i = 0; i < d6t_sector_count; i++) {
        (*data_array_ptr)[i] = (int)(raw_data_array[(i*2+2)]+(raw_data_array[(i*2+3)]<<8));
    }

    free(raw_data_array);    
    return 0;
}