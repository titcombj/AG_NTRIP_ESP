#include <dummy.h>

TaskHandle_t Core1;
TaskHandle_t Core2;
// ESP32 Ntrip Client by Coffeetrac
// Release: V1.26
// 01.01.2019 W.Eder
// Enhanced by Matthias Hammer 12.01.2019
//##########################################################################################################
//### Setup Zone ###########################################################################################
//### Just Default values ##################################################################################
struct Storage{
  
  char ssid[24]        = "yourSSID";          // WiFi network Client name
  char password[24]    = "YourPassword";      // WiFi network password
  unsigned long timeoutRouter = 65;           // Time (seconds) to wait for WIFI access, after that own Access Point starts 

  // Ntrip Caster Data
  char host[40]        = "192.168.0.149";    // Server IP
  int  port            = 2101;                // Server Port
  char mountpoint[40]  = "POD0";   // Mountpoint
  char ntripUser[40]   = "test";     // Username
  char ntripPassword[40]= "test";    // Password

  byte sendGGAsentence = 0; // 0 = No Sentence will be sended
                            // 1 = fixed Sentence from GGAsentence below will be sended
                            // 2 = GGA from GPS will be sended
  
  byte GGAfreq =10;         // time in seconds between GGA Packets

  char GGAsentence[100] = "$GPGGA,051353.171,4751.637,N,01224.003,E,1,12,1.0,0.0,M,0.0,M,,*6B"; //hc create via www.nmeagen.org
  
  long baudOut = 38400;     // Baudrate of RTCM Port

  byte send_UDP_AOG  = 0;   // 0 = Transmission of NMEA Off
                            // 1 = Transmission of NMEA Sentences to AOG via Ethernet-UDP

  byte enableNtrip   = 1;   // 0 = NTRIP disabled
                            // 1 = ESP NTRIP Client enabled
                            // 2 = AOG NTRIP Client enabled (Port=2233)
  
  byte AHRSbyte      = 0;   // 0 = No IMU, No Inclinometer
                            // 1 = BNO055 IMU installed
                            // 2 = MMA8452 Inclinometer installed
                            // 3 = BNO055 + MMA 8452 installed
}; Storage NtripSettings;

//##########################################################################################################
//### End of Setup Zone ####################################################################################
//##########################################################################################################

boolean debugmode = false;

// IO pins --------------------------------
#define RX0      3
#define TX0      1

#define RX1     26  //simpleRTK TX(xbee) = RX(f9p)
#define TX1     25  //simpleRTK RX(xbee) = TX(f9p)

#define RX2     17  
#define TX2     16 

#define SDA     21  //I2C Pins
#define SCL     22

#define LED_PIN_WIFI   32   // WiFi Status LED

//########## BNO055 adress 0x28 ADO = 0 set in BNO_ESP.h means ADO -> GND
//########## MMA8451 adress 0x1D SAO = 0 set in MMA8452_AOG.h means SAO open (pullup!!)

#define restoreDefault_PIN 36  // set to 1 during boot, to restore the default values


//libraries -------------------------------
#include <Wire.h>
#include <WiFi.h>
#include <base64.h>
#include "Network_AOG.h"
#include "EEPROM.h"
//#include "BNO_ESP.h"
//#include "MMA8452_AOG.h"

// Declarations
void DBG(String out, byte nl = 0);

//Accesspoint name and password:
const char* ssid_ap     = "NTRIP_Client_ESP_Net";
const char* password_ap = "";

//static IP
IPAddress myip(192, 168, 0, 79);  // Roofcontrol module
IPAddress gwip(192, 168, 0, 1);   // Gateway & Accesspoint IP
IPAddress mask(255, 255, 255, 0);
IPAddress myDNS(8, 8, 8, 8);      //optional

unsigned int portMy = 5544;       //this is port of this module: Autosteer = 5577 IMU = 5566 GPS = 
unsigned int portAOG = 8888;      // port to listen for AOG
unsigned int portMyNtrip = 2233;

//IP address to send UDP data to:
IPAddress ipDestination(192, 168, 0, 255);
unsigned int portDestination = 9999;  // Port of AOG that listens

// Variables ------------------------------
// WiFistatus LED 
// blink times: searching WIFI: blinking 4x faster; connected: blinking as times set; data available: light on; no data for 2 seconds: blinking
unsigned int LED_WIFI_time = 0;
unsigned int LED_WIFI_pulse = 700;   //light on in ms 
unsigned int LED_WIFI_pause = 700;   //light off in ms
boolean LED_WIFI_ON = false;
unsigned long Ntrip_data_time = 0;

// program flow
bool AP_running=0, EE_done = 0, restart=0;
int value = 0; 
unsigned long repeat_ser;   
//int error = 0;
unsigned long repeatGGA, lifesign, aogntriplife;

//loop time variables in microseconds
const unsigned int LOOP_TIME = 100; //10hz 
unsigned int lastTime = LOOP_TIME;
unsigned int currentTime = LOOP_TIME;
unsigned int dT = 50000;


// GPS-Bridge
int cnt=0;
int i=0;  
byte gpsBuffer[100], c;
bool newSentence = false;
char lastSentence[100]="";

char strmBuf[512];         // rtcm Message Buffer

//Array to send data back to AgOpenGPS
byte GPStoSend[100]; 

// Instances ------------------------------
//MMA8452 accelerometer;
WiFiServer server(80);
WiFiClient ntripCl;
WiFiClient client_page;
//JTI- udpRoof is for sending the GPS data to. In our case this needs to be the relative to the BasePod
AsyncUDP udpRoof;
//JTI- udpNtrip is where we get the Ntrip data from. This is the caster on the BasePod
AsyncUDP udpNtrip;


// Setup procedure ------------------------
void setup() {
  pinMode(restoreDefault_PIN, INPUT);  //
  
  restoreEEprom();
//  Wire.begin(SDA, SCL, 400000);

  //  Serial1.begin (NtripSettings.baudOut, SERIAL_8N1, RX1, TX1); 
  if (debugmode) { Serial1.begin(38400, SERIAL_8N1, RX1, TX1); } //set new Baudrate
  else { Serial1.begin(NtripSettings.baudOut, SERIAL_8N1, RX1, TX1); } //set new Baudrate
  Serial2.begin(NtripSettings.baudOut,SERIAL_8N1,RX2,TX2); 

  Serial.begin(115200);

 pinMode(LED_PIN_WIFI, OUTPUT);
   
  //------------------------------------------------------------------------------------------------------------  
  //create a task that will be executed in the Core1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Core1code, "Core1", 10000, NULL, 1, &Core1, 0);
  delay(500); 
  //create a task that will be executed in the Core2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Core2code, "Core2", 10000, NULL, 1, &Core2, 1); 
  delay(500); 
  //------------------------------------------------------------------------------------------------------------
 
}

void loop() {
}

//--------------------------------------------------------------
//  Debug Messaging
//--------------------------------------------------------------
bool debug = 1;  // Print Debug Messages to Serial0

void DBG(String out, byte nl){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}

void DBG(int out, byte nl = 0){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}

void DBG(long out, byte nl = 0){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}

void DBG(char out, byte nl = 0){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}

void DBG(char out, byte type, byte nl = 0){ // type = HEX,BIN,DEZ..
  if (debug == 1) {
    if (nl) Serial.println(out,type);
    else Serial.print(out,type);
  }
}

void DBG(IPAddress out, byte nl = 0){
  if (debug == 1) {
    if (nl) Serial.println(out);
    else Serial.print(out);
  }
}
