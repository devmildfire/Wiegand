#include <Arduino.h>

#include <Wiegand.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>

#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ArduinoJson.h>







#define PIN_D0 D5
#define PIN_D1 D6

WIEGAND wg;

#define NOTE_B0  31
// change this to whichever pin you want to use
int buzzer = D7;





const char* ssid = "MyPublicWiFi";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Set your Static IP address
IPAddress local_IP(192, 168, 5, 25);
// Set your Gateway IP address
IPAddress gateway(192, 168, 5, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional


void getFSData() {
	Serial.println(F("Initializing FS ... "));
	if (SPIFFS.begin()) {
		Serial.println(F("done."));
	}else{
		Serial.println(F("fail."));
	}

	Serial.println("File sistem info.");


	FSInfo fs_info;
	SPIFFS.info(fs_info);

	Serial.print("Total space:      ");
	Serial.print(fs_info.totalBytes);
	Serial.print("bytes");
	Serial.println();

	Serial.print("Total space used:      ");
	Serial.print(fs_info.usedBytes);
	Serial.print("bytes");
	Serial.println();


	Serial.print("Block size:      ");
	Serial.print(fs_info.blockSize);
	Serial.print("bytes");
	Serial.println();

	Serial.print("Max path length:      ");
	Serial.print(fs_info.maxPathLength);
	Serial.println();


	Dir dir = SPIFFS.openDir("/");
	while(dir.next()) {
		Serial.print(dir.fileName());
		Serial.print(" - ");
		if(dir.fileSize()) {
		File f = dir.openFile("r");
		Serial.println(f.size());
		f.close();
		} else {
		Serial.println("0");
		}
	}
}

void notifyClients(String dataString) {
	ws.textAll(dataString);
}

// Loads the user data from a file and sends it via websocket
void SendUserFromFile(const String& filename) {
	// Open file for reading
	File file =  SPIFFS.open(filename, "r+");      

	// Allocate a temporary JsonDocument
	StaticJsonDocument<512> doc;

	// Deserialize the JSON document
	DeserializationError error = deserializeJson(doc, file);
	if (error)
		Serial.println(F("Failed to read file, using default configuration"));

	// Close the file (Curiously, File's destructor doesn't close the file)
	file.close();

	// Copy values from the JsonDocument to the varianles
	String cardNumber = doc["card_number"];
	String userName = doc["user_name"];
	String coffeeCount = doc["coffee_count"];
	String coffeePool = doc["coffee_pool"];

	String FullString = (String)"card_number: " + cardNumber + " user_name: " + userName +  " coffee_count: " + coffeeCount + " coffee_pool: " + coffeePool;
	notifyClients(FullString);
}

void getUserData() {

	Serial.print("getting users data...");

	Dir dir = SPIFFS.openDir("/");
	while(dir.next()) {

		if (  (dir.fileName() != "/example.html") && (dir.fileName() != "/example.css") && (dir.fileName() != "/example.js")  ) {
			Serial.println(dir.fileName());
			String fileData = dir.fileName(); 
			// notifyClients(fileData);
			SendUserFromFile(fileData);
		}
		
	}
}



void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    // if (strcmp((char*)data, "toggle") == 0) {
    //   ledState = !ledState;
    //   notifyClients();
    // }
	notifyClients("Return Hello from websocket!");
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
			getUserData();
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}



// Loads the user data from a file and sends it via websocket
void editUserFile(const String& filename) {
	// Open file for reading
	File file =  SPIFFS.open(filename, "r");

	// Allocate a temporary JsonDocument
	StaticJsonDocument<512> doc;

	// Deserialize the JSON document
	DeserializationError error = deserializeJson(doc, file);
	if (error)
		Serial.println(F("Failed to read file, using default configuration"));
	file.close();

	// Get values from the JsonDocument and edit them
	int coffeeCount = doc["coffee_count"];
	coffeeCount += 1;
	int coffeePool = doc["coffee_pool"];
	coffeePool -= 1;

    doc["coffee_count"] = coffeeCount;
	doc["coffee_pool"] = coffeePool;

	file =  SPIFFS.open(filename, "w");
	serializeJson(doc, file);

	file.close();
}


bool createNewUser(const String& filename) {

	if (SPIFFS.exists(filename) == true) {
		Serial.println("file already exists ... " + filename);
		return false;
	}

	File file = SPIFFS.open(filename, "w");

		String str = filename;
		str.remove(0,1);

		StaticJsonDocument<512> doc;
		doc["card_number"] = str;
		doc["user_name"] = "User_" + str;
		doc["coffee_pool"] = 50;
		doc["coffee_count"] = 0;

		serializeJson(doc, file);

	file.close();
	Serial.println("created new user file ... " + filename);
	return true;
	
}


String processor(const String& var)
{
  if(var == "HELLO_FROM_TEMPLATE")
    return F("Hello world!");
  return String();
};




void setup() {

	Serial.begin(115200);  

	getFSData();





    pinMode(PIN_D0, INPUT);
    pinMode(PIN_D1, INPUT);


	
    Serial.println("input configured");
	
	// default Wiegand Pin 2 and Pin 3 see image on README.md
	// for non UNO board, use wg.begin(pinD0, pinD1) where pinD0 and pinD1 
	// are the pins connected to D0 and D1 of wiegand reader respectively.
	wg.begin(PIN_D0, PIN_D1);

	// tone(buzzer, NOTE_B0, 500);

	digitalWrite(buzzer, HIGH);
	delay(1000);
	digitalWrite(buzzer, LOW);




	if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
		Serial.println("STA Failed to configure");
	}

	// Serial.begin(115200);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	Serial.println("");

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());


	String responseString = (String)"Hi! This page is connected through local WiFi network with ip... " + WiFi.localIP().toString();

	initWebSocket();



	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(SPIFFS, "/example.html", String(), false, processor);
	});

	server.on("/example.css", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(SPIFFS, "/example.css",  "text/css");
	});

	server.on("/example.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(SPIFFS, "/example.js",  "text/javascript");
	});


	AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
	server.begin();
	Serial.println("HTTP server started");

	// getUserData();




}

void loop() {
	if(wg.available())
	{
		Serial.print("Wiegand HEX = ");
		Serial.print(wg.getCode(),HEX);
		Serial.print(", DECIMAL = ");
		Serial.print(wg.getCode());
		Serial.print(", Type W");
		Serial.println(wg.getWiegandType());    

		String wgCode = String(wg.getCode(), HEX);

		String wgData = (String)"Wiegand HEX = " + wgCode + ", DECIMAL = " + wg.getCode() + ", Type W" + wg.getWiegandType() ;
		notifyClients(wgData);
		delay(1000);

		String fileName = "/" + wgCode;

		if (SPIFFS.exists(fileName)) {
			Serial.println("found file ... " + fileName);

			notifyClients("found user file!" + fileName); 
			SendUserFromFile(fileName);
			editUserFile(fileName);
			notifyClients("edited user file!");
			SendUserFromFile(fileName); 
		} else {
			if (createNewUser(fileName)) {
				notifyClients("created user ... " + fileName );
				SendUserFromFile(fileName); 
			}
		}

	}

	// tone(buzzer, NOTE_B0, 500);
	// delay(1000);

	// digitalWrite(buzzer, HIGH);
	// delay(1000);
	// digitalWrite(buzzer, LOW);


	ws.cleanupClients();


}