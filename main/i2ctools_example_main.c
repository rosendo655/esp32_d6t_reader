/* i2c-tools example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_fat.h"
#include "d6t_reader.h"
#include "cmd_i2ctools.h"
#include "heatmap_reader.h"
#include "nw_websocket.h"
#include "nw_wifi.h"

#define FPS 5
#define QUEUE_SIZE 40


QueueHandle_t xFrameQueue;

TaskHandle_t xSensorTask;
TaskHandle_t xConsumerTask;
TaskHandle_t xAlertTask;

uint8_t * ssid;
uint8_t * pass;

int init_system(void)
{
   

   ZONE** ini_zones = malloc(sizeof(ZONE) * 4);
   ZONE** fin_zones = malloc(sizeof(ZONE) * 1);

   if(!ini_zones || !fin_zones)
   {
      //err
      return 1;
   }

   ini_zones[0] = hr_create_zone(3, 0, 4, 8);
   ini_zones[1] = hr_create_zone(3, 1, 5, 9);
   ini_zones[2] = hr_create_zone(3, 2, 6, 10);
   ini_zones[3] = hr_create_zone(3, 3, 7, 11);

   fin_zones[0] = hr_create_zone(4, 12, 13, 14, 15);

   ZONE* ref_zone = hr_create_zone(16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);



   HR_PARAMS input_params =
   {
      .min_delta = 0.40f,
      .min_final_temp = 1.0f,
      .heat_transfer_range = 0.20f,
      .min_avg_difference = 0.50f,
      .multiply_factor = 1000,

      .frame_length = 16,
      .window_size = 10,

      .reference_zone = ref_zone,
      
      .init_zones = ini_zones,
      .init_zones_length = 4,

      .final_zones = fin_zones,
      .final_zones_length = 1
   };

   hr_init(input_params);

   wf_init_wifi((uint8_t*)ssid,(uint8_t*)pass);

   ws_init();
   
   xFrameQueue = xQueueCreate(QUEUE_SIZE , sizeof(int*));
   //printf("queue created...\n");
   if (xFrameQueue ==  NULL)
   {
      //err
      printf("queue cannot be created.\n");
      return 1;
   }
   init_sensor();
   //printf("queue exists...\n");
   return 0;
}

void print__temps(int *int_arr_ptr, int len)
{
   for (int i = 0; i < len; i++)
   {
      printf("%d  ", int_arr_ptr[i]);
   }
   printf("\n");
   
}


void read_sensor_task(void *pvArguments)
{
   for (;;)
   {
      //printf("trying to read frame...");
      int *new_frame;
      if(read_sensor(&new_frame))
      {
         //err
         printf("error reading sensor...\n");
      }
      //print__temps(new_frame, 16);
      //free(new_frame);
      uint32_t result = xQueueSend(xFrameQueue, &new_frame, 10);

      vTaskDelay((1000 / FPS ) / portTICK_PERIOD_MS);
   }
}

void consume_sensor_task(void * pvArguments)
{
   for(;;)
   {
      //printf("trying to consume frame...");
      int* next_frame;
      
      if(xQueueReceive(xFrameQueue , &next_frame , 10))
      {
         
         int result = hr_analize(&next_frame);
         //if(result)
         {
            char buff[1024];
            char* buff_ptr = buff;
            int buff_count = 0;

            //hr_cur_data(&buff_ptr , &buff_count);

            hr_cur_frame(&buff_ptr , &buff_count);

            if(result)
            {
               printf("FALL\n");
               buff_count -= 2;
               buff_count += sprintf(buff_ptr,"],fall:%d}",result);
            }

            ws_send(buff , buff_count);

            //printf("%s\n",buff);
            print__temps(next_frame,16);
            //printf("FALL\n");
         }
      }
      else
      {
         //printf("error reading frame from queue...\n");
      }
      
      vTaskDelay((1000 / FPS ) / portTICK_PERIOD_MS);
   }
}

int app_main(void)
{
   //printf("system init ...\n");
   if (init_system())
   {
      //err
      return 1;
   }

   //printf("system init complete...\n");
   xTaskCreatePinnedToCore(
      read_sensor_task,
      "READ_TASK",
      10000,
      NULL,
      1,
      &xSensorTask,
      0
   );
   //printf("read_task_init...\n");
   xTaskCreatePinnedToCore(
      consume_sensor_task,
      "CONSUME_TASK",
      10000,
      NULL,
      1,
      &xConsumerTask,
      1
   );
   printf("end of main...\n");
   // while (true)
   // {
   //    printf("sleeping...\n");
   //    vTaskDelay(1000);
   // }
   return 0;
}
