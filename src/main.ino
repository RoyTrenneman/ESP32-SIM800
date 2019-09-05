// Your GPRS credentials (leave empty, if missing)
const char apn[]      = "orange"; // Your APN
const char gprsUser[] = "orange"; // User
const char gprsPass[] = "orange"; // Password
const char simPIN[]   = "0000"; // SIM card PIN code, if any
bool       Charging_cur;
bool       Charging ;
int        BatLevel ;

// TTGO T-Call pin definitions
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22
// TTGP T-CALL Register definition
#define IP5306_ADDR          0X75
#define IP5306_REG_SYS_CTL0  0x00
#define IP5306_REG_SYS_CTL1  0x01
#define IP5306_REG_SYS_CTL2  0x02
#define IP5306_REG_READ0     0x70
#define IP5306_REG_READ1     0x71
#define IP5306_REG_READ3     0x78  

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
}

void poweroff_gsm() {
  digitalWrite(MODEM_POWER_ON, LOW);
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
    SerialMon.println("Network connected");
    return true;
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
  poweron_GSM();
  connect_GSM();
}

void loop() {
  
   BatLevel = getBatteryLevel();
   Charging = isCharging();
   String infoPower = "Is Charging?" + String ((Charging ? " YES" : " NO")); 
   Serial.println(infoPower);
   String  infoPower3 = "Battery Level: " + String(BatLevel) + "%" ;  
   Serial.println(infoPower3);

  if ((Charging) && (BatLevel != 0) ){
     modem.sendSMS("+336xxxxxxx", String("Power is back!") );
     Charging_cur = true;
  }
  while (Charging_cur) {
       Charging = isCharging();
       bool   ChargeFull = isChargeFull();
       BatLevel = getBatteryLevel();

       String infoPower = "Is Charging?" + String ((Charging ? " YES" : " NO")); 
       String infoPower2 = "Is Charge full?" + String ((ChargeFull ? " YES" : " NO")); 
       String  infoPower3 = "Battery Level: " + String(BatLevel) + "%" ;  
       Serial.println(infoPower);
       Serial.println(infoPower2);
       Serial.println(infoPower3);
       Serial.println("");

       if (!Charging) {
	poweron_GSM();
	while (!connect_GSM()){};
  	modem.sendSMS("+336xxxxxxxx", String("Power Outage detected!") );
	delay(3000);
	Charging_cur = false ;
       }
   delay(5000);
  }
 delay(10000);
}
