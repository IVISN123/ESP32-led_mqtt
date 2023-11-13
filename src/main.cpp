#include "Arduino.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "Adafruit_NeoPixel.h"
#include <SPI.h>   
#define modes 2
/////////////////// SETTINGS /////////////////////////////////////////////////////////

WiFiClient espClient;
PubSubClient client(espClient);

// Wi-Fi
const char* ssid = "pressF";
const char* password = "010720291";

// MQTT
const char* mqtt_server = "m5.wqtt.ru";
const int mqtt_port = 8374;
const char* mqtt_user = "u_XDXG92";
const char* mqtt_password = "KbBfNyHK";

// Topic
const String led_topic = "/led";//"Прием" яркость -"/led/brig", цвет - "/led/color"
const char* led_send_topic = "/led"; //"Отправка"
bool led_on = false;// Переменная состаяния светодиодов


//NeoPixel обозначение пинов
const int LED_PIN = 2;
const int LED_COUNT = 140;
//NeoPixel начальные настройки
Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
int led_brig = 90;
String led_color = "#ffffff";

int lamp_r = 250 ;
int lamp_g = 250 ;
int lamp_b = 250 ;

//Настройки сенсора
const int bt1 = 32;        // сенсор пин
uint8_t BtF1 = 1;    // Фильтр от ложных срабатываний (по умолчанию не касаемся)
uint8_t StBt = 0;   // переменная для хранения статуса кнопки
const bool retain_flag = false;


TaskHandle_t Task1;
/////////////////////// VOID`S ///////////////////////////////////

//функции для светодиодов
void setPixel(int p, byte r, byte g, byte b) {
  r = r * led_brig / 100;
  g = g * led_brig / 100;
  b = b * led_brig / 100;
  neoPixel.setPixelColor(p, neoPixel.Color(r, g, b));
}
void setAll(byte r, byte g, byte b) {
  neoPixel.clear();
  for(int i = 0; i <= LED_COUNT; i++ ) {
    setPixel(i, r, g, b);
  }
  neoPixel.show();
}

//Подключение к Wi-Fi
void setup_wifi() {
      
  Serial.print("connecting running on core ");
  Serial.println(xPortGetCoreID());
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Обновление пинов светодиодной ленты
void updateStatePins(void){
    long color = strtol( &led_color[1], NULL, 16);
     lamp_r = color >> 16;
     lamp_g = color >> 8 & 0xFF;
     lamp_b = color & 0xFF;

    if(led_on){
      setAll(lamp_r, lamp_g, lamp_b);
    }else{
      setAll(0, 0, 0);
    }
}

// Чтение из топика топика
void callback(char* topic, byte* payload, unsigned int length) {
  String data_pay;
  for (int i = 0; i < length; i++) {
    data_pay += String((char)payload[i]);
  }
    Serial.println(data_pay);

//Светодиоды ВКЛ-ВЫКЛ
  if( String(topic) == led_topic ){
        if(data_pay == "ON" || data_pay == "1") led_on = true;
        if(data_pay == "OFF" || data_pay == "0") led_on = false;
    }
//Светодиоды ХЗ
  if( String(topic) == (led_topic + "/brig") ){
        led_brig = data_pay.toInt();
    }
//Светодиоды цвет
  if( String(topic) == (led_topic + "/color") ){
        led_color = data_pay;
    }

    updateStatePins();
}

// Переподключение
void reconnect() {
  if (!client.connected()) {
        
    Serial.print("reconnecting running on core ");
    Serial.println(xPortGetCoreID());
    Serial.print("Подключение к MQTT");
    String clientId = "ESP32-зеркало" + WiFi.macAddress();
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password) ) {
      Serial.println("Подключено");
      
      /*Для добавления топиков нужно прописать здесь в формате: 
      client.subscribe("название строки с топиком без скобок" + /#).c_str()
      */
      client.subscribe( (led_topic + "/#").c_str() ); // подписка на топик

    } else {
      Serial.print("Ошибка, #");
      Serial.print(client.state());
      Serial.println(" Повторное подключение через 5 с.");
      delay(50);
    }
  }
}

void Task1code( void * pvParameters ) {

  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  
  for(;;){
    delay(10);
  if ((touchRead(bt1)) <50) { 
      Serial.println(touchRead(bt1));   // Если прикоснулись к сенсорной кнопке (повысим чувсвительность)
  if (bitRead(BtF1, 7)) {
      bitSet(StBt, 0);
      } else BtF1 <<= 1;
    }
  else {                          // Не касаемся сенсорной кнопки
      if (bitRead(BtF1, 0)) {
        bitClear(StBt, 0); bitClear(StBt, 1);
        } else BtF1 >>= 1;       
  } 
  if (bitRead(StBt, 0) && !bitRead(StBt, 1)) {  // Обрабатываем только одно прикосновение

    bitSet(StBt, 1);        // пока не отпустят кнопку - больше не реагируем
    led_on = !led_on;  //изменение значения ленты
    
    if(led_on){
      setAll(lamp_r, lamp_g, lamp_b);
    }else{
      setAll(0, 0, 0);
    }

      switch (modes) {
    case 0: 
       lamp_r = 255 ;
       lamp_g = 248 ;
       lamp_b = 220 ;
      break;
    case 1: 
       lamp_r = 250 ;
       lamp_g = 250 ;
       lamp_b = 250 ;
      break;
  }

    client.publish(led_send_topic, String(led_on).c_str(), retain_flag); //изменение значения перменной на топике
    updateStatePins; 
    }}
  delay(100);
}



//////////////////////// main ////////////////////////////
void setup() {
  //Запуск ленты и выставление 0го цвета
  neoPixel.begin();
  setAll(0, 0, 0);

  //создаем задачу, которая будет выполняться на ядре 0 с максимальным приоритетом (1)
  xTaskCreatePinnedToCore(
                    Task1code,   /* Функция задачи. */
                    "Task1",     /* Ее имя. */
                    10000,       /* Размер стека функции */
                    NULL,        /* Параметры */
                    1,           /* Приоритет */
                    &Task1,      /* Дескриптор задачи для отслеживания */
                    0);          /* Указываем пин для данного ядра */  

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

}
void loop() {
  //Переподключение
  if (!client.connected()) {
    reconnect();
    delay(3000);
  }
  client.loop();

}
