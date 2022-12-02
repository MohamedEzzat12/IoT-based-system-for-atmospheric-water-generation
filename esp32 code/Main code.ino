#include <WiFi.h>
#include "DFRobot_ESP_PH.h"
#include "EEPROM.h"
const char* ssid = "YOUR WI-FI SSID";
const char* password = "YOUR PASSWORD";
WiFiServer server(8888);
String header;
#define TdsSensorPin 34
#define VREF 3.3      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0;
DFRobot_ESP_PH ph;
#define ESPADC 4096.0   //the esp Analog Digital Convertion value
#define ESPVOLTAGE 3300 //the esp voltage supply value
#define PH_PIN 35    //the esp gpio data pin number
float voltage, phValue, temperature = 25;
unsigned long t=0, duration=0;
#define motor_A1 23
#define motor_dr1 22
char x;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
void setup() {
 t=millis();
    Serial.begin(115200);
  Serial.println("For drinkable water send: d");//print on serial monitor  
  Serial.println("For agriculture water send: a"); //print on serial monitor 
  ph.begin();
  pinMode(motor_A1, OUTPUT);
    pinMode(motor_dr1,OUTPUT);
    Serial.print("Connecting to ");
Serial.println(ssid);
WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
Serial.println("");
Serial.println("WiFi connected.");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());
server.begin();
}
void pH(){
    static unsigned long timepoint = millis();
  if (millis() - timepoint > 1000U) //time interval: 1s
  {
    timepoint = millis();
    //voltage = rawPinValue / esp32ADC * esp32Vin
    voltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE; // read the voltage
    /*Serial.print("voltage:");
    Serial.println(voltage, 4);
    temperature = readTemperature();  // read your temperature sensor to execute temperature compensation
    Serial.print("temperature:");
    Serial.print(temperature, 1);
    Serial.println("^C");
*/
  phValue = ph.readPH(voltage, temperature); // convert voltage to pH with temperature compensatio
    Serial.println("pH = " + String(phValue, 4));
     }
  ph.calibration(voltage, temperature); // calibration process by Serail CMD
}
void TDS(){
  static unsigned long analogSampleTimepoint = millis();
   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }   
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      //Serial.print("      \t TDS Value:  ppm");
      Serial.println( "TDS = "+String(tdsValue,0)+ " ppm");
      //Serial.print("      \t");
   }
}
int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
      for (i = 0; i < iFilterLen - j - 1; i++) 
          {
        if (bTab[i] > bTab[i + 1]) 
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;}
void agriculture_ON(){
  digitalWrite(motor_A1,HIGH);
  }

void agriculture_OFF(){
  digitalWrite(motor_A1,LOW);
  }
void drinkable_ON(){
  digitalWrite(motor_dr1,HIGH);
 }
void drinkable_OFF(){
  digitalWrite(motor_dr1,LOW);
 }
 void sensors(){
 duration=millis()-t;
 if(duration>=1000){
  TDS();
 pH();
 t=millis();
 } 
 }
void loop() {
x=Serial.read();
sensors();
   WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // checking if header is valid
            // dXNlcjpwYXNz = 'user:pass' (user:pass) base64 encode
            // Finding the right credential string, then loads web page
            if(header.indexOf("eW91c2VmOjI3NzIwMDU=") >= 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
           // Display the HTML web page
        client.println("<!DOCTYPE html>");
client.println("<html lang=\"en\">");
client.println("<head><meta charset=\"UTF-8\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Document</title><style>* {margin: 0;padding: 0;box-sizing: border-box;}");
client.println("body {width: 100%;height: 1000px;background: #eee;color: #333;display: flex;flex-direction: column;justify-content:space-evenly;align-items: center; }");
client.println("table {width: 400px;height: 250px;}");
client.println("tr:nth-child(1){background-color: rgb(68, 66, 66);color: #fff;}");
client.println("tr{background-color: #fff;}");
client.println("td {text-align: center;}");
client.println("@media screen and (max-width: 1055px) {h2{font-size: 3vw;}table {width: 90%;}iframe {width: 95%;}}");
client.println("</style>");
client.println("</head><body><h2>IoT Based Solar System For Atmospheric Water Generation</h2><table><tr><th>Parameter</th><th>Value</th></tr><tr><td>pH</td><td>");
client.println(phValue);
client.println("</td></tr><tr><td>TDS</td><td>");
client.println(tdsValue);
client.println("</td></table><td id=\"help\"><h3>To know more</h3><iframe width=\"1000\" height=\"500\" src=\"http://solar-desiccant-water.atwebpages.com/\" frameborder=\"1\"></iframe></td></body></html>");
// The HTTP response ends with another blank line
              client.println();
              // Break out of the while loop
              break;
            }
            // Wrong user or password, so HTTP request fails...   
            else {            
              client.println("HTTP/1.1 401 Unauthorized");
              client.println("WWW-Authenticate: Basic realm=\"Secure\"");
              client.println("Content-Type: text/html");
              client.println();
              client.println("<html>Authentication failed</html>");
            }   
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
if(x=='a'){
       Serial.println("preparing dirnkable water,please wait!");
      sensors();
      if(tdsValue<150){
        sensors();
        drinkable_ON();
        }
      else if( tdsValue>150){
                  sensors();
                  drinkable_OFF();}      
                }
   else if(x=='a'){
  Serial.println("Preparing agricultural water,please wait!");
  sensors();
      if(tdsValue<600){
        sensors();
        agriculture_ON();
        }
      else if( tdsValue>800){
                  sensors();
                  agriculture_OFF();
                                 }  
  }
  delay(20);
}
