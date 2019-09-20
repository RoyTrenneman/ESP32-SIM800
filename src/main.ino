// Your GPRS credentials (leave empty, if missing)
const char apn[]      = "orange"; // Your APN
const char gprsUser[] = "orange"; // User
const char gprsPass[] = "orange"; // Password
const char simPIN[]   = "0000"; // SIM card PIN code, if any
const char Mynumber[] = "+33XXXXXXXXX";
bool       Charging_cur;
bool       newSMS     = false ;
char       msg[100]   = "" ;
int 	   alive      = 1 ;
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

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);

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
 display.clearDisplay();
 display.setCursor(0,20);
 display.print(Mysms.substring(50));

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

void do_in_loop(){
 showSMS();  
 display_OLED(msg, newSMS, checkNetwork(), isCharging() );
    
    String infoPower = "Is Charging?" + String ((isCharging() ? " YES" : " NO")); 
    String infoPower2 = "Is Charge full?" + String ((isChargeFull() ? " YES" : " NO")); 
    String infoPower3 = "Battery Level: " + String(getBatteryLevel()) + "%" ;  
    Serial.println(infoPower);
    Serial.println(infoPower2);
    Serial.println(infoPower3);
    Serial.println("");

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
  
  // Set-up the display
  display.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDR);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.display(); 
}



void loop() {
  do_in_loop();
  
  if (( isCharging() ) && (getBatteryLevel() != 0) ){
     modem.sendSMS(Mynumber, String("Power is back!") );
     Charging_cur = true ;
  }

  while (Charging_cur) {
    do_in_loop();
    if (!isCharging()) {
      poweron_GSM();
      while (!connect_GSM()) {};
      modem.sendSMS(Mynumber, String("Power Outage detected!") );
      delay(3000);
      Charging_cur = false;
    }
   delay(5000);
  }
 delay(10000);
}
