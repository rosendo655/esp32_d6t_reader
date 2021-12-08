/* cmd_i2ctools.h

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void register_i2ctools(void);
void init_i2c(int,int);
int read_i2c(uint8_t i2c_address , uint8_t comm, uint8_t data_len , uint8_t ** byte_data_ptr);

#ifdef __cplusplus
}
#endif
