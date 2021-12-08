#include "heatmap_reader.h"

void d_print_zone(ZONE *in_zone);
void d_print_array(int* array , int size);
void d_print_zone_wname(const char* name , ZONE* in_zone);
void d_print_array_wname(const char* name , int* array , int size );

void d_print_zone_wname(const char* name , ZONE* in_zone)
{
    printf("%s " , name);
    d_print_zone(in_zone);
    printf("\n");
}

void d_print_zone(ZONE *in_zone)
{
    printf("SIZE: %d , ELEMENTS: (", in_zone->z_size);
    d_print_array(in_zone->z_elements , in_zone->z_size );
    printf(")\n");
}

void d_print_array_wname(const char* name , int* array , int size )
{
    printf("%s " , name);
    d_print_array(array , size);
    printf("\n");
}

void d_print_array(int* array , int size)
{
    for(int i = 0 ; i < size ; i ++)
    {
        printf("%d, ", array[i]);
    }
}