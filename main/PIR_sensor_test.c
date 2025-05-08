#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define GPIO_SENSOR GPIO_NUM_22 
#define GPIO_LED GPIO_NUM_32 

QueueHandle_t queue = NULL;
TaskHandle_t blink_handle = NULL;

void blink_led(void *params){
  gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

  while(true){
    gpio_set_level(GPIO_LED, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(GPIO_LED, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void check_PIRsensor(void *params){

  gpio_set_direction(GPIO_SENSOR, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_SENSOR, GPIO_PULLUP_ONLY); 
  printf("Sensor Test. \nPIR connected and warming up...\n");
  vTaskDelay(pdMS_TO_TICKS(30000));
  printf("Sensor Test. \nPIR ready.\n");
  while(true){
    bool sensorValue = gpio_get_level(GPIO_SENSOR);
    printf("Sensor Value: %s\n", sensorValue ? "HIGH" : "LOW");
    xQueueSendToFront(queue, &sensorValue, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
  }

}

void app_main(void)
{
  queue = xQueueCreate(5, sizeof(bool));
  configASSERT(queue);
  xTaskCreate(check_PIRsensor, "check_PIRsensor", 2048, NULL, 5, NULL);
  vTaskDelay(pdMS_TO_TICKS(5000));
  xTaskCreate(blink_led, "blink_led", 2048, NULL, 5, &blink_handle);
  vTaskSuspend(blink_handle);
  gpio_set_level(GPIO_LED, 0);
  bool paused = true;
  bool sensorValue = 0;

  while(1){
    
    if(xQueueReceive(queue, &sensorValue, 0) == pdTRUE){
      if(sensorValue == 1 && paused == true){
        vTaskResume(blink_handle);
        printf("Led blinking.\n");
        paused = false;
      } else if(sensorValue == 0 && paused == false){
        vTaskSuspend(blink_handle);
        gpio_set_level(GPIO_LED, 0);
        printf("Led stopped.\n");
        paused = true;
      }
    }
    else{
      printf("PIR not ready.\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelay(pdMS_TO_TICKS(paused ? 250 : 1000));
  }
}
