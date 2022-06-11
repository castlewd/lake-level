#include <SoftwareSerial.h>
#include "ThingSpeak.h"
SoftwareSerial esp(2,3);

// ThingSpeak variable declarations
#define DEBUG true 
#define IP "184.106.153.149"// thingspeak.com ip
String Api_key = "GET /update?key=FMDGFU2OT2NR4QO1"; //change it with your api key like "GET /update?key=Your Api Key"

int error;
//const int sensor_pin = A0;
float temp;  
float output;  

// Thermistor variable declarations
const int ThermistorPin = 0; // if zero doesn't work try A0 
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

// Level sensor variable declarations
#define LAKE_LEVEL_ANALOG_PIN A2
#define RANGE 5000 // Depth measuring range 5000mm (for water)
#define VREF 5000 // ADC's reference voltage on your Arduino,typical value:5000mV
#define CURRENT_INIT 4.00 // Current @ 0mm (uint: mA)
#define DENSITY_WATER 1  // Pure water density normalized to 1
#define DENSITY_GASOLINE 0.74  // Gasoline density
#define PRINT_INTERVAL 1000

int16_t dataVoltage;
float dataCurrent, depth; //unit:mA
float sensorDepthOffset = 0; //TODO when sensor installed
unsigned long timepoint_measure;

void setup()
{ 
  Serial.begin(9600);
  
  // ESP Wifi Setup
  esp.begin(9600);
  send_command("AT+RST\r\n", 2000, DEBUG); //reset module
  send_command("AT+CWMODE=1\r\n", 1000, DEBUG); //set station mode
  send_command("AT+CWJAP=\"Winni\",\"ramparts\"\r\n", 2000, DEBUG);   //connect wifi network
  while(!esp.find("OK")) { //wait for connection
  Serial.println("Connected");} 
  
  // Lake Level Setup
  pinMode(LAKE_LEVEL_ANALOG_PIN, INPUT);
  timepoint_measure = millis();


}

void loop()
{
  // Three wire temp sensor
  //output=analogRead(sensor_pin);
  //temp =(output*500)/1023;
  
  temp = getThermistorTemp();
  depth = getLakeLevelDepth();
  startTemp: //label 
  error=0;
  
  updatedata(0);
  if (error==1){
    goto startTemp; //go to label "start"
  }
  delay(1000);

  startDepth: //label
  error=0;
  updatedata(1);
    if (error==1){
    goto startDepth; //go to label "start"
  }
  delay(1000);
}

void updatedata(int selector){
  String command = "AT+CIPSTART=\"TCP\",\"";
  command += IP;
  command += "\",80";
  Serial.println(command);
  esp.println(command);
  delay(2000);
  if(esp.find("Error")){
    return;
  }
  if(selector == 0)
  {
  command = Api_key ;
  command += "&field1=";   
  command += temp;
  command += "\r\n";
  }
  else if (selector == 1)
  {
    command = Api_key ;
    command += "&field2=";   
    command += depth;
    command += "\r\n";
  }
  else
  {
    // We used this wrong!
  }
  Serial.print("AT+CIPSEND=");
  esp.print("AT+CIPSEND=");
  Serial.println(command.length());
  esp.println(command.length());
  if(esp.find(">")){
    Serial.print(command);
    esp.print(command);
  }
  else{
    
   Serial.println("AT+CIPCLOSE");
   esp.println("AT+CIPCLOSE");
    //Resend...
    error=1;
  }
  }

String send_command(String command, const int timeout, boolean debug)
{
  String response = "";
  esp.print(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (esp.available())
    {
      char c = esp.read();
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);
  }
  return response;
}

float getThermistorTemp()
{
  Vo = analogRead(ThermistorPin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  T = (T * 9.0)/ 5.0 + 32.0; 
  return T;
  Serial.println("temp");
}

float getLakeLevelDepth()
{
  dataVoltage = analogRead(LAKE_LEVEL_ANALOG_PIN)/ 1024.0 * VREF;
  dataCurrent = dataVoltage / 120.0; //Sense Resistor:120ohm
  depth = (dataCurrent - CURRENT_INIT) * (RANGE/ DENSITY_WATER / 16.0); //Calculate depth from current readings

  if (depth < 0) 
  {
    depth = 0.0;
  }

  return depth + sensorDepthOffset; // Units : Feet 
}
