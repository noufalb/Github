/*
    ESP8266 MQTT Sample
 
	Objective of this sample project is to implement MQTT client on a ESP8266, a WiFi based development board.
	Button and led are connected to ESP8266 and a Mosquitto based MQTT broker is established for remote monitoring and control. 
	The project implements following functionality 
	
	1.	It connects to an MQTT server
	2.  Monitor a switch and publishes a message on each button press
	3.	subscribes to the topic "controlLED" and Control LED when on/off messages are received 
	4.	Send heartbeat message periodically 
	5.  It will reconnect to the server if the connection is lost using a blocking reconnect function.

	To install the ESP8266 board, (using Arduino 1.6.4+):
	- Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
	   http://arduino.esp8266.com/stable/package_esp8266com_index.json
	- Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
	- Select your ESP8266 in "Tools -> Board"
*/


#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/*change these #defines to desired gpio ,time for heartbeat and topics to publish for heartbeat and switch */
/*****************************************************************/
#define SWITCH              D2                          /*Switch                        */
#define HEARTBEATTIME       1*10000                     /*multiples of 10sec            */
#define SWITCH_TOPIC        "esp/switch"                /*mqtt topic for burtton presss */
#define HEARTBEAT_TOPIC     "esp/heartbeat"             /*mqtt topic for heartbeat      */
#define MAX_MSG_LEN         100                         /*Max topic length              */
#define CTRL_LED            D4                          /*LED to control some board its  BUILTIN_LED*/
#define LED_TOPIC           "controlLED"                /*mqtt subscribe topic          */
/*****************************************************************/
/* Update these with values suitable for network.   */

const char* ssid = "typeWifissid";                 /*user ssid            */
const char* password = "enterwifpassword";         /*user password        */
const char* mqtt_server = "test.mosquitto.org";    /*public mqtt server(for example) */

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;     /*for heartbeat   */
int touch_flag = 0;            /*Flag for switch */


/*function declaration */
void setupWifi();        
void switchRead(int sw);   
void reconnect();
/******************************************************************/

/*setupWifi() 
 * Function to intilaize wifi with given ssid and password
 * if the wifissid and password are correct serial terminal will show connected
*/
void setupWifi() {

  delay(10);
  /* We start by connecting to a WiFi network */
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


/* switchRead(switch number) 
 * Function to read switch input
 * input switch number in intiger form
 * when switch is pressed gpio status will be 0  other wise it will be 1
 * when button press is dected then the status will be published over mqtt
*/
void switchRead(int sw)
{
  int switchStatus;
  /*reads the current status of switch */
  switchStatus = digitalRead(sw);                   
  delay(300);
  if (switchStatus == 0 && touch_flag == 0)   
  {

    /*when switch pressed  publish the topic esp/switch */
    client.publish(SWITCH_TOPIC, "Button press detected", false);   
    delay(300);
    touch_flag = 1;
  }
  else if (switchStatus == 1 && touch_flag == 1)
  {
    touch_flag = 0;
    Serial.print("off");
  }
}


/*
 * reconnect() 
 * function is for reconnecting mqtt
 * 
*/

void reconnect()
{
  /* Loop until we're reconnected */
  while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      /*  Create a random client ID */
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      /*  Attempt to connect  */
      if (client.connect(clientId.c_str())) {
        Serial.println("connected");
        /*  Once connected, publish an announcement...*/
        client.publish("outTopic", "hello world");
        /* subscribe to recieve topic*/
        client.subscribe(LED_TOPIC);
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        /*  Wait 5 seconds before retrying*/
        delay(5000);
      }
  }
}


/*
 *  setup()
 * function to init wifi and configure input/output pins
 * 
*/
void setup()
{
  /* Initialize the BUILTIN_LED pin as an output */
  pinMode(CTRL_LED, OUTPUT);     
  /*configure as SWITCH pin as input */
  pinMode(SWITCH, INPUT_PULLUP ); 
  /*Serial_baudrate    */         
  Serial.begin(115200);   
  /*connects to given wifi   */               
  setupWifi();   
  /*connects to the given mqtt server and port   */                       
  client.setServer(mqtt_server, 1883);  
  /*mqtt_mqttCallback function to recive mqtt publish */
  client.setCallback(mqttCallback);            
}

/*
 * loop() 
 * Function to monitor connection, switch continously and publish heartbeat message over mqtt
*/
void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  /* monitor switch */
  switchRead(SWITCH);

  unsigned long now = millis();
  /* send heartbeat message every HEARTBEATTIME milli second */
  if (now - lastMsg > HEARTBEATTIME) 
  {
    lastMsg = now;
    client.publish(HEARTBEAT_TOPIC, "Heartbeat");     
  }
}

/*
 * mqttCallback() 
 * Function to recive message from server or messages published by other nodes
*/
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  static char message[MAX_MSG_LEN+1];
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  /*copy payload to a static string */
  
  if (length > MAX_MSG_LEN) {
  length = MAX_MSG_LEN;
  }
  strncpy(message, (char *)payload, length);
  message[length] = '\0';
   

  /* Switch on the LED if an on is recieved*/
  if (strcmp(message, "on") == 0) {
   /*Serial.print("Message on /n"); //Debug */
   /* Turn the LED on (Note that LOW is the voltage level */
    digitalWrite(CTRL_LED, LOW);
  } else if (strcmp(message, "off") == 0) {
    /* Turn the LED off by making the voltage HIGH */
    /*Serial.print("Message off \n"); //Debug */
    digitalWrite(CTRL_LED, HIGH);
  }
  
  
}
