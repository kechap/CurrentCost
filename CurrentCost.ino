#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>

// The hostname of server
#define SERVER_HOST_NAME "example.com"
// The port to connect to
#define SERVER_PORT 80


// Ethernet reset line attached to pin 7
#define ETHERNET_RESET 7

char hexChars[] = "0123456789ABCDEF";
#define HEX_MSB(v) hexChars[(v & 0xf0) >> 4]
#define HEX_LSB(v) hexChars[v & 0x0f]

int activeSensors = 1;

typedef struct {
  int counter;
  long value;
} sensor;

sensor sensors[] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
                      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}      
                      };


const unsigned long fiveMinutes = 300000; 
static unsigned long nextUpdate;


// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char uuid[] = "INSERTURIDHERE";

char server[] = SERVER_HOST_NAME;

#define STATE_INITIAL 0
#define STATE_IN_MSG 1
byte state = STATE_INITIAL;

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 177);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// Returns the next byte from the meter
// Blocks until there is something to return
char get_byte() {
  int a = -1;
  while ((a = Serial.read()) == -1) {};
  return a;
}

int resetConnection() {
  // Reset Ethernet shield in a controlled manner
  pinMode(ETHERNET_RESET,OUTPUT);
  digitalWrite(ETHERNET_RESET,LOW);
  delay(250);
  digitalWrite(ETHERNET_RESET,HIGH);
  pinMode(ETHERNET_RESET,INPUT);
  delay(500);

  return 1;
}

void setup() {
  // ethernet hack
  pinMode(ETHERNET_RESET, OUTPUT);
  digitalWrite(ETHERNET_RESET, HIGH);

  delay(500);

  // Open serial communications and wait for port to open:
  Serial.begin(57600);

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  // give the Ethernet shield a second to initialize:
  delay(1500);
  //wdt_enable(WDTO_8S); // "wdt" stands for "watchdog timer"
  nextUpdate = millis() + fiveMinutes;
}

void loop()
{
  //wdt_reset();
  state = STATE_INITIAL;
  while (state == STATE_INITIAL) {
    // Scan for </time
    while (get_byte() != '<') {}
    if (get_byte() == '/' && get_byte() == 't' && 
        get_byte() == 'i' && get_byte() == 'm' && 
        get_byte() == 'e') {
      state = STATE_IN_MSG;
    }
  }
  get_byte(); // '>'
  get_byte(); // '<'
  if (get_byte() == 'h') {
    // skip history messages
    state = STATE_INITIAL;
  } 
  else {
    for (int i=0;i<4;i++) { // 'mpr>'
      get_byte();
    }
    
    char tmp_temperature[6];
    tmp_temperature[5] = '\0';
    for (int i=0;i<4;i++) {
      tmp_temperature[i]=get_byte();
    }
    sensors[10].value += atof(tmp_temperature);
    sensors[10].counter++;
    
    int channel_counter = 0;
    int sensor_number = 0;
    while (state == STATE_IN_MSG) {
      while (get_byte() != '<') {}
      char c = get_byte();
      if (c == 's' && get_byte() == 'e' && get_byte() == 'n' && 
          get_byte() == 's' && get_byte() == 'o' && get_byte() == 'r') {
        get_byte();
        sensor_number =  get_byte() - '0';
      }
      else if (c == 'c' && get_byte() == 'h') {
        get_byte(); //ch[num]
        for (int i=0;i<8;i++) { // '><watts>'
          get_byte(); //ch[num]
        }
        char tmp_value[6];
        tmp_value[5] = '\0';
        for (int i=0;i<5;i++) {
          tmp_value[i] = get_byte();
        }
        sensors[sensor_number].value += atoi(tmp_value);
        if(channel_counter == 0) sensors[sensor_number].counter++;
        channel_counter++;
      }
      else if (c == '/' && get_byte() == 'm') {
        state = STATE_INITIAL;
        if (nextUpdate <= millis()){
           handleSending();
        }   
      }
    }
  }
  if(Ethernet.maintain() % 2 == 1) resetConnection();
}

void httpRequest(String json){
  client.print("PUT /upload/");
  client.print(uuid);
  client.println(" HTTP/1.1");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(json.length());
  client.println();
  client.println(json);
  client.println();

}

String buildJSON(){
  String json = "[";
  for(int i=0;i<11;i++){
      if(sensors[i].counter > 0){
        json += "{";
        json += "\"id\":";
        json += (int) i;
        json += ",\"value\":";
        json += (float)sensors[i].value/sensors[i].counter;
        json += "},";
        sensors[i].value = 0;
        sensors[i].counter = 0;
      }
    }

    json.setCharAt(json.length() -1, ']');
    return json;
}

void handleSending() {
  // if there's a successful connection:
  if (client.connect(server, SERVER_PORT)) {
    // send the HTTP PUT request:
    String json = buildJSON();
    httpRequest(json);
    nextUpdate = millis() + fiveMinutes;
  } 
  else {
    // if you couldn't make a connection:
    client.stop();
  }
}
