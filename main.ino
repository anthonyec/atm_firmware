// Need this empty line to stop big compile errors hmmm
#include "Adafruit_Thermal.h"
#include "images/eyes.h"
#include "images/sad.h"
#include "images/happy.h"
#include "images/shocked.h"
#include "images/confused.h"

// http://bit.ly/2crnrj0
SYSTEM_MODE(SEMI_AUTOMATIC);

TCPClient client;
Adafruit_Thermal printer;

uint8_t retry_count = 0;
unsigned long old_time = millis();

void setup() {
  WiFi.on();

  // Setup WiFi, the device can store up to 5 credentials
  WiFi.setCredentials("ATM_2.4G", "c0pb0trulez");

  // Connect to USB serial
  Serial.begin(9600);
  Serial.println("USB connected");

  // Connect to printer serial. Pass serial to printer lib
  Serial1.begin(19200);
  printer.begin(&Serial1);
  Serial.println("Printer connected");

  // Set up Particle cloud functions
  Particle.function("printData", printData);
  Particle.function("printText", printText);

  // Debug stuff
  String ssid = WiFi.SSID();
  Serial.print("[SSID]: ");
  Serial.println(ssid);
}

void loop() {
  if (millis() - old_time >= 2000) {
    if (retry_count < 10) {
      if (!WiFi.ready()) {
        WiFi.connect();
        retry_count++;
      } else if (!Particle.connected()) {
        Particle.connect();
        retry_count++;
      }
    } else {
      WiFi.off();
      retry_count = 0;
      WiFi.on();
    }
    old_time = millis();
  }

  // Observe the bytes coming in to see if they match any formatting rules
  // Bytes 14 to 22 dont seem to be used for anything else
  while(client.available()) {
    byte currentByte = client.read();

    switch(currentByte) {
      case 14:
        printer.justify('C');
        break;
      case 15:
        printer.justify('L');
        break;
      case 16:
        printer.boldOn();
        break;
      case 17:
        printer.boldOff();
        break;
      case 18:
        printer.underlineOn();
        break;
      case 19:
        printer.underlineOff();
        break;
      case 20:
        printer.setSize('S');
        break;
      case 21:
        printer.setSize('M');
        break;
      case 22:
        printer.setSize('L');
        break;
      case 23:
        printer.printBitmap(eyes_width, eyes_height, eyes_data);
        break;
      case 24:
        printer.printBitmap(happy_width, happy_height, happy_data);
        break;
      case 25:
        printer.printBitmap(sad_width, sad_height, sad_data);
        break;
      case 26:
        printer.printBitmap(confused_width, confused_height, confused_data);
        break;
      case 27:
        printer.printBitmap(shocked_width, shocked_height, shocked_data);
        break;
      default:
        // By default send the byte to the printer. Usually this will just be
        // characters that can be printed
        Serial1.write(currentByte);
    }
  }

  if (!client.connected()) {
    client.stop();
  }

  // if (Particle.connected() == false) {
  //   System.reset();
  // }
}

// https://community.particle.io/t/spark-function-limits/952/6
void splitArgStringToArray(String arguments, String *target){
  int numArgs = 0;
  int beginIdx = 0;
  int idx = arguments.indexOf(";");

  while (idx != -1) {
    String arg = arguments.substring(beginIdx, idx);
    arg.trim();
    target[numArgs] = arg;

    beginIdx = idx + 1;
    idx = arguments.indexOf(";", beginIdx);
    ++numArgs;
  }

  // Single or last parameter
  String lastArg = arguments.substring(beginIdx);
  target[numArgs] = lastArg;
}

int printData(String data) {
  String args[3] = {NULL};

  // Need to manually split the arguments coming in because Particle cloud can
  // only send 1 string argument. The dilemter is ";"
  splitArgStringToArray(data, args);

  String url = args[0];
  String portStr = args[1];
  String idStr = args[2];
  int port = portStr.toInt();

  Serial.println("[REQUESTING]");
  Particle.publish("tcp", "requesting");

  if (client.connect(url, port)) {
    Serial.println("[SUCCESS]");
    Particle.publish("tcp", "success");
    client.write(idStr);
    client.flush();
    return 1;
  } else {
    Serial.println("[FAILED]");
    Particle.publish("tcp", "failed");
    return 0;
  }
}

int printText(String text) {
  printer.println(text);
  return 1;
}
