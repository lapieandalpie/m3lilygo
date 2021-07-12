#include <WiFi.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include "Free_Fonts.h"
#define GFXFF 1
#include "config.h"


int mins;
int batterypower = 0;
TTGOClass *ttgo;
AXP20X_Class *power;
static lv_obj_t * ta;
unsigned long startMillis;
unsigned long currentMillis;
unsigned long helpMillis;

const char* ssid = "XXX"; // the name of your wifi
const char* password = "YYY";  // your wifi password
const char* mqtt_server = "192.168.1.25"; //whatever address your mqtt server is on


char* parsechar = "fartotron";

int battleft = 0;
bool helpon = false;
int helppressed = 0;
char temp[100];
long lastMsg = 0;
char msg[100];
int value = 0;
int fail = 0;
bool countdown = false;
char buf[256];
bool sleepmode = false;
bool dim = false;

WiFiClient espClient;
PubSubClient client(espClient);
LV_FONT_DECLARE(arial_20);
TTGOClass *watch = nullptr;
PCF8563_Class *rtc;
LV_IMG_DECLARE(gm); //convert files at https://lvgl.io/tools/imageconverter

void setup()
{
  pinMode (4, OUTPUT); //vibrator motor port
  Serial.begin(9600);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();
         ttgo->lvgl_begin();
      ttgo->bl->adjust(20); //brightness


  lv_obj_t *img1 = lv_img_create(lv_scr_act(), NULL);
  lv_img_set_src(img1, &gm);
  lv_obj_align(img1, NULL, LV_ALIGN_CENTER, 0, 0);

  lv_task_handler();

  delay (1000); // needs a sec to let the systems start up

  setup_wifi();
}


void loop()
{

  if (!sleepmode) {
    if (ttgo->power->isVBUSPlug()) {
      ttgo->tft->setTextColor(0xDF9E);
      ttgo->tft->setTextFont(2);
      ttgo->tft->setCursor(92, 6);
      ttgo->tft->print("USB");

    }  else {
      ttgo->tft->fillRect(92, 6, 79, 14, 0x0000);
    }
  }

  currentMillis = millis();
  if (currentMillis - startMillis > 15000) { //have 15 seconds passed? if so, dim screen
    if (!dim) {
      helpon = false;
      ttgo->tft->fillRect(158, 205, 60, 24, 0xA000);
      ttgo->bl->adjust(5); // dim the backlight
      dim = true;
    }
  }

  if (currentMillis - startMillis > 20000) {  //have 20 seconds elapsed?  if so, sleep.
    if (!sleepmode) {
      client.publish("M3/Stories/AH/LastMessage/Message", " ");
      ttgo->displaySleep();
      ttgo->closeBL();
      sleepmode = true;
      delay (50);

    }
  }

  if (!sleepmode && !dim ) {
    if (!helpon && currentMillis - startMillis > 1000) {
      ttgo->tft->setTextFont(4);
      ttgo->tft->setTextColor(0xffff);
      ttgo->tft->setCursor(158, 205);
      ttgo->tft->fillRect(158, 205, 60, 24, 0xA000);
      ttgo->tft->print("Help!");
      helpon = true;
    }
  }



  int16_t x, y;
  if (ttgo->getTouch(x, y)) {  //if screen is touched

    if (dim && !sleepmode) { //if screen is dimmed but not asleep then wake up
      startMillis = millis();
      sleepmode = false;
      dim = false;
      ttgo->bl->adjust(55);

    }
    if (sleepmode) {  // if asleep, then wake up with a short vibration as touch feedback
      digitalWrite (4, HIGH);
      delay (10);
      digitalWrite (4, LOW);
      startMillis = millis();
      sleepmode = false;
      dim = false;
      ttgo->openBL();
      ttgo->displayWakeup();
      delay (5);
      ttgo->bl->adjust(55);
    }

    if (!sleepmode && !dim && x > 150 && y > 200 && currentMillis - startMillis > 800) { // if not asleep and touch is in the "help" button area

      if (helppressed == 1) { //if this is the second help press, then ask for help
        ttgo->tft->fillRect(0, 80, 240, 116, 0x3A4A);
        ttgo->tft->setTextFont(4);
        ttgo->tft->setTextColor(0xDF9E);
        ttgo->tft->setCursor(0, 120);
             ttgo->tft->fillRect(158, 205, 60, 24, 0xA000);
        client.publish("w1/help", "help");
        digitalWrite (4, HIGH);
        delay (25);
        digitalWrite (4, LOW);
        ttgo->tft->println(" >Help Requested!<");
        helppressed = 2;
        delay (1000);
      }

      if (helppressed == 0) { //if this is the first help press, then ask if user is sure
        ttgo->tft->fillRect(0, 80, 240, 116, 0x3A4A);;
        ttgo->tft->setTextFont(4);
        ttgo->tft->setTextColor(0xDF9E);
        ttgo->tft->setCursor(0, 100);

             ttgo->tft->fillRect(158, 205, 60, 24, 0xA000);
        digitalWrite (4, HIGH);
        delay (25);
        digitalWrite (4, LOW);
        ttgo->tft->println(" Need help?");
        ttgo->tft->print(" Press help again...");
        helppressed = 1;
        helpMillis = millis();
        currentMillis = millis();
        startMillis = millis();
        while (currentMillis - helpMillis < 900) {
          client.loop();
          currentMillis = millis();
        }
        ttgo->tft->setTextFont(4);
        ttgo->tft->setTextColor(0xffff);
        ttgo->tft->setCursor(158, 205);
             ttgo->tft->fillRect(158, 205, 60, 24, 0xA000);
        ttgo->tft->print("Help!");
      }
    }
    else {
      if (helppressed == 1) { //if you pressed help first time, but this time tapped somewhere else, abort the help
        ttgo->tft->fillRect(0, 80, 240, 116, 0x3A4A);
        helppressed = 0;
      }
      else helppressed = 0;
    }

  }

  


  if (random (10000) == 500) {  //pick a random number to 1000.  if it is 500, then update battery %
    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(0xDF9E);
    ttgo->tft->setCursor(188, 13),
         ttgo->tft->fillRect(184, 13, 33, 15, 0x0000);
    battleft = ttgo->power->getBattPercentage();
    if (battleft == 100) {  //only space for 2 digits in battery.  if 100, show 99.
      battleft = 99;
    }
    if (battleft > 99) { //weak/broken batteries often show as 127%.  if over 99, display "err" instead
      ttgo->tft->print("ERR");
    }
    else {
      ttgo->tft->print(battleft);
      ttgo->tft->print("%");
    }
  }

  if (!client.connected()) { //if link to mqtt is broken, reconnect it
    reconnect();
  }
  client.loop();
}

void setup_wifi() {

  delay(10);
  //  start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (ttgo->power->isVBUSPlug()) { //if usb cable plugged in, display "usb" on screen.
      ttgo->tft->setTextColor(0xDF9E);
      ttgo->tft->setTextFont(2);
      ttgo->tft->setCursor(92, 6),
           ttgo->tft->print("USB");
    }  else {
      ttgo->tft->fillRect(92, 6, 79, 14, 0x0000); //if no usb, black out the space where "usb" was
    }


    ttgo->tft->setTextFont(2); // battery setup, but within wifi setup
    ttgo->tft->setTextColor(0xDF9E);
    ttgo->tft->setCursor(188, 13),
         ttgo->tft->fillRect(184, 13, 33, 15, 0x0000);
    battleft = ttgo->power->getBattPercentage();
    if (battleft == 100) {
      battleft = 99;
    }
    if (battleft > 99) {
      ttgo->tft->print("ERR");
    }
    else {
      if (battleft != batterypower)  {
        ttgo->tft->print(battleft);
        ttgo->tft->print("%");
      }
    }


    ttgo->tft->fillRect(30, 210, 56, 14, 0x0000);
    ttgo->tft->setTextColor(0xDF9E);
    ttgo->tft->setTextFont(2);
    ttgo->tft->setCursor(30, 210),
         ttgo->tft->print("OFFLINE"); //not sure why this is here...
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ttgo->tft->fillRect(30, 210, 56, 14, 0x0000);


}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String msg;
  for (int i = 0; i < length; i++) { // convert message to char sequence
    Serial.print((char)payload[i]);
    msg += (char)payload[i];
  }

  if (strcmp(topic, "M3/Stories/AH/LastMessage/Message") == 0) {    //Replace "AH" with your m3 story ID
    if (msg == "" || msg == " ") {  //if message is blank or space, then wipe old message
      ttgo->tft->fillRect(0, 80, 240, 105, 0x3A4A);
      ttgo->tft->fillRect(12, 183, 208, 11, 0x3A4A);
      ttgo->displaySleep();
      ttgo->closeBL();
      sleepmode = true;
      helppressed = 0;

    }
    else { //as long as message isn't blank or space then...
      startMillis = millis(); //reset the timer
      sleepmode = false;  //set not alseep
      dim = false;  //set not dimmed
      ttgo->openBL(); 
      ttgo->displayWakeup(); // wake up screen
      digitalWrite (4, HIGH);//vibrate watch
      ttgo->bl->adjust(55); //turn up brightness

      ttgo->tft->fillRect(0, 80, 240, 105, 0x3A4A); //blank out old message
      ttgo->tft->fillRect(12, 183, 208, 11, 0x3A4A);
      ttgo->tft->setTextFont(4);
      ttgo->tft->setTextColor(0xDF9E);
      if (length > 50) { //if message is more than 50 chars, start it higher up
        ttgo->tft->setCursor(0, 85);
      }
      else ttgo->tft->setCursor(0, 100);
      ttgo->tft->print(msg);  //put the message on the screen
      ttgo->bl->adjust(55);

      msg.toCharArray(temp, msg.length() + 1); //packaging up the data to publish to mqtt
      client.publish("w1/displaying", temp);
      delay (1200);
      digitalWrite (4, LOW);
      helppressed = 0;

    }
  }



  if (strcmp(topic, "w1/bl") == 0) {  //you can send a value to w1/bl and it'll set the screen to that number's brightness
    ttgo->bl->adjust(msg.toInt());
  }

  if (strcmp(topic, "w1/clear") == 0) {
    countdown = false;
    sleepmode = true;
    ttgo->displaySleep();
    ttgo->closeBL();

  }


  if (strcmp(topic, "setup") == 0) {

    if (msg == "reset") {
      lv_obj_t *img1 = lv_img_create(lv_scr_act(), NULL);
      lv_img_set_src(img1, &gm);
      lv_obj_align(img1, NULL, LV_ALIGN_CENTER, 0, 0);
      lv_task_handler();

    }

    if (msg == "clear") {
      lv_obj_t *img1 = lv_img_create(lv_scr_act(), NULL);
      lv_img_set_src(img1, &gm);
      lv_obj_align(img1, NULL, LV_ALIGN_CENTER, 0, 0);
      lv_task_handler();

    }



  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    uint64_t chipid = ESP.getEfuseMac(); // MAC address of ESP32
    uint16_t chip = (uint16_t)(chipid >> 32);
    char clientid[25];
    snprintf(clientid, 25, "WIFI-Display-%04X", chip);
    if (client.connect(clientid)) {
      Serial.println("connected");
      // Once connected, publish an announcement...

      client.subscribe("M3/Stories/AH/LastMessage/Message");
      delay (10);
      client.subscribe("w1/time");  // this is left over from when i was sending display time to watch
      delay (10);
      client.subscribe("w1/mins"); //you can subscribe to the story's seconds remaining and divide it to get minutes
      delay (10);
      client.subscribe("w1/setup");
      delay (10);
      client.subscribe("w1/clear");
      delay (10);
      client.subscribe("w1/bl");
      delay (10);
      client.publish("w1", "watch1online");
      ttgo->tft->fillRect(30, 210, 56, 14, 0x0000);

    } else {
      ttgo->tft->fillRect(30, 210, 56, 14, 0x0000);
      ttgo->tft->setTextColor(0xDF9E);
      ttgo->tft->setTextFont(2);
      ttgo->tft->setCursor(30, 210),
           ttgo->tft->print("OFFLINEmq");

      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

char *my_strcpy(char *destination, char *source)
{
  char *start = destination;

  while (*source != '\0')
  {
    *destination = *source;
    destination++;
    source++;
  }

  *destination = '\0'; // add '\0' at the end
  return start;
}
