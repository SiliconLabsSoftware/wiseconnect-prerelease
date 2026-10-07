/***************************************************************************/ /**
* @file sensorhub_app.c
* @brief sensorhub_app example
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 *    If you use this software In in a product,
 *    an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
/***************************************************************************
 * @brief : This file perform the sensor hub application related operations
 * like intilize ,scanning the sensors,and create/start the sensor HUB
 ******************************************************************************/

// Include files
#include "rsi_ccp_user_config.h"
#include "sensor_hub.h"
#include <inttypes.h>
#include "rsi_os.h"
#include "rsi_debug.h"
#include "cmsis_os2.h"
#include "rsi_pll.h"
#include "rsi_power_save.h"
#include "rsi_rom_clks.h"
#include "sl_wifi.h"
#include "sl_si91x_driver.h"
#include <string.h>

/*******************************************************************************
 **************  Sensor app Task Attributes structure for thread   *************
 ******************************************************************************/
#define SL_APP_TASK_STACK_SIZE 4096
static const osThreadAttr_t app_thread_attributes = {
  .name       = "SensorHub_App",        // Name of thread
  .stack_size = SL_APP_TASK_STACK_SIZE, // Stack size of sensor_app task
  .priority   = osPriorityLow,          // Priority of Sensor task
};

#if SH_AWS_ENABLE
static const osThreadAttr_t aws_thread_attributes = {
  .name       = "AWS_App",
  .stack_size = 3072,
  .priority   = osPriorityLow3,
};
#endif

rsi_task_handle_t app_task_handle = NULL;

#if SH_AWS_ENABLE
osSemaphoreId_t sl_semaphore_app_task_id_2;

char mqtt_publish_payload[500];

void sl_si91x_aws_task(void);

// Convert unsigned integer to null-terminated decimal string (no snprintf).
static void u32_to_str(uint32_t val, char *str)
{
  char tmp[10];
  int16_t ii = 0;
  int16_t jj = 0;

  if (val == 0U) {
    str[0] = '0';
    str[1] = '\0';
    return;
  }

  while (val != 0U) {
    tmp[ii] = (char)('0' + (val % 10U));
    val /= 10U;
    ii++;
  }

  for (jj = 0, ii--; ii >= 0; ii--, jj++) {
    str[jj] = tmp[ii];
  }
  str[jj] = '\0';
}

// Append src into mqtt_publish_payload without exceeding the buffer.
static void mqtt_append_str(const char *src)
{
  size_t used;
  size_t avail;
  size_t copy_len;

  if (src == NULL) {
    return;
  }

  used = strlen(mqtt_publish_payload);
  if (used >= (sizeof(mqtt_publish_payload) - 1U)) {
    return;
  }

  avail    = sizeof(mqtt_publish_payload) - 1U - used;
  copy_len = strlen(src);
  if (copy_len > avail) {
    copy_len = avail;
  }

  if (copy_len > 0U) {
    memcpy(&mqtt_publish_payload[used], src, copy_len);
    mqtt_publish_payload[used + copy_len] = '\0';
  }
}

// Append label + signed value + suffix into mqtt_publish_payload (no snprintf).
static void mqtt_append_value(const char *label, int32_t val, const char *suffix)
{
  char num[12];

  if (label != NULL) {
    mqtt_append_str(label);
  }
  if (val < 0) {
    mqtt_append_str("-");
    u32_to_str((uint32_t)(-(uint32_t)val), num);
  } else {
    u32_to_str((uint32_t)val, num);
  }
  mqtt_append_str(num);
  if (suffix != NULL) {
    mqtt_append_str(suffix);
  }
}

static void mqtt_append_indexed_value(const char *prefix,
                                      uint32_t idx,
                                      const char *mid,
                                      int32_t val,
                                      const char *suffix)
{
  char num[12];

  mqtt_append_str(prefix);
  u32_to_str(idx, num);
  mqtt_append_str(num);
  mqtt_append_value(mid, val, suffix);
}

static void mqtt_append_indexed_cid_value(const char *prefix,
                                          uint32_t idx,
                                          uint32_t cid,
                                          int32_t val,
                                          const char *suffix)
{
  char num[12];

  mqtt_append_str(prefix);
  u32_to_str(idx, num);
  mqtt_append_str(num);
  mqtt_append_str(": C_ID[");
  u32_to_str(cid, num);
  mqtt_append_str(num);
  mqtt_append_str("] ");
  mqtt_append_value(NULL, val, suffix);
}
#endif
/*******************************************************************************
 ********************  Extern variables/structures   ***************************
 ******************************************************************************/
extern sl_sensor_info_t sensor_hub_info_t[SL_MAX_NUM_SENSORS]; // Sensor info structure
extern sl_bus_intf_config_t bus_intf_info;                     //< Bus interface configuration structure
extern osSemaphoreId_t sl_semaphore_aws_task_id;
/*******************************************************************************
 ********************** Local/global variables  *******************************
 ******************************************************************************/
uint32_t event_ack              = 0; // Sensor event acknowledge
static uint32_t sensor_scan_cnt = 0; // Sensor scan count

#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

/*******************************************************************************
 ************************* Local functions  ************************************
 ******************************************************************************/
void sl_si91x_sensorhub_app_task(void);                               // application task
void sensorhub_app_init(void);                                        // application initialization
void sl_si91x_sensor_event_handler(uint8_t sensor_id, uint8_t event); // application event handler
#ifndef SH_AWS_ENABLE
static sl_status_t initialize_wireless(void);
void wireless_sleep(void);
#endif
/**************************************************************************/ /**
 * @fn           void gy61_adc_raw_data_map()
 * @brief        Map the raw input data of adc gy61 to output range
 *
 * @param[in]    x        :Sensor raw data
 * @param[in]    in_min   :Sensor raw min
 * @param[in]    in_max   :Sensor raw max
 * @param[in]    out_min  :Sensor output min
 * @param[in]    out_max  :Sensor output max
 * @param[out]   sensor g output
 *
******************************************************************************/
#ifdef GY61_ADC_SENSOR
static long gy61_adc_raw_data_map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
#endif
/**************************************************************************/ /**
 * @fn           void sl_si91x_sensor_event_handler()
 * @brief        This Sensor event handle to the Sensor HUB.
 *
 * @param[in]    sensor_id        :Id of the Sensor
 * @param[in]    events           :Sensor hub event
 * @param[out]   NULL
 *
******************************************************************************/
void sl_si91x_sensor_event_handler(uint8_t sensor_id, uint8_t event)
{
#if SH_AWS_ENABLE
  osStatus_t sl_sem_status;

  strcpy(mqtt_publish_payload, "");
#endif

  uint8_t sens_ind;
  for (sens_ind = 0; sens_ind < SL_MAX_NUM_SENSORS; sens_ind++) {
    if (sensor_hub_info_t[sens_ind].sensor_id == sensor_id) {
      break;
    }
  }

  if (SL_MAX_NUM_SENSORS == sens_ind) {
    // Failures use SL_PRINT_STRING_ERROR; success/status/data use SL_PRINT_STRING_ERROR.
    SL_PRINT_STRING_ERROR("Sensor not Found!");
    return;
  }

  switch (event) {
    case SL_SENSOR_CREATION_FAILED:
      SL_PRINT_STRING_ERROR("%s Not created\r\n", (uintptr_t)sensor_hub_info_t[sens_ind].sensor_name);
      break;
    case SL_SENSOR_STARTED:
      SL_PRINT_STRING_ERROR("Sensor Started:%u \r\n", sensor_id);
      break;
    case SL_SENSOR_STOPPED:
      SL_PRINT_STRING_ERROR("Sensor Stopped:%u \r\n", sensor_id);
      break;
    case SL_SENSOR_DATA_READY:
      SL_PRINT_STRING_ERROR("\r\n %s ", (uintptr_t)sensor_hub_info_t[sens_ind].sensor_name);
      SL_PRINT_STRING_ERROR("\r\n Sensor data Ready:%u \t", sensor_id);

      if (SL_SENSOR_ADXL345_ID == sensor_id) {
        if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_INTERRUPT_MODE) {
          SL_PRINT_STRING_ERROR(
            "Axis X = %ld mg, \t",
            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.x) * 1000.0f));
          SL_PRINT_STRING_ERROR(
            "Axis Y = %ld mg, \t",
            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.y) * 1000.0f));
          SL_PRINT_STRING_ERROR(
            "Axis Z = %ld mg \t\n ",
            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.z) * 1000.0f));
#if SH_AWS_ENABLE
          mqtt_append_value(
            "SL_SENSOR_ADXL345_ID_x: ",
            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.x) * 1000.0f),
            "mg    ");
          mqtt_append_value(
            "SL_SENSOR_ADXL345_ID_y: ",
            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.y) * 1000.0f),
            "mg    ");
          mqtt_append_value(
            "SL_SENSOR_ADXL345_ID_z: ",
            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.z) * 1000.0f),
            "mg    ");
#endif
        } else if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_POLLING_MODE) {
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_THRESHOLD) {
            SL_PRINT_STRING_ERROR(
              "\r\n Axis X = %ld mg, \t",
              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.x) * 1000.0f));
            SL_PRINT_STRING_ERROR(
              "\r\n Axis Y = %ld mg, \t",
              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.y) * 1000.0f));
            SL_PRINT_STRING_ERROR(
              "\r\n Axis Z = %ld mg \t\r\n ",
              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.z) * 1000.0f));
#if SH_AWS_ENABLE
            mqtt_append_value(
              "SL_SENSOR_ADXL345_ID_x: ",
              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.x) * 1000.0f),
              "mg    ");
            mqtt_append_value(
              "SL_SENSOR_ADXL345_ID_y: ",
              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.y) * 1000.0f),
              "mg    ");
            mqtt_append_value(
              "SL_SENSOR_ADXL345_ID_z: ",
              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].accelerometer.z) * 1000.0f),
              "mg    ");
#endif
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_TIMEOUT) {
            for (uint32_t i = 0;
                 i < sensor_hub_info_t[sens_ind].data_deliver.timeout / sensor_hub_info_t[sens_ind].sampling_interval;
                 i++) {
              SL_PRINT_STRING_ERROR(
                "\r\n Axis X = %ld mg, \t",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.x) * 1000.0f));
              SL_PRINT_STRING_ERROR(
                "\r\n Axis Y = %ld mg, \t",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.y) * 1000.0f));
              SL_PRINT_STRING_ERROR(
                "\r\n Axis Z = %ld mg \r\n ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.z) * 1000.0f));
#if SH_AWS_ENABLE
              mqtt_append_indexed_value(
                "SL_SENSOR_ADXL345_ID_",
                (uint32_t)((i + 1)),
                "_x: ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.x) * 1000.0f),
                "mg    ");
              mqtt_append_indexed_value(
                "SL_SENSOR_ADXL345_ID_",
                (uint32_t)((i + 1)),
                "_y: ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.y) * 1000.0f),
                "mg    ");
              mqtt_append_indexed_value(
                "SL_SENSOR_ADXL345_ID_",
                (uint32_t)((i + 1)),
                "_z: ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.z) * 1000.0f),
                "mg    ");
#endif
            }
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_NUM_OF_SAMPLES) {
            for (uint32_t i = 0; i < sensor_hub_info_t[sens_ind].data_deliver.numofsamples; i++) {
              SL_PRINT_STRING_ERROR(
                "\r\n Axis:- X = %ld mg, \t",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.x) * 1000.0f));
              SL_PRINT_STRING_ERROR(
                "\r\n Y = %ld mg, \t",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.y) * 1000.0f));
              SL_PRINT_STRING_ERROR(
                "\r\n Z = %ld mg  \t",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.z) * 1000.0f));
#if SH_AWS_ENABLE
              mqtt_append_indexed_value(
                "SL_SENSOR_ADXL345_ID_",
                (uint32_t)((i + 1)),
                "_x: ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.x) * 1000.0f),
                "mg    ");
              mqtt_append_indexed_value(
                "SL_SENSOR_ADXL345_ID_",
                (uint32_t)((i + 1)),
                "_y: ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.y) * 1000.0f),
                "mg    ");
              mqtt_append_indexed_value(
                "SL_SENSOR_ADXL345_ID_",
                (uint32_t)((i + 1)),
                "_z: ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].accelerometer.z) * 1000.0f),
                "mg    ");
#endif
            }
          }
        }
      }
      if (SL_SENSOR_APDS9960_ID == sensor_id) {
        if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_INTERRUPT_MODE) {
          SL_PRINT_STRING_ERROR("\r\n R = %ld, \t",
                                (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.r));
          SL_PRINT_STRING_ERROR("G = %ld, \t",
                                (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.g));
          SL_PRINT_STRING_ERROR("B = %ld \t\n ",
                                (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.b));
          SL_PRINT_STRING_ERROR("Proximity = %ld ;\t\n",
                                (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.proximity));
#if SH_AWS_ENABLE
          mqtt_append_value("SL_SENSOR_APDS9960_ID_r: ",
                            (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.r),
                            "    ");
          mqtt_append_value("SL_SENSOR_APDS9960_ID_g: ",
                            (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.g),
                            "    ");
          mqtt_append_value("SL_SENSOR_APDS9960_ID_b: ",
                            (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.b),
                            "    ");
          mqtt_append_value("SL_SENSOR_APDS9960_ID_prox: ",
                            (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.proximity),
                            "    ");
#endif

        } else if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_POLLING_MODE) {
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_THRESHOLD) {
            SL_PRINT_STRING_ERROR("\r\n R = %ld, \t",
                                  (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.r));
            SL_PRINT_STRING_ERROR("\r\n G = %ld, \t",
                                  (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.g));
            SL_PRINT_STRING_ERROR("\r\n B = %ld \t\n ",
                                  (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.b));
            SL_PRINT_STRING_ERROR(
              "\r\n Proximity = %ld ;\t\n",
              (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.proximity));
            //  SL_PRINT_STRING_ERROR("Gesture = %c \t\n",
            //  (char)sensor_hub_info_t[sens_ind].sens_data_ptr->sensor_data[0].gesture);
#if SH_AWS_ENABLE
            mqtt_append_value("SL_SENSOR_APDS9960_ID_r: ",
                              (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.r),
                              "    ");
            mqtt_append_value("SL_SENSOR_APDS9960_ID_g: ",
                              (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.g),
                              "    ");
            mqtt_append_value("SL_SENSOR_APDS9960_ID_b: ",
                              (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.b),
                              "    ");
            mqtt_append_value("SL_SENSOR_APDS9960_ID_prox: ",
                              (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].rgbw.proximity),
                              "    ");
#endif
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_TIMEOUT) {
            for (uint32_t i = 0;
                 i < sensor_hub_info_t[sens_ind].data_deliver.timeout / sensor_hub_info_t[sens_ind].sampling_interval;
                 i++) {
              SL_PRINT_STRING_ERROR(
                "\r\n Proximity = %ld ;\t",
                (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.proximity));
              SL_PRINT_STRING_ERROR("\r\n R = %ld, \t",
                                    (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.r));
              SL_PRINT_STRING_ERROR("\r\n G = %ld, \t",
                                    (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.g));
              SL_PRINT_STRING_ERROR("\r\n B = %ld \t\n ",
                                    (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.b));
              // DEBUGOUT("Gesture = %c \t\n", (char)sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].gesture);
#if SH_AWS_ENABLE
              mqtt_append_indexed_value("SL_SENSOR_APDS9960_ID_",
                                        (uint32_t)((i + 1)),
                                        "_r: ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.r),
                                        "    ");
              mqtt_append_indexed_value("SL_SENSOR_APDS9960_ID_",
                                        (uint32_t)((i + 1)),
                                        "_g: ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.g),
                                        "    ");
              mqtt_append_indexed_value("SL_SENSOR_APDS9960_ID_",
                                        (uint32_t)((i + 1)),
                                        "_b: ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.b),
                                        "    ");
              mqtt_append_indexed_value(
                "SL_SENSOR_APDS9960_ID_",
                (uint32_t)((i + 1)),
                "_prox: ",
                (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.proximity),
                "    ");
#endif
            }
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_NUM_OF_SAMPLES) {
            for (uint32_t i = 0; i < sensor_hub_info_t[sens_ind].data_deliver.numofsamples; i++) {
              SL_PRINT_STRING_ERROR("\r\n R = %ld, \t",
                                    (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.r));
              SL_PRINT_STRING_ERROR("\r\n G = %ld, \t",
                                    (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.g));
              SL_PRINT_STRING_ERROR("\r\n B = %ld \t; ",
                                    (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.b));
              SL_PRINT_STRING_ERROR(
                "\r\n Proximity = %ld ",
                (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.proximity));
#if SH_AWS_ENABLE
              mqtt_append_indexed_value("SL_SENSOR_APDS9960_ID_",
                                        (uint32_t)((i + 1)),
                                        "_r: ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.r),
                                        "    ");
              mqtt_append_indexed_value("SL_SENSOR_APDS9960_ID_",
                                        (uint32_t)((i + 1)),
                                        "_g: ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.g),
                                        "    ");
              mqtt_append_indexed_value("SL_SENSOR_APDS9960_ID_",
                                        (uint32_t)((i + 1)),
                                        "_b: ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.b),
                                        "    ");
              mqtt_append_indexed_value(
                "SL_SENSOR_APDS9960_ID_",
                (uint32_t)((i + 1)),
                "_prox: ",
                (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].rgbw.proximity),
                "    ");
#endif
            }
          }
        }
      }

      if (SL_SENSOR_LM75_ID == sensor_id) {
        if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_INTERRUPT_MODE) {
          SL_PRINT_STRING_ERROR(
            "\r\n %ld m°C \t",
            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].temperature) * 1000.0f));
#if SH_AWS_ENABLE
          mqtt_append_value(
            "SL_SENSOR_LM75_ID: ",
            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].temperature) * 1000.0f),
            " m°C    ");
#endif
        } else if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_POLLING_MODE) {
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_TIMEOUT) {
            for (uint32_t i = 0;
                 i < sensor_hub_info_t[sens_ind].data_deliver.timeout / sensor_hub_info_t[sens_ind].sampling_interval;
                 i++) {
              SL_PRINT_STRING_ERROR(
                "\r\n %ld m°C \t ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].temperature) * 1000.0f));
#if SH_AWS_ENABLE
              mqtt_append_indexed_value(
                "SL_SENSOR_LM75_ID_",
                (uint32_t)((i + 1)),
                ": ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].temperature) * 1000.0f),
                " m°C    ");
#endif
            }
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_NUM_OF_SAMPLES) {
            for (uint32_t i = 0; i < sensor_hub_info_t[sens_ind].data_deliver.numofsamples; i++) {
              SL_PRINT_STRING_ERROR(
                "\r\n %ld m°C \t",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].temperature) * 1000.0f));
#if SH_AWS_ENABLE
              mqtt_append_indexed_value(
                "SL_SENSOR_LM75_ID_",
                (uint32_t)((i + 1)),
                ": ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].temperature) * 1000.0f),
                " m°C    ");
#endif
            }
          }

          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_THRESHOLD) {
            SL_PRINT_STRING_ERROR(
              "\r\n %ld m°C \t",
              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].temperature) * 1000.0f));
#if SH_AWS_ENABLE
            mqtt_append_value(
              "SL_SENSOR_LM75_ID: ",
              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].temperature) * 1000.0f),
              " m°C    ");
#endif
          }
        }
      }

      if (SL_SENSOR_BH1750_ID == sensor_id) {
        if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_INTERRUPT_MODE) {
          SL_PRINT_STRING_ERROR(
            "\r\n %ld mlx \t",
            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].light) * 1000.0f));
#if SH_AWS_ENABLE
          mqtt_append_value("SL_SENSOR_BH1750_ID: ",
                            (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].light) * 1000.0f),
                            " mlx    ");
#endif
        } else if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_POLLING_MODE) {
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_TIMEOUT) {
            for (uint32_t i = 0;
                 i < sensor_hub_info_t[sens_ind].data_deliver.timeout / sensor_hub_info_t[sens_ind].sampling_interval;
                 i++) {
              SL_PRINT_STRING_ERROR(
                "\r\n %ld mlx \t",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].light) * 1000.0f));
#if SH_AWS_ENABLE
              mqtt_append_indexed_value(
                "SL_SENSOR_BH1750_ID_",
                (uint32_t)((i + 1)),
                ": ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].light) * 1000.0f),
                " mlx    ");
#endif
            }
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_NUM_OF_SAMPLES) {
            for (uint32_t i = 0; i < sensor_hub_info_t[sens_ind].data_deliver.numofsamples; i++) {
              SL_PRINT_STRING_ERROR(
                "\r\n %ld mlx \t",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].light) * 1000.0f));
#if SH_AWS_ENABLE
              mqtt_append_indexed_value(
                "SL_SENSOR_BH1750_ID_",
                (uint32_t)((i + 1)),
                ": ",
                (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[i].light) * 1000.0f),
                " mlx    ");
#endif
            }
          }

          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_THRESHOLD) {
            SL_PRINT_STRING_ERROR(
              "\r\n %ld mlx \t",
              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].light) * 1000.0f));
#if SH_AWS_ENABLE
            mqtt_append_value("SL_SENSOR_BH1750_ID: ",
                              (int32_t)((sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].light) * 1000.0f),
                              " mlx    ");
#endif
          }
        }
      }

      if (SL_SENSOR_ADC_JOYSTICK_ID == sensor_id) {

#if ((defined SH_ADC_ENABLE) || (defined SH_SDC_ENABLE))
        float vout = 0;
#endif
        if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_INTERRUPT_MODE) {
#ifdef SH_ADC_ENABLE
          for (uint32_t i = 0; i < bus_intf_info.adc_config.adc_ch_cfg.num_of_samples[JS_ADC_CHANNEL]; i++) {
            SL_PRINT_STRING_ERROR("\r\n %dmV \t", sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]);
#if SH_AWS_ENABLE
            mqtt_append_indexed_value("SL_SENSOR_JOYSTICK_ID_",
                                      (uint32_t)(i + 1),
                                      ": ",
                                      (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]),
                                      "mV    ");
#endif
          }
#endif

#ifdef SH_SDC_ENABLE
#if ((defined SDC_MUTI_CHANNEL_ENABLE) || (SH_AWS_ENABLE))
          uint16_t sdc_channel_id = 0;
#endif
#ifdef SDC_MUTI_CHANNEL_ENABLE

          for (uint32_t i = 0; i <= bus_intf_info.sh_sdc_config.sh_sdc_sample_ther; i++) {
            //DEBUGOUT("%dmV \t", (0x0FFF&sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].sh_sdc_data[i]));
            sdc_channel_id = sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].sh_sdc_data[i];
            sdc_channel_id = sdc_channel_id >> 12;
            vout = (((float)((0x0FFF & sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].sh_sdc_data[i]))
                     / (float)SL_SH_ADC_MAX_OP_VALUE)
                    * SL_SH_ADC_VREF_VALUE);

            SL_PRINT_STRING_ERROR("\r\n SDC Channel_Id:[%d]\tSample: %ldmV", sdc_channel_id, (int32_t)(vout * 1000.0f));
#else
          SL_PRINT_STRING_ERROR("\r\n SDC_Samples:");
          for (uint32_t i = 0; i <= bus_intf_info.sh_sdc_config.sh_sdc_sample_ther; i++) {
            //DEBUGOUT("%dmV \t", sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].sh_sdc_data[i]);
            vout = (((float)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].sh_sdc_data[i])
                     / (float)SL_SH_ADC_MAX_OP_VALUE)
                    * SL_SH_ADC_VREF_VALUE);
            SL_PRINT_STRING_ERROR("\r\n %ldmV \t", (int32_t)(vout * 1000.0f));
#endif
#if SH_AWS_ENABLE
            mqtt_append_indexed_cid_value(
              "SL_SENSOR_JOYSTICK_ID_",
              (uint32_t)(i + 1),
              (uint32_t)sdc_channel_id,
              (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].sh_sdc_data[i]),
              "mV    ");
#endif
          }

#endif
        } else if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_POLLING_MODE) {
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_TIMEOUT) {
            for (uint32_t i = 0;
                 i < sensor_hub_info_t[sens_ind].data_deliver.timeout / sensor_hub_info_t[sens_ind].sampling_interval;
                 i++) {
              SL_PRINT_STRING_ERROR("\r\n %dmV \t", sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]);
#if SH_AWS_ENABLE
              mqtt_append_indexed_value("SL_SENSOR_JOYSTICK_ID_",
                                        (uint32_t)(i + 1),
                                        ": ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]),
                                        "mV    ");
#endif
            }
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_NUM_OF_SAMPLES) {
            for (uint32_t i = 0; i < sensor_hub_info_t[sens_ind].data_deliver.numofsamples; i++) {
              SL_PRINT_STRING_ERROR("\r\n %dmV \t", sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]);
#if SH_AWS_ENABLE
              mqtt_append_indexed_value("SL_SENSOR_JOYSTICK_ID_",
                                        (uint32_t)(i + 1),
                                        ": ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]),
                                        "mV    ");
#endif
            }
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_THRESHOLD) {
            SL_PRINT_STRING_ERROR("\r\n %dmV \t", sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[0]);
#if SH_AWS_ENABLE
            mqtt_append_value("SL_SENSOR_JOYSTICK_ID: ",
                              (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[0]),
                              "mV    ");
#endif
          }
        }
#ifdef SH_ADC_ENABLE
        vout =
          (((float)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[0]) / (float)SL_SH_ADC_MAX_OP_VALUE)
           * SL_SH_ADC_VREF_VALUE);

#if SH_AWS_ENABLE
        mqtt_append_value("Single-ended output: ", (int32_t)(vout * 1000.0f), "mV    ");
#endif
#endif
#ifdef SH_ADC_ENABLE
        SL_PRINT_STRING_ERROR("\r\n Single ended input: %ldmV \t", (int32_t)(vout * 1000.0f));
#endif
      }

      if (SL_SENSOR_ADC_GUVA_S12D_ID == sensor_id) {
        float vout = 0;
        if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_INTERRUPT_MODE) {
          for (uint32_t i = 0; i < bus_intf_info.adc_config.adc_ch_cfg.num_of_samples[GUVA_ADC_CHANNEL]; i++) {
            SL_PRINT_STRING_ERROR("\r\n %d \t", sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]);
#if SH_AWS_ENABLE
            mqtt_append_indexed_value("SL_SENSOR_GUVA_S12D_ID_",
                                      (uint32_t)(i + 1),
                                      ": ",
                                      (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]),
                                      "    ");
#endif
          }
        } else if (sensor_hub_info_t[sens_ind].sensor_mode == SL_SH_POLLING_MODE) {
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_TIMEOUT) {
            for (uint32_t i = 0;
                 i < sensor_hub_info_t[sens_ind].data_deliver.timeout / sensor_hub_info_t[sens_ind].sampling_interval;
                 i++) {
              SL_PRINT_STRING_ERROR("\r\n %dmV \t", sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]);
#if SH_AWS_ENABLE
              mqtt_append_indexed_value("SL_SENSOR_GUVA_S12D_ID_",
                                        (uint32_t)((i + 1)),
                                        ": ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]),
                                        "    ");
#endif
            }
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_NUM_OF_SAMPLES) {
            for (uint32_t i = 0; i < sensor_hub_info_t[sens_ind].data_deliver.numofsamples; i++) {
              SL_PRINT_STRING_ERROR("\r\n %dmV \t", sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]);
#if SH_AWS_ENABLE
              mqtt_append_indexed_value("SL_SENSOR_GUVA_S12D_ID_",
                                        (uint32_t)((i + 1)),
                                        ": ",
                                        (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[i]),
                                        "    ");
#endif
            }
          }
          if (sensor_hub_info_t[sens_ind].data_deliver.data_mode == SL_SH_THRESHOLD) {
            SL_PRINT_STRING_ERROR("\r\n %dmV \t", sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[0]);
#if SH_AWS_ENABLE
            mqtt_append_value("SL_SENSOR_GUVA_S12D_ID: ",
                              (int32_t)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[0]),
                              "mV    ");
#endif
          }
        }
        vout =
          (((float)(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].adc[0]) / (float)SL_SH_ADC_MAX_OP_VALUE)
           * SL_SH_ADC_VREF_VALUE);
#if SH_AWS_ENABLE
        mqtt_append_value("Single-ended output: ", (int32_t)(vout * 1000.0f), "mV    ");
#endif
        SL_PRINT_STRING_ERROR("\r\n Single ended input: %ldmV \t", (int32_t)(vout * 1000.0f));
      }

      if (SL_SENSOR_ADC_GY_61_ID == sensor_id) {
#ifdef GY61_X_AXIS_ADC_CHANNEL
        for (uint32_t i = 0; i < bus_intf_info.adc_config.adc_ch_cfg.num_of_samples[GY61_X_AXIS_ADC_CHANNEL]; i++) {
          double x_g =
            ((float)gy61_adc_raw_data_map(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].gy61.x[i],
                                          GY61_X_RAW_MIN,
                                          GY61_X_RAW_MAX,
                                          GY61_G_SCALE_MIN,
                                          GY61_G_SCALE_MAX))
            / -100.0;
          SL_PRINT_STRING_ERROR("\r\n X = %ld mg, \t", (int32_t)((float)x_g * 1000.0f));
        }
#endif
#ifdef GY61_Y_AXIS_ADC_CHANNEL
        for (uint32_t i = 0; i < bus_intf_info.adc_config.adc_ch_cfg.num_of_samples[GY61_Y_AXIS_ADC_CHANNEL]; i++) {
          double y_g =
            ((float)gy61_adc_raw_data_map(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].gy61.y[0],
                                          GY61_Y_RAW_MIN,
                                          GY61_Y_RAW_MAX,
                                          GY61_G_SCALE_MIN,
                                          GY61_G_SCALE_MAX))
            / -100.0;
          SL_PRINT_STRING_ERROR("\r\n Y = %ld mg, \t", (int32_t)((float)y_g * 1000.0f));
        }
#endif
#ifdef GY61_Z_AXIS_ADC_CHANNEL
        for (uint32_t i = 0; i < bus_intf_info.adc_config.adc_ch_cfg.num_of_samples[GY61_Z_AXIS_ADC_CHANNEL]; i++) {
          double z_g =
            ((float)gy61_adc_raw_data_map(sensor_hub_info_t[sens_ind].sensor_data_ptr->sensor_data[0].gy61.z[0],
                                          GY61_Z_RAW_MIN,
                                          GY61_Z_RAW_MAX,
                                          GY61_G_SCALE_MIN,
                                          GY61_G_SCALE_MAX))
            / -100.0;
          SL_PRINT_STRING_ERROR("\r\n Z = %ld mg \t", (int32_t)((float)z_g * 1000.0f));
        }
#endif
      }

      SL_PRINT_STRING_ERROR("\r\n data_deliver.mode:%d \r\n", sensor_hub_info_t[sens_ind].data_deliver.data_mode);
      // Acknowledge data reception
      event_ack = sensor_id;

#if SH_AWS_ENABLE
      sl_sem_status = osSemaphoreRelease(sl_semaphore_aws_task_id);
      if (sl_sem_status != osOK) {
        SL_PRINT_STRING_ERROR("\r\n event post osSemaphoreRelease failed :%d \r\n", sl_sem_status);
      }
      sl_sem_status = osSemaphoreAcquire(sl_semaphore_app_task_id_2, osWaitForever);
      if (sl_sem_status != osOK) {
        SL_PRINT_STRING_ERROR("\r\n osSemaphoreAcquire failed :%d \r\n", sl_sem_status);
      }
#endif
      break;

    case SL_SENSOR_CNFG_INVALID:

      break;
    case SL_SENSOR_START_FAILED:
      SL_PRINT_STRING_ERROR("\r\n Sensor START failed:%u \r\n", sensor_id);

      break;
    case SL_SENSOR_STOP_FAILED:
      SL_PRINT_STRING_ERROR("\r\n Sensor STOP failed:%u \r\n", sensor_id);

      break;
    case SL_SENSOR_DELETED:
      SL_PRINT_STRING_ERROR("Sensor deleted:%u \r\n", sensor_id);
      break;

    case SL_SENSOR_DELETE_FAILED:
      SL_PRINT_STRING_ERROR("Sensor deleted failed:%u \r\n", sensor_id);
      break;
    default:
      break;
  }
}
#ifndef SH_AWS_ENABLE
/*******************************************************************************
   * Initialization of wireless APIs.
   * M4-TA handshake is performed.
   * TA is send to standby with RAM retention mode if SWITCH_TO_PS0 is disabled.
   * TA is send to standby without RAM retention mode if SWITCH_TO_PS0 is enabled.
   ******************************************************************************/
static sl_status_t initialize_wireless(void)
{
  // For M4-sleep wakeup, and to achieve minimum current in powersave application,
  // wifi is initialized, handshake is performed between M4 and TA, then
  // TA powersave profile is updated sleep with/without retention as per
  // requirements.
  // Wifi device configuration
  const sl_wifi_device_configuration_t client_init_configuration = {
    .boot_option     = LOAD_NWP_FW,
    .mac_address     = NULL,
    .band            = SL_SI91X_WIFI_BAND_2_4GHZ,
    .region_code     = US,
    .boot_config     = { .oper_mode = SL_SI91X_CLIENT_MODE,
                         .coex_mode = SL_SI91X_WLAN_ONLY_MODE,
                         .feature_bit_map =
                           (SL_WIFI_FEAT_SECURITY_OPEN | SL_WIFI_FEAT_WPS_DISABLE | SL_SI91X_FEAT_ULP_GPIO_BASED_HANDSHAKE),
                         .tcp_ip_feature_bit_map =
                           (SL_SI91X_TCP_IP_FEAT_DHCPV4_CLIENT | SL_SI91X_TCP_IP_FEAT_DNS_CLIENT | SL_SI91X_TCP_IP_FEAT_SSL
                        | SL_SI91X_TCP_IP_FEAT_ICMP | SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID),
                         .custom_feature_bit_map     = (SL_WIFI_SYSTEM_CUSTOM_FEAT_EXTENSION_VALID),
                         .ext_custom_feature_bit_map = 0,
                         .bt_feature_bit_map         = 0,
                         .ext_tcp_ip_feature_bit_map =
                           (SL_SI91X_EXT_TCP_IP_WINDOW_SCALING | SL_SI91X_EXT_TCP_IP_TOTAL_SELECTS(10)
                        | SL_SI91X_CONFIG_FEAT_EXTENSION_VALID),
                         .ble_feature_bit_map     = 0,
                         .ble_ext_feature_bit_map = 0,
                         .config_feature_bit_map  = SL_SI91X_FEAT_SLEEP_GPIO_SEL_BITMAP },
    .ta_pool         = { .tx_ratio_in_buffer_pool = 0, .rx_ratio_in_buffer_pool = 0, .global_ratio_in_buffer_pool = 0 },
    .efuse_data_type = SL_SI91X_EFUSE_MFG_SW_VERSION,
    .nwp_fw_image_number = SL_SI91X_NWP_FW_IMAGE_NUMBER_0
  };
  sl_status_t status;
  // Initialize the wifi interface.
  status = sl_wifi_init(&client_init_configuration, NULL, NULL);
  DEBUGINIT();
  if (status != SL_STATUS_OK) {
    // If status is not OK, return with the error code.
    SL_PRINT_STRING_ERROR("sl_wifi_init failed, Error Code: 0x%lX \n", status);
    return status;
  }
  uint8_t xtal_enable = 1;
  // M4-TA handshake is required for TA communication.
  status = sl_si91x_m4_ta_secure_handshake(SL_SI91X_ENABLE_XTAL, 1, &xtal_enable, 0, NULL);
  if (status != SL_STATUS_OK) {
    // If status is not OK, return with error code.
    SL_PRINT_STRING_ERROR("sl_si91x_m4_ta_secure_handshake failed, Error Code: 0x%lX \n", status);
    return status;
  }
  // Wireless Sleep with ram retention
  wireless_sleep();
  // If reaches here, returns SL_STATUS_OK.
  return SL_STATUS_OK;
}

/*******************************************************************************
   * After PS2 to PS4 transition, flash is initialized and to initialize flash
   * wireless processor is set to active mode.
   * This function sends the wireless processor to sleep with retention.
   ******************************************************************************/
void wireless_sleep(void)
{
  sl_status_t status;
  // Wifi Profile (TA Mode) is set to High Performance.
  sl_wifi_performance_profile_v2_t ta_performance_profile = { .profile = HIGH_PERFORMANCE };

  status = sl_wifi_set_performance_profile_v2(&ta_performance_profile);
  if (status != SL_STATUS_OK) {
    // If status is not OK, return with error code.
    SL_PRINT_STRING_ERROR("sl_wifi_set_performance_profile_v2 failed, Error Code: 0x%lX \n", status);
    return;
  }
  // Wifi Profile (TA Mode) is set to standby power save with RAM retention.
  ta_performance_profile.profile = DEEP_SLEEP_WITH_RAM_RETENTION;

  // Wifi Profile (TA Mode) is set to standby power save with RAM retention.
  status = sl_wifi_set_performance_profile_v2(&ta_performance_profile);
  if (status != SL_STATUS_OK) {
    // If status is not OK, return with error code.
    SL_PRINT_STRING_ERROR("sl_wifi_set_performance_profile_v2 failed, Error Code: 0x%lX \n", status);
    return;
  }
}
#endif
/**************************************************************************/ /**
 * @fn           void sl_si91x_sensorhub_app_task()
 * @brief        This function perform the all sensor related operations.
 *
 * @param[in]    None
 * @param[out]   None
******************************************************************************/
void sl_si91x_sensorhub_app_task(void)
{

  uint32_t status = 0;
  sl_sensor_id_t sl_sensor_scan_info[SL_MAX_NUM_SENSORS];

  osSemaphoreId_t sl_semaphore_app_task_id;
  osStatus_t sl_semapptaskacq_status;

  osSemaphoreAttr_t sl_app_semaphore_attr_st;
  sl_app_semaphore_attr_st.attr_bits = 0U;
  sl_app_semaphore_attr_st.cb_mem    = NULL;
  sl_app_semaphore_attr_st.cb_size   = 0U;
  sl_app_semaphore_attr_st.name      = NULL;

  SL_PRINT_STRING_ERROR("\r\n Start Sensor HUB APP Task \r\n");
  sl_semaphore_app_task_id = osSemaphoreNew(1U, 0U, &sl_app_semaphore_attr_st);
#ifndef SH_AWS_ENABLE
  // Initialize the wireless interface and put the TA in Standby with RAM retention mode.
  status = initialize_wireless();
  if (status != SL_STATUS_OK) {
    // If status is not OK, return with the error code.
    SL_PRINT_STRING_ERROR("Wireless API initialization failed, Error Code: 0x%lX \n", status);
    return;
  }
#endif

  // Register callback handler for getting different events from the sensor hub
  status = sl_si91x_sensorhub_notify_cb_register(sl_si91x_sensor_event_handler, (sl_sensor_id_t *)&event_ack);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("\r\n Unable to create call back info: %lu \r\n", status);
    while (1)
      ;
  }
  // Initialize sensor interface
  status = sl_si91x_sensorhub_init();
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("\r\n Sensor Hub Init failed \r\n");
    while (1)
      ;
  }

  // sensor hub scan
  sensor_scan_cnt = sl_si91x_sensorhub_detect_sensors((sl_sensor_id_t *)&sl_sensor_scan_info, SL_MAX_NUM_SENSORS);
  if (sensor_scan_cnt == 0) {
    SL_PRINT_STRING_ERROR("\r\n No sensor is detected \r\n");
    while (1)
      ;
  }

  // create sensors for scanned sensors
  for (uint32_t sensor_cnt = 0; sensor_cnt < sensor_scan_cnt; sensor_cnt++) {
    status = sl_si91x_sensorhub_create_sensor(sl_sensor_scan_info[sensor_cnt]);
    if (status != SL_STATUS_OK) {
      SL_PRINT_STRING_ERROR("\r\n Unable to create sensor %d,Error Code:%lu \r\n",
                            sl_sensor_scan_info[sensor_cnt],
                            status);
    }
  }
  // Start the sensor HUb Tasks
  status = sl_si91x_sensor_hub_start();
  if (status != RSI_OK) {
    SL_PRINT_STRING_ERROR("\r\n  Sensor HUB start failed \r\n");
    while (1)
      ;
  }
#if SH_AWS_ENABLE
  osThreadNew((osThreadFunc_t)sl_si91x_aws_task, NULL, &aws_thread_attributes);

  sl_semapptaskacq_status = osSemaphoreAcquire(sl_semaphore_app_task_id_2, osWaitForever);
  if (sl_semapptaskacq_status != osOK) {
    SL_PRINT_STRING_ERROR("\r\n osSemaphoreAcquire failed :%d \r\n", sl_semapptaskacq_status);
  }
#endif
  // Start the sensors
  for (uint32_t sensor_cnt = 0; sensor_cnt < sensor_scan_cnt; sensor_cnt++) {
    // start a sensor, data ready events will be posted once data is acquired successfully
    status = sl_si91x_sensorhub_start_sensor(sl_sensor_scan_info[sensor_cnt]);
    if (status != SL_STATUS_OK) {
      SL_PRINT_STRING_ERROR("\r\n Unable to start sensor %d \r\n", sl_sensor_scan_info[sensor_cnt]);
    }
  }
  while (1) {
    // waiting for the semaphore release
    sl_semapptaskacq_status = osSemaphoreAcquire(sl_semaphore_app_task_id, osWaitForever);
    if (sl_semapptaskacq_status != osOK) {
      SL_PRINT_STRING_ERROR("\r\n osSemaphoreAcquire failed :%d \r\n", sl_semapptaskacq_status);
    }
  }
}
/**************************************************************************/ /**
 * @fn           void sensorhub_app_init()
 * @brief        This function will update the SCB->VTOR Register with new Vector address.
 *               It will change the core frequency to 20 MHz and create the
 *               thread to the sensor app task.
 * @param[in]    None
 * @param[out]   None
******************************************************************************/

void sensorhub_app_init(void)
{
  // Updating the CPU core clock by 20 MHz to work in PS2 mode
  RSI_IPMU_M20rcOsc_TrimEfuse();
  RSI_PS_FsmHfFreqConfig(20);
  RSI_CLK_M4SocClkConfig(M4CLK, M4_ULPREFCLK, 0);

  // Initializes board UART for Prints
  DEBUGINIT();
#if SH_AWS_ENABLE
  osSemaphoreAttr_t sl_app_semaphore_attr_st;
  sl_app_semaphore_attr_st.attr_bits = 0U;
  sl_app_semaphore_attr_st.cb_mem    = NULL;
  sl_app_semaphore_attr_st.cb_size   = 0U;
  sl_app_semaphore_attr_st.name      = NULL;

  sl_semaphore_app_task_id_2 = osSemaphoreNew(1U, 0U, &sl_app_semaphore_attr_st);
#endif

  // Create the APP task
  osThreadNew((osThreadFunc_t)sl_si91x_sensorhub_app_task, NULL, &app_thread_attributes);
}
