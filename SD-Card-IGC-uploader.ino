/*
  SD-Card-IGC-uploader

  Uploads .igc files from SD Card to www.lz-spl.ga

  Author: Mr. Psycho, psychotm@gmail.com
*/

//#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <FS.h>
#include <SD.h>
#include <SPI.h>

#define SSID_NAME     "WIFI-SSID"
#define SSID_PASSWORD "WIFI-PASS"
#define IGC_DIRECTORY "/igc/"
#define IGC_ARCHIVE   "/archive/"
WiFiServer ESPserver(80);//Service Port

#define GPIO_PIN_LED  2 // GPIO2 of ESP8266

/*
  #define GPIO_PIN_MISO 12
  #define GPIO_PIN_MOSI 13
  #define GPIO_PIN_SCK 14
  #define GPIO_PIN_SS 15
*/
#define GPIO_PIN_CS 15

/*
  #define GPIO_PIN_SCL 5
  #define GPIO_PIN_SDA 4
*/
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void WiFi_setup() {
  pinMode(GPIO_PIN_LED, OUTPUT);
  digitalWrite(GPIO_PIN_LED, HIGH);

  Serial.printf("\n\nConnecting to %s", SSID_NAME);

  WiFi.begin(SSID_NAME, SSID_PASSWORD);
  /*
    IPAddress ip(192, 168, 1, 254);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.config(ip, gateway, subnet);
  */
  delay(5000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(". ");
  }

  Serial.println("\nWiFi connected");

  // Start the server
  ESPserver.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Control URL http://");
  Serial.println(WiFi.localIP());
}

void WiFi_loop() {
  // Check if a client has connected
  WiFiClient client = ESPserver.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Match the request

  int value = digitalRead(GPIO_PIN_LED);
  if (request.indexOf("/LED=ON") != -1) {
    Serial.println("LED is ON");
    digitalWrite(GPIO_PIN_LED, LOW);
    value = HIGH;
  }
  if (request.indexOf("/LED=OFF") != -1) {
    Serial.println("LED is OFF");
    digitalWrite(GPIO_PIN_LED, HIGH);
    value = LOW;
  }

  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  IMPORTANT
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.print("Status of the LED: ");

  if (value == HIGH) {
    client.print("ON");
  } else {
    client.print("OFF");
  }
  client.println("<br><br>");
  client.println("Click <a href=\"/LED=ON\">here</a> to turn ON the LED<br>");
  client.println("Click <a href=\"/LED=OFF\">here</a> to turn OFF the LED<br>");
  client.println("</html>");

  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");
}

bool Upload_to_API(String uFile) {

  WiFiClient client;
  const char* host =  "log.my-domain.com";
  String URI =        "/importigc";
  //String URI =        "/files/post.php";
  String fe_user =    "WEB user";
  String fe_pass =    "WEB PASS";
  String import_uid = "WEB uID";

  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return false;
  }
  auto upload_file = SD.open(IGC_DIRECTORY + uFile, FILE_READ);
  const auto upload_size = upload_file.size();
//  Serial.print("Name: ");
//  Serial.println(upload_file.name());
//  Serial.print("Size: ");
//  Serial.println(upload_size);
  if ( upload_size < 1) return false;
  String file_line;
  while(upload_file.available()) {
    file_line += upload_file.readStringUntil('\r');
  }
  upload_file.close();

  String request_body;// = "\r\n";
  request_body = "\r\n------WebKitFormBoundaryjg2qVIUS8teOAbN3\r\n";
  request_body += "Content-Disposition: form-data; name=\"import_uid\"\r\n";
  request_body += "\r\n";
  request_body += import_uid;
  request_body += "\r\n------WebKitFormBoundaryjg2qVIUS8teOAbN3\r\n";
  request_body += "Content-Disposition: form-data; name=\"fe_user\"\r\n";
  request_body += "\r\n";
  request_body += fe_user;
  request_body += "\r\n------WebKitFormBoundaryjg2qVIUS8teOAbN3\r\n";
  request_body += "Content-Disposition: form-data; name=\"fe_pass\"\r\n";
  request_body += "\r\n";
  request_body += fe_pass;
  request_body += "\r\n------WebKitFormBoundaryjg2qVIUS8teOAbN3\r\n";
  request_body += "Content-Disposition: form-data; name=\"importigc_igc\"; filename=\"";
  uFile.replace(IGC_DIRECTORY, "");
  request_body += uFile;
  request_body += "\"\r\n"; // file name
  request_body += "Content-Type: application/octet-stream\r\n";
  request_body += "\r\n";
  request_body += file_line;
  request_body += "\r\n------WebKitFormBoundaryjg2qVIUS8teOAbN3--";
  request_body += "\r\n";

  client.print("POST ");
  client.print(URI);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(host);
  client.println("User-Agent: SD Card Reader toolkit");
  client.println("Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryjg2qVIUS8teOAbN3");
  //client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");           //file size  //change it
  client.println(request_body.length());
  client.println(request_body);

  if (client.connected()) {

//    while (client.connected()) {
//      String line = client.readStringUntil('\r');
//      Serial.print(line);
//    }
    client.stop();  // DISCONNECT FROM THE SERVER
    SD.mkdir(IGC_ARCHIVE);
    //rename(uFile, IGC_ARCHIVE + "/" + uFile);
    return true;
  } else {
    return false;
  }
}

void listDir(String root_path, bool debug) {
  //Serial.printf("Listing directory: %s\n", dirname);
  String line_1;
  String line_2;
  String line_3;
  String line_4;
  int count = 0;

  File root = SD.open(root_path);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    // check for folders starting with 202 (i.e 2020-11-22)
    if (file.isDirectory()) continue;
    if ( String(file.name()).indexOf("202") == 0 && String(file.name()).endsWith(".igc")) {
      if (debug) Serial.println("UPLOAD: " + root_path + file.name());

      // last 4 files for the 0.96" OLED display
      line_4 = line_3;
      line_3 = line_2;
      line_2 = line_1;
      line_1 = file.name();
      line_1 += Upload_to_API(file.name())? "*" : "-";

      // every file with full name
      line_1.replace("PSY-CHO-", "");
      line_1.replace(".igc", "");

      // every file (short name) in directory
      display.clearDisplay();

      display.setTextSize(1);
      display.setTextColor(WHITE);

      display.setCursor(0, 0);
      //display.println(line_4);
      display.println(root_path + "  " + ++count);
      display.display();

      display.setCursor(0, 16);
      display.println(line_3);
      display.display();

      display.setCursor(0, 32);
      display.println(line_2);
      display.display();

      display.setCursor(0, 48);
      display.println(line_1);
      display.display();

      //delay(1000);
    }
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);

  /* WiF */
  WiFi_setup();

  /* display */
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
  } else {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.print("WiFi: ");
    display.println(SSID_NAME);

    display.setCursor(0, 32);
    display.print("IP: ");
    display.println(WiFi.localIP());

    display.display();
    delay(2500);
    display.clearDisplay();
    display.display();
  }

  /* SD Card */
  if (!SD.begin(GPIO_PIN_CS)) {
    Serial.println("SD Card init failed!");
    display.clearDisplay();

    display.setTextSize(2);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.println("SD Card ..");

    display.setCursor(0, 32);
    display.println("  FAILED");

    display.display();
    return;
  }

  listDir(IGC_DIRECTORY, false);
}

void loop() {
  WiFi_loop();
}
