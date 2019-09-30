// Your GPRS credentials (leave empty, if missing)
#define CONFIG_ESP32_ENABLE_COREDUMP_TO_FLASH 1
const char apn[]      = "orange"; // Your APN
const char gprsUser[] = "orange"; // User
const char gprsPass[] = "orange"; // Password
const char simPIN[]   = "0000"; // SIM card PIN code, if any
const char Mynumber[] = "+336XXXXXXX";
bool       newSMS     = false ;
char       msg[1000]   = "" ;
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SSD1306_ADDR         0x3C
Adafruit_SSD1306 display(128, 32, &Wire, -1);

// TTGO T-Call pin definitions
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22
#define IP5306_ADDR                 0X75
#define IP5306_REG_SYS_CTL0         0x00
#define IP5306_REG_SYS_CTL1         0x01
#define IP5306_REG_SYS_CTL2         0x02
#define IP5306_REG_READ0 	    0x70
#define IP5306_REG_READ1 	    0x71
#define IP5306_REG_READ3 	    0x78  

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)
#define SerialAT  Serial1

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb

// Define the serial console for debug prints, if needed
//#define TINY_GSM_DEBUG SerialMon
#define DUMP_AT_COMMANDS
#include <Wire.h>
#include <TinyGsmClient.h>
#include <HTTPClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
//Define Wifi
#include <WiFi.h>
const char* ssid = "Livebox";                   
const char* password =  "XXXXX";

const char server[] = "httpbin.org";
const char resource[] = "/get";
const int  port = 80;

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
HttpClient http(client, server, port);

static const unsigned char PROGMEM iconeTel [] =
{
0x18, 0x00, 0x38, 0x00, 0x7c, 0x00, 0x7c, 0x00, 0x7c, 0x00, 0x78, 0x00, 0x70, 0x00, 0x78, 0x00,
0x38, 0x00, 0x3c, 0x00, 0x1e, 0x18, 0x0f, 0x3c, 0x07, 0xfe, 0x03, 0xfe, 0x01, 0xfc, 0x00, 0x78
};

bool isCharging() {
	
 uint8_t data;
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_READ0);
  Wire.endTransmission(false);
  Wire.requestFrom(IP5306_ADDR, 1) ;
  data = Wire.read() & 0x08  ;
  Wire.endTransmission() ;
 return data ;
}

bool isChargeFull() {

 uint8_t data;
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_READ1);
  Wire.endTransmission(false);
  Wire.requestFrom(IP5306_ADDR, 1);
  data = Wire.read() & 0x08 ;
  Wire.endTransmission() ;
 return data ;

}

int8_t getBatteryLevel() {
 
 uint8_t data;
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_READ3);
  Wire.endTransmission(false);
  Wire.requestFrom(IP5306_ADDR, 1);
  data = Wire.read()  ;
  Wire.endTransmission() ;
  switch (data & 0xF0) {
        case 0x00:
	        return 100;
	      case 0x80:
	        return 75;
	      case 0xC0:
	        return 50;
	      case 0xE0:
	        return 25;
	      default:
	        return 0;
    }
}

void  setPowerBoostKeepOn(){

  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  Wire.endTransmission() ;
  
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL1);
  Wire.write(0x1D);
  Wire.endTransmission() ;
  
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL2);
  Wire.write(0x64);
  Wire.endTransmission();
  
}

void poweron_GSM() {
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  SerialMon.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem: ");
  SerialMon.println(modemInfo);

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }
  delay(3000);
}

void poweroff_gsm() {
  digitalWrite(MODEM_POWER_ON, LOW);
}

bool checkNetwork() {
 return (modem.isNetworkConnected());
}

bool connect_GSM() {
  
  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(240000L)) {
    SerialMon.println(" fail");
    delay(10000);
    return false;
  }
  SerialMon.println(" OK");

  if (!modem.isNetworkConnected()) {
    return false;
  }
  modem.receiveSMS();
 
  SerialMon.println("Network connected");
  return true ; 
}

void display_OLED(char* sms, bool newSMS, bool isconnected, bool ischarging ) {
  
 String Mysms = String(sms);
 Mysms.trim();
 Serial.println("------------ ");
 Serial.print("dump stream is: ");
 Serial.println(Mysms);
 Serial.println("------------ ");
 display.clearDisplay();
 display.setCursor(0,15);
 if (Mysms.substring(0, 4).equals("+CMT")) {
  display.print(Mysms.substring(46));
  //Serial.println(Mysms.substring(46));
 // Serial.println(Mysms.substring(0, 4));
 }
 
 if (Mysms.substring(0, 4).equals("RING")) {
  display.drawBitmap(90, 5, iconeTel , 16, 16, 1);
 // Serial.println(Mysms.substring(0, 4));
 }

 if (!isconnected) {
    delay(800);
    display.setCursor(0,0);
    display.print("Not connected!");
    } else {
    delay(800);
    display.setCursor(0,0);
    display.print("GSM connected!");
 };
 if (!ischarging) {
    delay(800);
    display.setCursor(0,10);
    display.print("Power Outage!");
 } else {
    delay(800);
    display.setCursor(0,10);
    display.print("Power OK!");
 };
 static int alive = 1 ;
 display.setCursor(80,10);
 switch (alive) {
	case 1:
	display.print("|");
	break;
	case 2:
	display.print("/");
	break;
	case 3:
	display.print("-");
	break;
	case 4:
	display.print("\\");
	alive = 0 ;
	break;
 }
 alive++;
 display.display();

}

void showSMS() {
  int i = 0;
   while (SerialAT.available()) {
   msg[i] = SerialAT.read();
   i++;
   newSMS = true;
   }
   
   if (newSMS) {
   msg[i] = '\0';
   newSMS = false;
   };
}

bool checkLan() {
  HTTPClient http2;
  http2.begin("http://httpbin.org/ip"); //HTTP
  int httpCode = http2.GET();
  if(httpCode > 0) {
   // HTTP header has been send and Server response header has been handled
    if(httpCode == HTTP_CODE_OK) {
      String payload = http2.getString();
      StaticJsonDocument<200> doc; // <- a little more than 200 bytes in the stack
      deserializeJson(doc, payload);
      const char* IPLAN  = doc["origin"]; 
      Serial.println("IP LAN is " + ((String(IPLAN)).substring(0, 14)));
      return true;
      } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http2.errorToString(httpCode).c_str());
      return false;
     }
  }
 return false;
}

bool checkWan() {
  int err = http.get(resource);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    delay(9999);
    return false; 
  }

  int status = http.responseStatusCode();
  if (!status) {
    delay(10000);
    return false;
  }

  String body = http.responseBody();
  StaticJsonDocument<200> doc; // <- a little more than 200 bytes in the stack
  deserializeJson(doc, body);
  const char* IPWAN  = doc["origin"]; 
  Serial.println("IP WAN is " + ((String(IPWAN)).substring(0, 13)));		
  http.stop();
  return true;
}


void do_in_loop(){
// showSMS();  
 bool isconnected = checkNetwork();
 bool charging = isCharging();
 if (!isconnected) {
  poweron_GSM();
  connect_GSM();
 }
 
 display_OLED(msg, newSMS, isconnected ,charging );
    
    String infoPower = "Is Charging?" + String ((charging ? " YES" : " NO")); 
    String infoPower2 = "Is Charge full?" + String ((isChargeFull() ? " YES" : " NO")); 
    String infoPower3 = "Battery Level: " + String(getBatteryLevel()) + "%" ;  
    Serial.println(infoPower);
    Serial.println(infoPower2);
    Serial.println(infoPower3);
    Serial.println("");

 static int t = 0;
 if (t >= 5) {  
 checkLan();
 checkWan(); 
 t = 0;
 }
 t++;
 
}


void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);
  // Set-up modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  setPowerBoostKeepOn();
   
  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  // Start GSM module
  poweron_GSM();
  while (!connect_GSM()){};
  delay(2);
  
  // Set gprs
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      Serial.println(" fail");
      delay(10000);
      return;
    }
    Serial.println(" success");

    if (modem.isGprsConnected()) {
      Serial.println("GPRS connected");
   }
  
  // Start Wifi
  WiFi.mode(WIFI_STA);                                           
  WiFi.begin(ssid, password);                                    
                                                               
  while (WiFi.status() != WL_CONNECTED) {                        
    delay(500);                                                
  Serial.println("Connecting to WiFi..");       
  }                                                            
                                                               
  Serial.println("Connected to the WiFi network");  
  Serial.print("IP: ");          
  Serial.println(WiFi.localIP());

  // Set-up the display
  display.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDR);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.display(); 
}



void loop() {
  do_in_loop();
  static bool isfirst = true;
  static bool Charging_cur;
  if (isfirst && !isCharging()) {
      modem.sendSMS(Mynumber, String("Power Outage detected!") );
      isfirst = false ;
  } 
  if (( isCharging() ) && (getBatteryLevel() != 0) ){
  //   poweron_GSM();
     while (!connect_GSM()) {};
    // modem.sendSMS(Mynumber, String("Power is back!") );
//   modem.sendFlashSMS("07913386....4F4F29C0E", 17);
     Charging_cur = true ;
  }

  while (Charging_cur) {
    do_in_loop();
    if (!isCharging()) {
      setPowerBoostKeepOn();
      poweron_GSM();
      while (!connect_GSM()) {};
      isfirst = false ;
      modem.sendSMS(Mynumber, String("Power Outage detected!") );
      delay(3000);
      Charging_cur = false;
    }
   delay(5000);
  }
 delay(10000);
}
