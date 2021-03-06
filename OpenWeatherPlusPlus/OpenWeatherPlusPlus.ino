#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>
#include <SPI.h>
#include <WProgram.h>

#include <OneWire.h>
#include <DS2438.h> //OneWire Battary Monitor found in HobbyBoards Tempurature, Humidity & Solar sensor, as well as Barometer
#include <DS2409.h> //OneWire hub IC
#include <DS2423.h> //Lightning Sensor
#include <Time.h> 
#include <Serial.h>
#ifndef NULL
#define NULL (void *)0
#endif

//Pins
#define PIN_ANEMOMETER  2     // Digital 2
#define PIN_VANE        5     // Analog 5
#define PIN_RAIN        3     // Digital 3
#define PIN_LED_ONE     13    // Digital 13
OneWire oneWire(7); //OneWire on Digital 4

//Ethernet
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x98, 0x2B }; //MAC address of ethernet sheild
byte server[] = { 128, 206, 9, 212}; // Google
EthernetClient client;

//OneWire Addresses
DeviceAddress barometer_add = { 0x26, 0xE8, 0x99, 0xBC, 0x00, 0x00, 0x00, 0x2E}; //Address of barometer
DeviceAddress humidity_temp_solar_add = { 0x26,0x8A,0x9F,0x21,0x01,0x00,0x00,0xC8 }; //This address can be found with the oneWire.search method or the method oneWire provided in the oneWire2408 walk switch example. This utility is provided as a sketch in the library (It is set to use digital pin 10)
DeviceAddress lightning_add = { 0x1D, 0xF1, 0xDF, 0x0F, 0x00, 0x00, 0x00, 0x2D }; //Lightning sensor module
byte oneWire18x20_address[] = {0x10,0x0F,0x43,0x5B,0x02,0x08,0x00,0x0D}; //DS1820 temp sensor


#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
#define uint  unsigned int
#define ulong unsigned long
// How often we want to calculate wind speed or direction
#define MSECS_CALC_WIND_SPEED 1000
#define MSECS_CALC_WIND_DIR   1000
#define MSECS_CALC_RAIN 1000
volatile int numRevsAnemometer = 0; // Incremented in the interrupt
volatile int numRainDrops = 0; // Incremented in the interrupt
ulong nextCalcSpeed;                // When we next calc the wind speed
ulong nextCalcDir;                  // When we next calc the direction
ulong nextCalcRain;                 // When we next calc the rainfall
ulong time;                         // Millis() at each start of loop().
#define NUMDIRS 8
ulong   adc[NUMDIRS] = {26, 45, 77, 118, 161, 196, 220, 256};
// These directions match 1-for-1 with the values in adc, but
// will have to be adjusted as noted above. Modify 'dirOffset'
// to which direction is 'away' (it's West here).
char *strVals[NUMDIRS] = {"W","NW","N","SW","NE","S","SE","E"};
byte dirOffset=0;

ds2438 hum_temp_sol(&oneWire, humidity_temp_solar_add); //Create the instance of the ds2438 class for humidty, temp and solar module
ds2438 barometer(&oneWire, barometer_add); //Create the instance of the ds2438 class for the barmometer
ds2423 lightning(&oneWire, lightning_add); //Create the instance of the ds2423 lightning sensor

float wspeed, precip;
byte wdir;


void setup(void)
{
  Serial.begin(9600);
  Ethernet.begin(mac);
  delay(1000);
  Serial.println("connecting...");
  Ethernet.maintain();

  
  hum_temp_sol.writeSetup(0x08); //Set register to read humidity over solar
  pinMode(PIN_ANEMOMETER, INPUT);
  pinMode(PIN_RAIN, INPUT);
  pinMode(PIN_LED_ONE, OUTPUT);
  digitalWrite(PIN_ANEMOMETER, HIGH);
  digitalWrite(PIN_RAIN, HIGH);
  attachInterrupt(0, countAnemometer, FALLING);
  attachInterrupt(1, countRain, CHANGE);
  nextCalcSpeed = millis() + MSECS_CALC_WIND_SPEED;
  nextCalcDir   = millis() + MSECS_CALC_WIND_DIR;
  nextCalcRain = millis() + MSECS_CALC_RAIN;
  unsigned long startTime = millis();
  delay(100);
}

void loop(void)
{
//  Serial.print("Temp F (DS1820): ");
//  Serial.print(getTemp());
//  Serial.println();
//  Serial.print("Temp F (2438): ");
//  Serial.print(hum_temp_sol.readTempF());
//  Serial.println();
//  Serial.print("% RH: ");
//  Serial.print(hum_temp_sol.readHum()); //Note that readHum may need to be modified if your sensor deviates from the formulas used with the HIH-4000
//  Serial.println();
//  Serial.print("Solar Current in mA: ");
//  Serial.print(hum_temp_sol.readCurrent());
//  Serial.println();
//  Serial.print("Solar Energy W/M^2: ");
//  Serial.print(hum_temp_sol.readCurrent()*.0001*1157598);
//  Serial.println();
//  Serial.print("Barometric Pressure (kPa): ");
//  Serial.print(barometer.readPressure());
//  Serial.println();
  time = millis();
  if (time >= nextCalcSpeed) {
      wspeed = calcWindSpeed();
      nextCalcSpeed = time + MSECS_CALC_WIND_SPEED;
  }
  if (time >= nextCalcDir) {
      wdir = calcWindDir();
      nextCalcDir = time + MSECS_CALC_WIND_DIR;
  }
  if (time >= nextCalcRain) {
      precip = calcRainFall();  
      nextCalcRain = time + MSECS_CALC_RAIN;
      numRainDrops = 0; //Reset the counter
  }
//  delay(15000);
delay(10000);
  
      if (client.connect(server, 80)) 
  {
    Serial.println("connected");
    client.print("GET /~twc9bc/capstone/insert.php?");
  } 
  else 
  {
    Serial.println("connection failed");
  }
  
  client.print("temp=");
  Serial.print("temp=");
  client.print(getTemp());
  Serial.println(getTemp());
  
  client.print("&hum=");
  Serial.print("&hum=");
  client.print(hum_temp_sol.readHum());
  Serial.println(hum_temp_sol.readHum());
  
  client.print("&solar=");
  Serial.print("&solar=");
  client.print(hum_temp_sol.readCurrent()*.0001*1157598);
  Serial.println(hum_temp_sol.readCurrent()*.0001*1157598);
  
  client.print("&wspeed=");
  Serial.print("&wspeed=");
  client.print(wspeed);
  Serial.println(wspeed);
  
  client.print("&wdir=");
  Serial.print("&wdir=");
  client.print(strVals[wdir]);
  Serial.println(strVals[wdir]);
  
  client.print("&prec=");
  Serial.print("&prec=");
  client.print(precip);
  Serial.println(precip);
  
  client.print("&press=");
  Serial.print("&press=");
  client.print(barometer.readPressure());
  Serial.println(barometer.readPressure());

  client.print("&light=");
  Serial.print("&light=");
  client.print(lightning.readCounter(2));
  Serial.println();
  
  client.println(" HTTP/1.1");
  Serial.println(" HTTP/1.1");
  client.println("Host: babbage.cs.missouri.edu");
  Serial.println("Host: babbage.cs.missouri.edu");
  client.println("User-Agent: Arduino");
  Serial.println("User-Agent: Arduino");
  client.println("Accept: text/html");
  Serial.println("Accept: text/html");
  client.println("Connection: close");
  Serial.println("Connection: close"); 
  client.println();
  Serial.println();
  client.stop();
  
}

//function to retrieve temp in cel from oneWire18s20 or oneWire1820
float getTemp(){
  byte i;
  byte data[12];
  int Temp;
  oneWire.reset();
  oneWire.select(oneWire18x20_address);
  oneWire.write(0x44,1);	   // start conversion, with parasite power on at the end
  // Resolution	            9 bit	10 bit	11 bit	12 bit
  // Conversion Time (ms)	93.75	187.5	375	    750
  // LSB (°C)	            0.5	    0.25	0.125	0.0625
  delay(750);     
  oneWire.reset();
  oneWire.select(oneWire18x20_address);    
  oneWire.write(0xBE);	   // Read Scratchpad
  for ( i = 0; i < 9; i++) {	     // we need 9 bytes (9 byte resolution on oneWire1820 & oneWire18s20)
    data[i] = oneWire.read();
  }
  Temp=(data[1]<<8)+data[0];//take the two bytes from the response relating to temperature
  Temp=Temp>>1;//divide by 16 to get pure celcius readout
  Temp=Temp*1.8+32; // comment this line out to get fahrenheit 
  float t = Temp;
  return t;
}
//=======================================================
// Interrupt handler for anemometer. Called each time the reed
// switch triggers (one revolution).
//=======================================================
void countAnemometer() {
   numRevsAnemometer++;
}
//=======================================================
// Interrupt handler for rain fall. Called each time the well
// switch triggers.
//=======================================================
void countRain() {
   numRainDrops++;
}
//=======================================================
// Find vane direction.
//=======================================================
byte calcWindDir() {
   int val;
   byte x, reading;
   val = analogRead(PIN_VANE);
   val >>=2;                        // Shift to 255 range
   reading = val;
   // Look the reading up in directions table. Find the first value
   // that's >= to what we got.
   for (x=0; x<NUMDIRS; x++) {
      if (adc[x] >= reading)
         break;
   }
   x = (x + dirOffset) % 8;   // Adjust for orientation
//   return strVals[x];
   return x;
  
}
//=======================================================
// Calculate the wind speed, and display it (or log it, whatever).
// 1 rev/sec = 1.492 mph
//=======================================================
float calcWindSpeed() {
   int x, iSpeed;
   // This will produce mph * 10
   long speed = 14920;
   speed *= numRevsAnemometer;
   speed /= MSECS_CALC_WIND_SPEED;
   iSpeed = speed;         // Need this for formatting below
   x = iSpeed / 100;
   x = iSpeed % 10;
   numRevsAnemometer = 0;        // Reset counter
   return(x);
}
//=======================================================
// Calculate Rain Fall.
// 1 interrupt = .011 inches of precipitation
//=======================================================

float calcRainFall() {
   return(numRainDrops * .011);
}
