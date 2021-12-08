#include "heatmap_reader.h"
#include "heatmap_printing.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

HR_PARAMS hr_params;

int **hr_frame_buffer;
int hr_frame_buffer_length;
int hr_frame_buffer_index;

int min_delta;
int min_final_temp;
int min_avg_difference;
float heat_transfer_range;

ZONE *hr_create_zone(int nz_size, ...)
{
    ZONE *new_zone = malloc(sizeof(ZONE));
    new_zone->z_size = nz_size;
    new_zone->z_elements = malloc(sizeof(int) * nz_size);

    va_list arg_list;

    va_start(arg_list, nz_size); /* Initialize the argument list. */

    for (int i = 0; i < nz_size; i++)
        new_zone->z_elements[i] = va_arg(arg_list, int); /* Get the next argument value. */

    va_end(arg_list); /* Clean up. */

    return new_zone;
}

int hr_init(
    HR_PARAMS hr_params_in)
{
    hr_params = hr_params_in;
    hr_frame_buffer_length = hr_params_in.window_size;
    hr_frame_buffer = malloc(sizeof(int *) * hr_frame_buffer_length);

    min_delta = hr_params_in.min_delta * hr_params_in.multiply_factor;
    min_final_temp = hr_params_in.min_final_temp * hr_params_in.multiply_factor;
    min_avg_difference = hr_params_in.min_avg_difference * hr_params_in.multiply_factor;
    heat_transfer_range = hr_params_in.heat_transfer_range;

    d_print_zone_wname("REFERENCE ZONE:", hr_params.reference_zone);

    for (int i = 0; i < hr_params.init_zones_length; i++)
    {
        d_print_zone_wname("INIT ZONES:", hr_params.init_zones[i]);
    }

    for (int i = 0; i < hr_params.final_zones_length; i++)
    {
        d_print_zone_wname("FINAL ZONES:", hr_params.final_zones[i]);
    }

    printf("MIN DELTA %d\n", min_delta);
    printf("MIN FINAL TEMP %d\n", min_final_temp);
    printf("MIN AVG DIFFERENCE %d\n", min_avg_difference);
    printf("HEAT TRANSFER RANGE %f\n", heat_transfer_range);

    if (!hr_frame_buffer)
    {
        return 0;
    }

    hr_frame_buffer_index = 0;
    return 1;
}

int hr_handle_buffer(int *in_frame)
{
    if (hr_frame_buffer_index == hr_frame_buffer_length)
    {
        free(hr_frame_buffer[0]);

        for (int i = 0; i < hr_frame_buffer_length - 1; i++)
        {
            hr_frame_buffer[i] = hr_frame_buffer[i + 1];
        }

        hr_frame_buffer[hr_frame_buffer_length - 1] = in_frame;
    }
    else
    {
        hr_frame_buffer[hr_frame_buffer_index++] = in_frame;
    }
    return 0;
}

void hr_multiply_array(int **array, int ar_len, int multiply_factor)
{
    int *arr = *array;
    for (int i = 0; i < ar_len; i++)
    {
        arr[i] = (arr[i]) * multiply_factor;
    }
}

int hr_get_temp_in_sector_at_frame(int sector, int frame)
{
    // int temp = hr_frame_buffer[frame][sector];
    // printf("%d\n" , temp);
    // return temp;
    return hr_frame_buffer[frame][sector];
}

int hr_get_delta_in_sector(int sector)
{
    return hr_get_temp_in_sector_at_frame(sector, hr_params.window_size - 1) - hr_get_temp_in_sector_at_frame(sector, 0);
}

int hr_get_avg_difference(int sector)
{
    int ac = 0;
    int cur_temp = hr_get_temp_in_sector_at_frame(sector, hr_params.window_size - 1);
    for (int p = 0; p < hr_params.window_size - 1; p++)
    {
        ac += cur_temp - hr_get_temp_in_sector_at_frame(sector, p);
    }
    return ac / (hr_params.window_size - 1);
}

int hr_calc_cells(int **cur_temp, int **delta_temp, int **avg_difference)
{
    *cur_temp = malloc(sizeof(int) * hr_params.frame_length);
    *delta_temp = malloc(sizeof(int) * hr_params.frame_length);
    *avg_difference = malloc(sizeof(int) * hr_params.frame_length);

    // printf("%p\n", *cur_temp);
    // printf("%p\n", *delta_temp);
    // printf("%p\n", *avg_difference);

    if (!(*cur_temp) || !(*delta_temp) || !(*avg_difference))
    {
        //err
        printf("err_calc_cells: cant create pointers\n");
        return 1;
    }

    for (int i = 0; i < hr_params.frame_length; i++)
    {
        (*cur_temp)[i] = hr_get_temp_in_sector_at_frame(i, hr_params.window_size - 1);
    }

    for (int i = 0; i < hr_params.frame_length; i++)
    {
        (*delta_temp)[i] = hr_get_delta_in_sector(i);
    }

    for (int i = 0; i < hr_params.frame_length; i++)
    {
        (*avg_difference)[i] = hr_get_avg_difference(i);
    }

    return 0;
}

int hr_calc_avg_zone(ZONE *in_zone, int *in_cells)
{
    int ac = 0;
    for (uint32_t i = 0; i < in_zone->z_size; i++)
    {
        ac += in_cells[in_zone->z_elements[i]];
    }
    return ac / in_zone->z_size;
}

int hr_calc_zones_metrics(
    //temps, avgs, and differences
    int *cur_temp,
    int *delta_temp,
    int *avg_difference,
    //output variables
    //reference zone
    int *ref_avg,
    int *ref_delta_avg,
    int *ref_avg_difference,
    //init zones
    int **init_avgs,
    int **init_delta_avgs,
    int **init_avgs_difference,
    //final zones
    int **final_avgs,
    int **final_delta_avgs,
    int **final_avgs_difference)
{
    *init_avgs = malloc(sizeof(int) * hr_params.init_zones_length);
    *init_delta_avgs = malloc(sizeof(int) * hr_params.init_zones_length);
    *init_avgs_difference = malloc(sizeof(int) * hr_params.init_zones_length);

    *final_avgs = malloc(sizeof(int) * hr_params.final_zones_length);
    *final_delta_avgs = malloc(sizeof(int) * hr_params.final_zones_length);
    *final_avgs_difference = malloc(sizeof(int) * hr_params.final_zones_length);

    if (!(*init_avgs) || !(*init_delta_avgs) || !(*init_avgs_difference) || !(*final_avgs) || !(*final_delta_avgs) || !(*final_avgs_difference))
    {
        //err
        return 1;
    }

    *ref_avg = hr_calc_avg_zone(hr_params.reference_zone, cur_temp);
    *ref_delta_avg = hr_calc_avg_zone(hr_params.reference_zone, delta_temp);
    *ref_avg_difference = hr_calc_avg_zone(hr_params.reference_zone, avg_difference);

    for (int ini = 0; ini < hr_params.init_zones_length; ini++)
    {
        (*init_avgs)[ini] = hr_calc_avg_zone(hr_params.init_zones[ini], cur_temp);
        (*init_delta_avgs)[ini] = hr_calc_avg_zone(hr_params.init_zones[ini], delta_temp);
        (*init_avgs_difference)[ini] = hr_calc_avg_zone(hr_params.init_zones[ini], avg_difference);
    }

    for (int fin = 0; fin < hr_params.final_zones_length; fin++)
    {
        (*final_avgs)[fin] = hr_calc_avg_zone(hr_params.final_zones[fin], cur_temp);
        (*final_delta_avgs)[fin] = hr_calc_avg_zone(hr_params.final_zones[fin], delta_temp);
        (*final_avgs_difference)[fin] = hr_calc_avg_zone(hr_params.final_zones[fin], avg_difference);
    }

    return 0;
}

int hr_calc_single_trans(
    //reference zone
    int ref_avg,
    int ref_delta_avg,
    int ref_avg_difference,
    //init zones
    int init_avg,
    int init_delta_avg,
    int init_avg_difference,
    //final zones
    int final_avg,
    int final_delta_avg,
    int final_avg_difference)
{

    //evaluate if final zone is warmer
    if (!(init_avg < (ref_avg + min_final_temp) && (ref_avg + min_final_temp) < final_avg))
    {
        //condition doesnt meet
        return 0;
    }

    // evaluate if init zone is colder than -min_delta and final zone is warmer than min delta
    if (!(init_delta_avg < (-min_delta) && min_delta < final_delta_avg))
    {
        //condition doesnt meet
        return 0;
    }

    //evaluate if init delta is lower than reference delta and reference delta is lower than final delta
    if (!(init_delta_avg < ref_delta_avg && ref_delta_avg < final_delta_avg))
    {
        //condition doesnt meet
        return 0;
    }

    // evaluates if heat goes from init to final zone and not the other way
    float heat_transfer_percentage = (float)init_delta_avg / (final_delta_avg == 0 ? 0.0001f : (float)final_delta_avg);
    if (!(heat_transfer_percentage < 0))
    {
        //condition doesnt meet
        return 0;
    }

    heat_transfer_percentage *= -1;
    //heat transfer percentage is now positive
    //evaluates if transfer percentage is between  (1 - range) <---> (1 + range)
    if (!((1.0f - heat_transfer_range) <= heat_transfer_percentage && heat_transfer_percentage <= (1.0f + heat_transfer_range)))
    {
        //condition doesnt meet
        return 0;
    }

    //all conditions have met
    return 1;
}

int hr_calc_trans_vector(
    int **out_vector,
    //reference zone
    int ref_avg,
    int ref_delta_avg,
    int ref_avg_difference,
    //init zones
    int *init_avgs,
    int *init_delta_avgs,
    int *init_avgs_difference,
    //final zones
    int *final_avgs,
    int *final_delta_avgs,
    int *final_avgs_difference)
{
    *out_vector = malloc(sizeof(int) * (hr_params.init_zones_length * hr_params.final_zones_length));

    if (!(*out_vector))
    {
        //err
        return 1;
    }

    int o_pos = 0;
    for (int init_z = 0; init_z < hr_params.init_zones_length; init_z++)
    {
        for (int fin_z = 0; fin_z < hr_params.final_zones_length; fin_z++)
        {
            (*out_vector)[o_pos++] = hr_calc_single_trans(
                ref_avg,
                ref_delta_avg,
                ref_avg_difference,
                init_avgs[init_z],
                init_delta_avgs[init_z],
                init_avgs_difference[init_z],
                final_avgs[fin_z],
                final_delta_avgs[fin_z],
                final_avgs_difference[fin_z]);
        }
    }
    return 0;
}

int hr_any(int vec_size, int *in_vec)
{
    for (int it = 0; it < vec_size; it++)
    {
        if (in_vec[it])
        {
            return 1;
        }
    }
    return 0;
}

int hr_sprintf(
    char **buffer,
    int *buffer_count,

    int *new_frame,
    //reference zone
    int ref_avg,
    int ref_delta_avg,
    int ref_avg_difference,
    //init zones
    int *init_avgs,
    int *init_delta_avgs,
    int *init_avgs_difference,
    //final zones
    int *final_avgs,
    int *final_delta_avgs,
    int *final_avgs_difference,

    int *vec_trans,
    int any_trans)
{
    char *original_pos = *buffer;
    *buffer_count += sprintf(*buffer, "{'frame':[");
    *buffer = strchr(*buffer, 0);
    for (int i = 0; i < hr_params.frame_length; i++)
    {
        *buffer_count += sprintf(*buffer, "%d,", new_frame[i]);
        *buffer = strchr(*buffer, 0);
    }
    *buffer = *buffer - 1;
    *buffer_count -= 1;

    *buffer_count += sprintf(*buffer, "],");
    *buffer = strchr(*buffer, 0);

    *buffer_count += sprintf(*buffer, "'any':%d", any_trans);
    *buffer = strchr(*buffer, 0);

    *buffer_count += sprintf(*buffer, "}");
    return 0;
}

int hr_cur_frame(char **buffer, int *buffer_count)
{
    char *original_pos = *buffer;
    *buffer_count += sprintf(*buffer, "{'frame':[");
    *buffer = strchr(*buffer, 0);

    *buffer = strchr(*buffer, 0);
    for (int sec = 0; sec < hr_params.frame_length; sec++)
    {
        int cur_temp_data = hr_get_temp_in_sector_at_frame(sec, hr_params.window_size - 1);
        float cur_temp_fl = (float)cur_temp_data / (float)(hr_params.multiply_factor * 10);

        *buffer_count += sprintf(*buffer, "%.1f,", cur_temp_fl);
        *buffer = strchr(*buffer, 0);
    }
    *buffer = *buffer - 1;
    *buffer_count -= 1;
    *buffer_count += sprintf(*buffer, "]}");
    //*buffer = strchr(*buffer, 0);

    return 0;
}

int hr_cur_data(char **buffer, int *buffer_count)
{
    char *original_pos = *buffer;
    *buffer_count += sprintf(*buffer, "{'framebuffer':[");
    *buffer = strchr(*buffer, 0);

    for (int fr = 0; fr < hr_params.window_size; fr++)
    {
        *buffer_count += sprintf(*buffer, "[");
        *buffer = strchr(*buffer, 0);
        for (int sec = 0; sec < hr_params.frame_length; sec++)
        {
            int cur_temp_data = hr_get_temp_in_sector_at_frame(sec, fr);
            float cur_temp_fl = (float)cur_temp_data / (float)(hr_params.multiply_factor * 10);

            *buffer_count += sprintf(*buffer, "%.1f,", cur_temp_fl);
            *buffer = strchr(*buffer, 0);
        }
        *buffer = *buffer - 1;
        *buffer_count -= 1;
        *buffer_count += sprintf(*buffer, "],");
        *buffer = strchr(*buffer, 0);
    }
    *buffer = *buffer - 1;
    *buffer_count -= 1;

    *buffer_count += sprintf(*buffer, "]");
    *buffer = strchr(*buffer, 0);

    *buffer_count += sprintf(*buffer, "}");
    return 0;
}

int hr_analize(int **new_frame)
{

    hr_multiply_array(new_frame, hr_params.frame_length, hr_params.multiply_factor);
    hr_handle_buffer(*new_frame);

    //d_print_array_wname("NEW FRAME:", *new_frame , 16);
    if (hr_frame_buffer_index != hr_frame_buffer_length)
    {
        return 0;
    }

    int *cur_temp, *delta_temp, *avg_difference;

    if (hr_calc_cells(&cur_temp, &delta_temp, &avg_difference))
    {
        //err
        printf("err_calc_cells\n");
        return 1;
    }

    //reference zone metrics
    int ref_avg, ref_delta_avg, ref_avg_difference;
    //init zones metrics
    int *init_avgs, *init_delta_avgs, *init_avgs_difference;
    //final zones metrics
    int *final_avgs, *final_delta_avgs, *final_avgs_difference;

    int calc_result = hr_calc_zones_metrics(
        cur_temp,
        delta_temp,
        avg_difference,

        &ref_avg,
        &ref_delta_avg,
        &ref_avg_difference,

        &init_avgs,
        &init_delta_avgs,
        &init_avgs_difference,

        &final_avgs,
        &final_delta_avgs,
        &final_avgs_difference);

    if (calc_result)
    {
        printf("err calc zone metrics\n");
        return 1;
    }

    int *trans_vector;

    int trans_calc_result = hr_calc_trans_vector(
        //out result
        &trans_vector,

        ref_avg,
        ref_delta_avg,
        ref_avg_difference,

        init_avgs,
        init_delta_avgs,
        init_avgs_difference,

        final_avgs,
        final_delta_avgs,
        final_avgs_difference);

    if (trans_calc_result)
    {
        printf("err calc trans vector\n");
        return 1;
    }

    int any_trans = hr_any(hr_params.init_zones_length * hr_params.final_zones_length, trans_vector);

    // hr_sprintf(
    // buffer,
    // buffer_count,

    // *new_frame,
    // //reference zone
    // ref_avg,
    // ref_delta_avg,
    // ref_avg_difference,
    // //init zones
    // init_avgs,
    // init_delta_avgs,
    // init_avgs_difference,
    // //final zones
    // final_avgs,
    // final_delta_avgs,
    // final_avgs_difference,

    // trans_vector,
    // any_trans
    // );

    free(cur_temp);
    free(delta_temp);
    free(avg_difference);
    free(init_avgs);
    free(init_delta_avgs);
    free(init_avgs_difference);
    free(final_avgs);
    free(final_delta_avgs);
    free(final_avgs_difference);
    free(trans_vector);

    return any_trans;
}
