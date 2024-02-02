//
// Created by Aleksey Volkov on 23.11.2020.
//

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "string.h"
#include <esp_log.h>
#include <esp_system.h>
#include <math.h>
#include <stdio.h>

#include "adc.h"
#include "fanspeed.h"
#include "pwm.h"
#include "settings.h"

static const char *TAG = "FANSPEED";

TimerHandle_t xClearCounterTimer = NULL;
int counter = 0;
static pcnt_unit_handle_t s_pcnt_unit;
static pcnt_channel_handle_t s_pcnt_chan;

/* reset retry counter and try to connect tu AP */
void vClearCounterTimerCallback(TimerHandle_t pxTimer) {
  /* get current value */
  int cnt = 0;
  pcnt_unit_get_count(s_pcnt_unit, &cnt);

  /* clear counter */
  pcnt_unit_clear_count(s_pcnt_unit);
  if ( cnt != 0 )
  {
    counter += cnt;
    ESP_LOGW(TAG, "Count: %d", counter);
  }
}

static void pcnt_init(void) {

  /* Prepare configuration for the PCNT unit */
  // pcnt_config_t pcnt_config = {
  //     // Set PCNT input signal and control GPIOs
  //     .pulse_gpio_num = GPIO_NUM_26,
  //     .ctrl_gpio_num = PCNT_PIN_NOT_USED,
  //     .channel = PCNT_CHANNEL_0,
  //     .unit = PCNT_UNIT_0,
  //     // What to do on the positive / negative edge of pulse input?
  //     .pos_mode = PCNT_COUNT_DIS, // Count up on the positive edge
  //     .neg_mode = PCNT_COUNT_INC, // Keep the counter value on the negative edge
  // };
  pcnt_unit_config_t pcnt_config = {
    .high_limit = 1000,
    .low_limit = -1000,
  };
  ESP_ERROR_CHECK(pcnt_new_unit(&pcnt_config, &s_pcnt_unit));

  pcnt_chan_config_t chan_config = {
    .edge_gpio_num = GPIO_NUM_26,
    .level_gpio_num = -1,
    .flags.io_loop_back = true
  };
  ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt_unit, &chan_config, &s_pcnt_chan));
  
  ESP_ERROR_CHECK(pcnt_channel_set_edge_action(s_pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(s_pcnt_chan, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP));

  /* Initialize PCNT unit */
  //pcnt_unit_config(&pcnt_config);

  /* Configure and enable the input filter */
  // pcnt_set_filter_value(PCNT_UNIT_0, 100);
  // pcnt_filter_enable(PCNT_UNIT_0);
  pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
  };
  ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(s_pcnt_unit, &filter_config));

  /* Enable events on zero, maximum and minimum limit values */
  // pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_ZERO);
  // pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_H_LIM);
  // pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_L_LIM);

  /* Initialize PCNT's counter */
  pcnt_unit_enable(s_pcnt_unit);
  pcnt_unit_clear_count(s_pcnt_unit);

  /* Everything is set up, now go to counting */
  pcnt_unit_start(s_pcnt_unit);
}

void fan_speed_init() {
  /* Initialize PCNT */
  pcnt_init();

  /* clear counter timer */
  xClearCounterTimer =
      xTimerCreate("ClearCounterTimer", (250 / portTICK_PERIOD_MS), pdTRUE, 0,
                   vClearCounterTimerCallback);

  if (xClearCounterTimer != NULL) {
    xTimerStart(xClearCounterTimer, 1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "xClearCounterTimer started");
  }
}

/* RPM */
uint32_t get_fan_rpm() { return counter * 4 * 60; }
