#include "main.h"
#include <lvgl.h>
#include <DHT.h>
#include "display_mng.h"

// Macros
#define DHT_PIN                             (33)
#define DHT_TYPE                            (DHT22)
#define LVGL_REFRESH_TIME                   (5u)      // 5 milliseconds
#define DHT_REFRESH_TIME                    (2000u)   // 2 seconds

// Task priorities
#define DHT_TASK_PRIORITY                   (configMAX_PRIORITIES - 1)
#define LVGL_TASK_PRIORITY                  (configMAX_PRIORITIES - 2)

// Task handles
TaskHandle_t dhtTaskHandle = NULL;
TaskHandle_t lvglTaskHandle = NULL;

// Private Variables
static uint32_t lvgl_refresh_timestamp = 0u;
static uint32_t dht_refresh_timestamp = 0u;

// DHT Related Variables and Instances
DHT dht(DHT_PIN, DHT_TYPE);
static Sensor_Data_s sensor_data = 
{
  .sensor_idx = 0u,
};

// Private functions
static void DHT_TaskInit( void );
static void DHT_Task( void *pvParameters );
static void LVGL_TaskInit( void );
static void LVGL_Task( void *pvParameters );

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println("Temperature and Humidity Graph Using LVGL");

  // Initialize the LVGL Library
  LVGL_TaskInit();
  
  // Initialize Display and Display Buffers
  Display_Init();

  // Initialize DHT sensor
  DHT_TaskInit();

  // Create tasks for FreeRTOS
  xTaskCreate(DHT_Task, "DHT Task", 2048, NULL, DHT_TASK_PRIORITY, &dhtTaskHandle);
  xTaskCreate(LVGL_Task, "LVGL Task", 2048, NULL, LVGL_TASK_PRIORITY, &lvglTaskHandle);

  // Start the scheduler
  vTaskStartScheduler();
}

void loop()
{
  // FreeRTOS tasks will handle everything
}

Sensor_Data_s * Get_TemperatureAndHumidity( void )
{
  return &sensor_data;
}

// Private Function Definition
static void DHT_TaskInit( void )
{
  dht.begin();
  dht_refresh_timestamp = millis();
}

static void DHT_Task( void *pvParameters )
{
  (void)pvParameters;
  uint32_t now;
  float temperature;
  float humidity;

  while(1)
  {
    now = millis();
    if( (now - dht_refresh_timestamp) >= DHT_REFRESH_TIME )
    {
      dht_refresh_timestamp = now;
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      
      if (isnan(humidity) || isnan(temperature) ) 
      {
        Serial.println(F("Failed to read from DHT sensor!"));
      }
      else
      {
        Serial.print(F("Humidity: "));
        Serial.print(humidity);
        Serial.print(F("%  Temperature: "));
        Serial.print(temperature);
        Serial.println(F("°C "));
        if( sensor_data.sensor_idx < SENSOR_BUFF_SIZE )
        {
          sensor_data.temperature[sensor_data.sensor_idx] = (uint8_t)(temperature);
          sensor_data.humidity[sensor_data.sensor_idx] = (uint8_t)(humidity);
          sensor_data.sensor_idx++;
          if( sensor_data.sensor_idx >= SENSOR_BUFF_SIZE )
          {
            sensor_data.sensor_idx = 0u;
          }
        }
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay to prevent task from running continuously
  }
}

static void LVGL_TaskInit( void )
{
  lv_init();
  lvgl_refresh_timestamp = millis();
}

static void LVGL_Task( void *pvParameters )
{
  (void)pvParameters;
  uint32_t now;

  while(1)
  {
    now = millis();
    if( (now - lvgl_refresh_timestamp) >= LVGL_REFRESH_TIME )
    {
      lvgl_refresh_timestamp = now;
      lv_timer_handler();
    }
    vTaskDelay(5 / portTICK_PERIOD_MS);  // Delay to allow other tasks to run
  }
}
