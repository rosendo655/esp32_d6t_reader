#include <stdio.h>
#include <stdlib.h>

typedef struct hr_zone
{
    int z_size;
    int* z_elements;
} ZONE ;

typedef struct hr_init_params
{
    float min_delta;
    float min_final_temp;
    float min_avg_difference;
    float heat_transfer_range;

    int multiply_factor;

    int frame_length;
    int window_size;

    ZONE* reference_zone;

    ZONE** init_zones;
    int  init_zones_length;

    ZONE** final_zones;
    int  final_zones_length;
}HR_PARAMS ;

ZONE * hr_create_zone(int nz_size , ...);


int hr_init
(
   HR_PARAMS hr_params_in
);

int hr_analize(int **new_frame);
int hr_cur_data(char** buffer , int* buffer_count);
int hr_cur_frame(char **buffer, int *buffer_count);