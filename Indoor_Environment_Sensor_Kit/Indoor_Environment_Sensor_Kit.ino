// Wire Master Reader

// Created 9 April 2019

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_SHT31.h"
#include "SparkFunCCS811.h"
#include "TSL2561.h"
#include "RTClib.h"
#include "Qwiic_LED_Stick.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_BMP280 bmp; // I2C
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#define CCS811_ADDR 0x5A //Default I2C Address
CCS811 mySensor(CCS811_ADDR);
TSL2561 tsl(TSL2561_ADDR_FLOAT); 
RTC_DS3231 rtc;
LED LEDStick; //Create an object of the LED class

/******************************* OLED *******************************/
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
uint8_t Display_Flag=0;
/******************************* PM2.5 *******************************/
#define PM_address 0x12
#define HEADER_H  0x42
#define HEADER_L  0x4D
#define FRAME_LENGTH  0x1C

uint8_t COMMAND = 0x00;
uint8_t Sensor_Databufer[32];

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
/******************************* Loundness *******************************/
#define COMMAND_LED_OFF     0x00
#define COMMAND_LED_ON      0x01
#define COMMAND_GET_VALUE        0x05
#define COMMAND_NOTHING_NEW   0x99

const byte Loundness_Address = 0x38;     //Default Address
uint16_t ADC_VALUE=0;

/******************************* DS3231 *******************************/
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


/******************************* setup *******************************/
void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(115200);  // start serial for output
  if (!bmp.begin()) {  
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  CCS811Core::status returnCode = mySensor.begin();
  if (returnCode != CCS811Core::SENSOR_SUCCESS)
  {
    Serial.println("CCS811.begin() returned with an error.");
    while (1); //Hang if there was a problem.
  }
  if (!tsl.begin()) {
    Serial.println("No TSL2561 sensor?");
    while (1);
  }
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  testForConnectivity();
  tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)

  LEDStick.begin();
  LEDStick.setLEDBrightness(10);
  LEDStick.LEDOff();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  pinMode(5,OUTPUT);
}

void loop() {
  digitalWrite(5,HIGH);
  LEDStick.setLEDColor(1, 0, 0, 180);
  Read_RTC_Data();
  Read_TSL2561_Sensordata();
  Read_PM_Sensordata();
  Read_BMP280_Sensordata();
  Read_SHT31_Sensordata();
  get_loundness_value();
  Read_CCS811_Sensordata();
  
  Serial.println();
  delay(1000);
  digitalWrite(5,LOW);
  LEDStick.LEDOff();
  delay(2000);
  Display_Flag++;
  if(Display_Flag==7)
  {Display_Flag=0;}
}

/******************************* TSL2561 *******************************/
void Read_RTC_Data()
{
  DateTime now = rtc.now();
  
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  
  if(Display_Flag==0)
  {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(now.year());
  display.print("/");
  display.print(now.month());
  display.print("/");
  display.print(now.day());
  display.print(" (");
  display.print(daysOfTheWeek[now.dayOfTheWeek()]);
  display.println(") ");
  display.print(now.hour());
  display.print(":");
  display.print(now.minute());
  display.print(":");
  display.print(now.second());
  display.display();
  }
  }
/******************************* TSL2561 *******************************/
void Read_TSL2561_Sensordata()
{
  uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE);
  Serial.print("VISIBLE:  ");
  Serial.println(x, DEC);
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  Serial.print("IR: "); Serial.print(ir);   Serial.print("\t\t");
  Serial.print("Full: "); Serial.print(full);   Serial.print("\t");
  Serial.print("Visible: "); Serial.print(full - ir);   Serial.print("\t");
  
  Serial.print("Lux: "); Serial.println(tsl.calculateLux(full, ir));
  if(Display_Flag==1)
  {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("TSL2561 Light Sensor:");
  display.print("VISIBLE: ");
  display.println(x);
  display.print("IR: ");
  display.print(ir);
  display.print("   Full:  ");
  display.println(full);
  display.print("Visible: ");
  display.print(full-ir);
  display.print(" Lux: ");
  display.print(tsl.calculateLux(full, ir));
  display.display();
  }
  }
/******************************* CCS811 *******************************/
void Read_CCS811_Sensordata()
{
  if (mySensor.dataAvailable())
  {
    //If so, have the sensor read and calculate the results.
    //Get them later
    mySensor.readAlgorithmResults();
    uint16_t CO2=mySensor.getCO2();
    uint16_t tVOC=mySensor.getTVOC();
    Serial.print("CO2[");
    //Returns calculated CO2 reading
    Serial.print(CO2);
    Serial.print("] tVOC[");
    //Returns calculated TVOC reading
    Serial.print(tVOC);
    Serial.println("]");
    if(Display_Flag==6)
    {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("CCS811 Sensor:");
    display.print("CO2: ");
    display.println(CO2);
    display.print("tVOC:");
    display.println(tVOC);
    display.display();
    }
  }
  }
/******************************* Loundness *******************************/
void loundness_ledOn() {
  Wire.beginTransmission(Loundness_Address);
  Wire.write(COMMAND_LED_ON);
  Wire.endTransmission();
}
void loundness_ledOff() {
  Wire.beginTransmission(Loundness_Address);
  Wire.write(COMMAND_LED_OFF);
  Wire.endTransmission();
}
void get_loundness_value() {
  uint8_t dbA=0;
  loundness_ledOff();
  Wire.beginTransmission(Loundness_Address);
  Wire.write(COMMAND_GET_VALUE); // command for status
  Wire.endTransmission();    // stop transmitting //this looks like it was essential.

  Wire.requestFrom(Loundness_Address, 2);    // request 1 bytes from slave device qwiicAddress

  while (Wire.available()) { // slave may send less than requested
  uint8_t ADC_VALUE_L = Wire.read(); 
  uint8_t ADC_VALUE_H = Wire.read();
  ADC_VALUE=ADC_VALUE_H;
  ADC_VALUE<<=8;
  ADC_VALUE|=ADC_VALUE_L;
  dbA=map(ADC_VALUE,0,1023,0,140);
  Serial.print("Loundness_VALUE(Max:1023):  ");
  Serial.print(ADC_VALUE,DEC);
  Serial.print("  Approx:  ");
  Serial.print(dbA);
  Serial.println("db");
  }
  uint16_t x=Wire.read(); 
  if(Display_Flag==5)
    {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("Loundness Sensor:");
    display.print("(Max:1023)  ");
    display.println(ADC_VALUE);
    display.print("Approx:  ");
    display.print(dbA);
    display.print(" db");
    display.display();
    }
}
void testForConnectivity() {
  Wire.beginTransmission(Loundness_Address);
  //check here for an ACK from the slave, if no ACK don't allow change?
  if (Wire.endTransmission() != 0) {
    Serial.println("Check connections. No slave attached.");
    while (1);
  }
}

/******************************* SHT31 *******************************/
void Read_SHT31_Sensordata()
{
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.println(t);
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = "); Serial.println(h);
  } else { 
    Serial.println("Failed to read humidity");
  }
  if(Display_Flag==4)
    {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("SHT31 Sensor:");
    display.print("Temp *C = ");
    display.println(t);
    display.print("Hum. %  = ");
    display.print(h);
    display.display();
    }
  }

/******************************* BMP280 *******************************/
void Read_BMP280_Sensordata()
{
  Serial.print(F("Temperature = "));
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");
  
  Serial.print(F("Pressure ="));
  Serial.print(bmp.readPressure());
  Serial.println("Pa");
  
  Serial.print(F("Approx altitude = "));
  Serial.print(bmp.readAltitude(1013.25)); // this should be adjusted to your local forcase
  Serial.println(" m");
  if(Display_Flag==3)
    {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("BMP280 Sensor:");
    display.print("Temperature=");
    display.print(bmp.readTemperature());
    display.println(" *C");
    display.print("Pressure =");
    display.print(bmp.readPressure());
    display.println("Pa");
    display.print("altitude =");
    display.print(bmp.readAltitude(1013.25));
    display.println(" m");
    display.display();
    }
  }
/******************************* PM2.5 *******************************/
void Read_PM_Sensordata()
{
  Wire.requestFrom(PM_address, 32);
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
    if(Display_Flag==2)
    {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("PMSA003I Sensor:");
    display.print("pm1.0: ");
    display.print(pm1_0);
    display.println("ug/m3");
    display.print("pm2.5: ");
    display.print(pm2_5);
    display.println("ug/m3");
    display.print("pm10 : ");
    display.print(pm10);
    display.println("ug/m3");
    display.display();
    }
  }

