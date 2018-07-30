/*******************************************************
    Author: Lim Ren Yi & Benjamin Low
    Date: Jul 2018
    Title: Raitong Environment Sensor
    Description: Environment Sensor for Black Soldier Fly and Earthworm monitoring
                 Includes a DHT22 (temp and humidity), capacitive soil moisture
                 sensor and light sensor.

                 Ver2 uses full size D1 and includes the SD card & Real Time Clock (RTC) DS3231 module

    Connections: WeMos full size D1
        D0 - OLED RESET
        D1 (SCL) - OLED D0 & BH1750 SCL
        D2 (SDA) - OLED D1 & BH1750 SDA
        D3 - DHT22 
        D4 - button with inbuilt pullup resistor
        D5/SCK - SDcard SCK
        D6/MISO - SDcard MISO
        D7/MOSI - SDcard MOSI
        D8/SS - SDcard CS

    Power for components:
        BH1750 - 5V
        OLED - 3.3V
        DHT22 - 5V
        SDcard - 5V

    Notes:
    OLED: Need to remove R3 resistor for oled and short R1, R4, R5, R6
    In SSD1306.h:  #define SSD1306_128_64
*******************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <dht.h>
#include <Wire.h> //BH1750 IIC Mode
#include <Math.h>
#include <Button.h>
#include <SD.h>
#include <DS3231.h>

//user settings
const int OLEDDISPLAYDURATION = 5000;//how long in ms to display sensor data on OLED screen
const int DATALOGINTERVAL = 1800000; //time interval between data logs

//sensors
int BH1750address = 0x23; //setting i2c address
byte buff[2];
#define DHTPIN D3 // what digital pin we're connected to
dht DHT;


//OLED screen
#define OLED_RESET D0
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 48)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
bool isOLEDon;

//SD card module
const int chipSelect = D8;

//RTC DS3231 module
DS3231 Clock;
bool Century = false, PM, h12 = false;

//push button
Button myButton = Button(D4, PULLUP);

//global variables
unsigned long lastLoggedTime, buttonPressedTime;
float tempVal, humidVal;
int lightVal;

void setup()   {
  Wire.begin();
  Serial.begin(9600);

  // SD card
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) { // see if the card is present and can be initialized:
    Serial.println("Card failed, or not present");
    // don't do anything more:
    exit;
  }
  Serial.println("card initialized.");

  //OLED screen
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE); //need to set this
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Raitong\n Environ-\n ment\n Sensor");
  display.display();

  Serial.println("Raitong Environment Sensor");

  delay(4000);
  display.clearDisplay();
  display.display();

  //RTC DS3231
  Clock.setClockMode(h12); //12h format -> true, 24h format -> false
}

void loop() {

  if (myButton.uniquePress() && !isOLEDon) {

    delay(100); //debouncing
    buttonPressedTime = millis();
    isOLEDon = true;
    Serial.println("button press");

    //display all the sensor values on OLED screen
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0); 
    display.print("Temp: ");
    display.print(tempVal,1);
    display.println("\n");

    display.print("Hum: ");
    display.print(humidVal);
    display.println("\n");
    
    display.print("Lux:");
    display.print(lightVal, DEC);

    display.display();

    // print to the serial port too:
    Serial.println();
    Serial.print(millis());
    Serial.println(": Button pressed");
    Serial.println();
    /*
            Serial.print("Temp: ");
            Serial.print(tempVal);
            Serial.println(" degC");
            Serial.println("");

            Serial.print("Humidity: ");
            Serial.print(humidVal);
            Serial.println("%");
            Serial.println("");

            Serial.print("Light");
            Serial.print(" ");
            Serial.print(lightVal, DEC);
            Serial.println(" lux");
    */
  }

  if (millis() - buttonPressedTime > OLEDDISPLAYDURATION && isOLEDon) {
    //auto-shutdown OLED screen
    display.clearDisplay();
    display.display();
    isOLEDon = false;
  }

  if (millis() - lastLoggedTime > DATALOGINTERVAL) {

    update_sensor_values();

    String dataString = "";

    String timeStampString = get_timestamp();
    //    Serial.println(timeStampString);

    dataString = timeStampString + String(tempVal) + ", " + String(humidVal) + ", " + String(lightVal);

    File dataFile = SD.open("datalog.txt", FILE_WRITE);

    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.print("logging to SD card: ");
      Serial.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.txt");
    }

    lastLoggedTime = millis();
  }
}

void update_sensor_values() {
  //get all the sensor values
  get_DHT_reading();

  BH1750_Init(BH1750address);
  delay(200);

  if (2 == BH1750_Read(BH1750address)) lightVal = ((buff[0] << 8) | buff[1]) / 1.2;
  delay(150);
}

String get_timestamp() {
  //get back a string formatted as "DD/MM/YY,hh:mm,"
  String _year = String(Clock.getYear(), DEC);
  String _month = String(Clock.getMonth(Century), DEC);//not yet 2100
  String _day = String(Clock.getDate(), DEC);
  String _hour = String(Clock.getHour(h12, PM), DEC);
  if (_hour.toInt() < 10) _hour = "0" + _hour;
  String _minute = String(Clock.getMinute(), DEC);
  if (_minute.toInt() < 10) _minute = "0" + _minute;

  String timestamp = _day + "/" + _month + "/" + _year + ", " + _hour + ":" + _minute + ", ";

  return timestamp;
}

void get_DHT_reading() {
  int chk = DHT.read22(DHTPIN);

  switch (chk)
  {
    case DHTLIB_OK:
      //      Serial.print("OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      //      Serial.print("Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      //      Serial.print("Time out error,\t");
      break;
    default:
      //      Serial.print("Unknown error,\t");
      break;
  }
  humidVal = DHT.humidity;
  tempVal = DHT.temperature;
}

int BH1750_Read(int address) //
{
  int i = 0;
  Wire.beginTransmission(address);
  Wire.requestFrom(address, 2);
  while (Wire.available()) //
  {
    buff[i] = Wire.read();  // receive one byte
    i++;
  }
  Wire.endTransmission();
  return i;
}

void BH1750_Init(int address)
{
  Wire.beginTransmission(address);
  Wire.write(0x10);//1lx reolution 120ms
  Wire.endTransmission();
}


