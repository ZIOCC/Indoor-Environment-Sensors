#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_SHT31.h"
#include <Adafruit_BMP280.h>
#include "Adafruit_CCS811.h"
#include <WiFiMulti.h>
#include "TSL2561.h"

WiFiMulti WiFiMulti;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_BMP280 bmp; // I2C
Adafruit_CCS811 ccs;
WiFiClient  client;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define led 5
#define PMaddress 0x12
#define HEADER_H  0x42
#define HEADER_L  0x4D
#define FRAME_LENGTH  0x1C
uint8_t COMMAND = 0x00;

const char* ssid     = "Your SSID"; // Your SSID
const char* password = "your wifi password"; // WiFi password

const char* host = "api.thingspeak.com";
String api_key = "ITYE5PN9RNOK6JMR"; // copy and paste the API Write Key provied by ThingSpeak Environment Sensor Data4

float Air_Temperature = 0;
float Air_Humidity = 0;
float Soil_Temperature = 0;
float Soil_Humidity = 0;
float AirPressure = 0;
int CO2=0;
int TVOC=0;

unsigned char cur_rx_data;
unsigned char pre_rx_data;
unsigned char main_status=0;
unsigned char recv_buff[256]={0};
unsigned char recv_buff_index=0;
unsigned char incomingByte;

int pm1_0_cf_1=0;
int pm2_5_cf_1=0;
int pm10_cf_1=0;

int pm1_0=0;
int pm2_5=0;
int pm10=0;

unsigned short check_sum;

#define COMMAND_LOUDNESS_GET_VALUE        0x05
#define COMMAND_LOUDNESS_NOTHING_NEW   0x99
const byte LOUDNESS_qwiicAddress = 0x38;     //Default Address
uint16_t ADC_LOUDNESS_VALUE=0;

TSL2561 tsl(TSL2561_ADDR_FLOAT);
uint8_t num=0; 
void setup() {
    
  Serial.begin(115200);
  pinMode(led,OUTPUT);
  WiFi.mode(WIFI_STA);
  connectToWifi();
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  Serial.println("SHT31 Initializing...");
  Serial.println(F("BMP280 test"));  
  if (!bmp.begin()) {  
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  if(!ccs.begin()){
    Serial.println("Failed to start CCS sensor! Please check your wiring.");
    while(1);
  }
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();

  testForConnectivity();

  if (tsl.begin()) {
    Serial.println("Found tsl sensor");
  } else {
    Serial.println("No tsl sensor?");
    while (1);
  }
  tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
 //calibrate temperature sensor
//  while(!ccs.available());
//  float temp = ccs.calculateTemperature();
//  ccs.setTempOffset(temp - 25.0); 
}

void loop() {

 readSensors();
 Send_Data(); //call function to send data to Thingspeak
 delay(1000); 
}

/********* Read Sensors value *************/
void readSensors(void)
{
 Air_Temperature = sht31.readTemperature();
 Air_Humidity = sht31.readHumidity();
 Serial.print("Temperature:  ");
 Serial.print(Air_Temperature);
 Serial.print("Humidity:  ");
 Serial.print(Air_Humidity);
 
 Serial.print(F("Temperature = "));
 Serial.print(bmp.readTemperature());
 Serial.println(" *C");

 Serial.print(F("Pressure = "));
 AirPressure=bmp.readPressure();
 Serial.print(AirPressure);
 Serial.println(" Pa");

 Serial.print(F("Approx altitude = "));
 Serial.print(bmp.readAltitude(1013.25)); // this should be adjusted to your local forcase
 Serial.println(" m");

 if(ccs.available())
 {
  if(!ccs.readData()){
  CO2=ccs.geteCO2();
  TVOC=ccs.getTVOC();
  Serial.print("CO2: ");
  Serial.print(CO2);
  Serial.print("ppm, TVOC: ");
  Serial.print(TVOC);
  Serial.println("ppb");
  }
 else{
      Serial.println("CCS811 ERROR!");
 }
 }
 readPMsensor();
 
 uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE); 
 Serial.println(x, DEC);
 uint32_t lum = tsl.getFullLuminosity();
 uint16_t ir, full;
 ir = lum >> 16;
 full = lum & 0xFFFF;
 Serial.print("IR: "); Serial.print(ir);   Serial.print("\t\t");
 Serial.print("Full: "); Serial.print(full);   Serial.print("\t");
 Serial.print("Visible: "); Serial.print(full - ir);   Serial.print("\t"); 
 Serial.print("Lux: "); Serial.println(tsl.calculateLux(full, ir));

 switch (num)
 {
  case 0:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.println("Loudness:");
          display.print("ADC:");
          display.print(get_loudness_value());
          display.display();
          delay(1);
          }break;
  case 1:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.print("IR: ");
          display.println(ir);
          display.print("Full: ");
          display.println(full);
          
          display.display();
          delay(1);
          }break;
  case 2:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.print("Vis: ");
          display.println(full-ir);
          display.print("Lux: ");
          display.println(tsl.calculateLux(full, ir));
          display.display();
          delay(1);
          }break;
  case 3:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.println("Temp:");
          display.print(Air_Temperature);
          display.println(" *C");
          display.display();
          delay(1);
          }break;
  case 4:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.println("Hum:");
          display.print(Air_Humidity);
          display.println(" %");
          display.display();
          delay(1);
          }break;
  case 5:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.println("Pressure:");
          display.print(AirPressure);
          display.println(" Pa");
          display.display();
          delay(1);
          }break;
  case 6:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.println("CO2:");
          display.print(CO2);
          display.print(" ppm");
          display.display();
          delay(1);
          }break;
  case 7:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.println("TVOC: ");
          display.print(TVOC);
          display.print(" ppb");
          display.display();
          delay(1);
          }break;
  case 8:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.println("pm1.0:  ");
          display.print(pm1_0);
          display.println(" ug/m3");
          display.display();
          delay(1);
          }break;
  case 9:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.println("pm2.5:  ");
          display.print(pm2_5);
          display.println(" ug/m3");
          display.display();
          delay(1);
          }break;
  case 10:
         {
          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.clearDisplay();
          display.println("pm10:  ");
          display.print(pm10);
          display.println(" ug/m3");
          display.display();
          delay(1);
          }break;
  default:break;
  }
  num++;
  if(num==10)
  {
    num=0;
    }
 
}
/********* Read PMSensors value *************/
void readPMsensor()
{
    Wire.requestFrom(PMaddress, 32);
  while(Wire.available())
  {
    cur_rx_data = Wire.read();
    switch(main_status)
     {
      case 0:
         if( cur_rx_data == HEADER_L )
         {
            if( pre_rx_data ==  HEADER_H )
            {
                 main_status++;
                 
                 check_sum += pre_rx_data;
                 check_sum += cur_rx_data;
                 
                 cur_rx_data = 0;
                 pre_rx_data = 0;

                 
            }
         }else
         {
            pre_rx_data = cur_rx_data;
         }
        break;
        case 1:
        if( cur_rx_data == FRAME_LENGTH )
         {
            if( pre_rx_data ==  0x00 )
            {
                 main_status++;
                 
                 check_sum += pre_rx_data;
                 check_sum += cur_rx_data;
                 
                 cur_rx_data = 0;
                 pre_rx_data = 0;
            }
         }else
         {
            pre_rx_data = cur_rx_data;
         }
        break;
        case 2:
        recv_buff[recv_buff_index++] = cur_rx_data;
        check_sum += cur_rx_data;
        if( recv_buff_index == 26)
        {
             main_status++;
          
             cur_rx_data = 0;
             pre_rx_data = 0;
        }
        break;
        case 3:
        recv_buff[recv_buff_index++] = cur_rx_data;
        if( recv_buff_index == 28)
        { 
            if( check_sum == ( (recv_buff[26]<<8) + recv_buff[27]) )
            {
                recv_buff_index = 0;  
                pm1_0_cf_1 = (recv_buff[recv_buff_index] << 8) + recv_buff[recv_buff_index+1];
                recv_buff_index += 2;
                pm2_5_cf_1 = (recv_buff[recv_buff_index] << 8) + recv_buff[recv_buff_index+1];
                recv_buff_index += 2;
                pm10_cf_1 = (recv_buff[recv_buff_index] << 8) + recv_buff[recv_buff_index+1];
                recv_buff_index += 2;
                
                pm1_0 = (recv_buff[recv_buff_index] << 8) + recv_buff[recv_buff_index+1];
                recv_buff_index += 2;
                pm2_5 = (recv_buff[recv_buff_index] << 8) + recv_buff[recv_buff_index+1];
                recv_buff_index += 2;
                pm10 = (recv_buff[recv_buff_index] << 8) + recv_buff[recv_buff_index+1];
                
                Serial.print("PMSA003I:    ");
//                Serial.print("pm1.0_cf_1:");Serial.print(pm1_0_cf_1,DEC);Serial.print("ug/m3");Serial.print("  ");
//                Serial.print("pm2.5_cf_1:"); Serial.print(pm2_5_cf_1,DEC);Serial.print("ug/m3");Serial.print("  ");
//                Serial.print("pm10_cf_1:");Serial.print(pm10_cf_1,DEC);Serial.print("ug/m3");Serial.print("  ");
                Serial.print("pm1.0:  ");Serial.print(pm1_0,DEC);Serial.print("ug/m3");Serial.print("  ");
                Serial.print("pm2.5:  ");Serial.print(pm2_5,DEC);Serial.print("ug/m3");Serial.print("  ");
                Serial.print("pm10:  ");Serial.print(pm10,DEC);Serial.println("ug/m3");
            }
            main_status = 0;    
            cur_rx_data = 0;
            pre_rx_data = 0; 
            check_sum = 0;
            for(int i=0;i< 256; i++)
            {
              recv_buff[i] = 0x00;
            }
            recv_buff_index = 0x00;      
        }
        break;
        case 4:
        break;
        case 5:
        break;
        case 6:
        break;
        case 7:
        break;
        case 8:
        break;
        case 9:
        break;
        case 10:
        break;
        case 11:
        break;
         case 12:
        break;
        case 13:
        break;
     } 
    }
  }
void connectToWifi()
{
  // connection to a WiFi network
  WiFiMulti.addAP(ssid, password);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
 }
 
void Send_Data()
{
  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  const int httpPort = 80;

  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  else
  {
    String data_to_send = api_key;
    data_to_send += "&field1=";
    data_to_send += String(Air_Temperature);
    data_to_send += "&field2=";
    data_to_send += String(Air_Humidity);
    data_to_send += "&field3=";
    data_to_send += String(pm1_0);
    data_to_send += "&field4=";
    data_to_send += String(pm2_5);
    data_to_send += "&field5=";
    data_to_send += String(pm10);
    data_to_send += "&field6=";
    data_to_send += String(CO2);
    data_to_send += "&field7=";
    data_to_send += String(TVOC);
    data_to_send += "&field8=";
    data_to_send += String(AirPressure);
    data_to_send += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + api_key + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(data_to_send.length());
    client.print("\n\n");
    client.print(data_to_send);
    digitalWrite(led,HIGH);
    delay(100);
    digitalWrite(led,LOW);
    delay(1000);
  }

  client.stop();

}

uint16_t get_loudness_value() {
  uint16_t ADC_LOUDNESS_VALUE=0;
  Wire.beginTransmission(LOUDNESS_qwiicAddress);
  Wire.write(COMMAND_LOUDNESS_GET_VALUE); // command for status
  Wire.endTransmission();    // stop transmitting //this looks like it was essential.

  Wire.requestFrom(LOUDNESS_qwiicAddress, 2);    // request 1 bytes from slave device qwiicAddress

  while (Wire.available()) { // slave may send less than requested
  uint8_t ADC_VALUE_L = Wire.read(); 
  uint8_t ADC_VALUE_H = Wire.read();
  ADC_LOUDNESS_VALUE=ADC_VALUE_H;
  ADC_LOUDNESS_VALUE<<=8;
  ADC_LOUDNESS_VALUE|=ADC_VALUE_L;
  Serial.print("ADC_LOUDNESS_VALUE:  ");
  Serial.println(ADC_LOUDNESS_VALUE,DEC);
  }
  uint16_t x=Wire.read(); 
  return ADC_LOUDNESS_VALUE;
}

void testForConnectivity() {
  Wire.beginTransmission(LOUDNESS_qwiicAddress);
  //check here for an ACK from the slave, if no ACK don't allow change?
  if (Wire.endTransmission() != 0) {
    Serial.println("Check connections. No slave attached.");
    while (1);
  }
}
