#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiUdp.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

const int LED_PIN = D4;
const int TOUCH_PIN = D1;

const int LED_COUNT = 100 ;
const int BRIGHTNESS = 255;

enum  pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };

int xmass_tree[LED_COUNT][2];
int white_stripe[LED_COUNT][2];

int blue_array[][3] = {{240, 248, 255}, {135, 206, 250}, {0, 191, 255}, {30, 144, 255}, {138, 43, 226}, {65, 105, 225}, {0, 0, 128}, {0, 0, 139}, {0, 0, 205}, {0, 0, 255}};

int snake_array[][3] = {{ 0xff, 0xe6, 0xff },
  { 0xff, 0xcc, 0xff },
  { 0xff, 0xb3, 0xff },
  { 0xff, 0x99, 0xff },
  { 0xff, 0x80, 0xff },
  { 0xff, 0x66, 0xff },
  { 0xff, 0x4d, 0xff },
  { 0xff, 0x33, 0xff },
  { 0xff, 0x1a, 0xff },
  { 0xff, 0x00, 0xff }
};

int rampouch_array[][3] = {{ 0x80, 0x80, 0xff },
  { 0x66, 0x66, 0xff },
  { 0x4d, 0x4d, 0xff },
  { 0x33, 0x33, 0xff },
  { 0x1a, 0x1a, 0xff },
  { 0x00, 0x00, 0xff },
  { 0x00, 0x00, 0xe6 },
  { 0x00, 0x00, 0xcc },
  { 0x00, 0x00, 0xb3 },
  { 0x00, 0x00, 0x99 }
};



// Nazev Wi-Fi site, do ktere se mam pripojit
const char* ssid = "JmenoAccessPointu_alias_wifi";
// Heslo Wi-Fi site, do ktere se mam pripojit
const char* password = "pristupove_heslo";




static int Index = 0;
static int rainbowActive = 0;
unsigned long lastUpdate;
unsigned long lastSwitchUpdate;

static int SnakeOffset = 0;
static int SnakeActive = 0;
static int Snake2Active = 0;
static int Snake3Active = 0;
static int RampouchActive = 0;
static int StackActive = 0;
static int XmassActive = 0;
static int SnakeDirection = 1;
static int SnakeLength = 10;
static int StackBottom = 0;
static int SnakeRotation = random(3, 30);
static int SnakeRed, SnakeGreen, SnakeBlue;
static int LightOnOff = 0;
static int LightBrightness = 4;
static int WhiteStripe = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "tik.cesnet.cz", 3600, 60000 );


Adafruit_NeoPixel stripe = Adafruit_NeoPixel( LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800 );


// Webserver on port 80
ESP8266WebServer server(80);


void setup() {
  // Serial
  Serial.begin(9600);

  randomSeed(analogRead(0));

  pinMode(TOUCH_PIN, INPUT);

  stripe.setBrightness(BRIGHTNESS);
  stripe.begin();
  stripe.show();

  TurnOff();

  stripe.setPixelColor(0, stripe.Color(255, 0, 0, 255));
  stripe.show();

  // Pripojeni k Wi-Fi
  Serial.println();
  Serial.print("Pripojuji k ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Dokud nejsem pripojeny k Wi-Fi,zapisuj do seriove linky tecky progressbaru
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  // Jsem pripojeny k Wi-Fi a mohu pokracovat
  Serial.println();
  Serial.println("WiFi pripojena!");
  Serial.println(WiFi.macAddress());


  stripe.setPixelColor(0, stripe.Color(0, 255, 0, 255));
  stripe.show();

  timeClient.begin();

  // Spusteni serveru
  server.begin();
  Serial.println("Server spusten");

  // Napis IP adresu, kterou mikropocitac dostal
  Serial.print("Pouzij k pripojeni tuto adresu: ");
  Serial.print("http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/turnon", []() {
    server.send(200, "text/plain", "window.location='/';");
    TurnOn();
  });

  server.on("/rainbow", []() {
    server.send(200, "text/plain", "rainbow");
    RainBowOn();
  });

  server.on("/turnoff", []() {
    server.send(200, "text/plain", "turnoff");
    TurnOff();
  });

  server.on("/turnonpink", []() {
    server.send(200, "text/plain", "turnonpink");
    TurnOnPink();
  });

  server.on("/turnonred", []() {
    server.send(200, "text/plain", "turnonred");
    TurnOnRed();
  });

  server.on("/turnongreen", []() {
    server.send(200, "text/plain", "turnongreen");
    TurnOnGreen();
  });

  server.on("/turnonblue", []() {
    server.send(200, "text/plain", "turnonblue");
    TurnOnBlue();
  });

  server.on("/random", []() {
    server.send(200, "text/plain", "random");
    TurnOnRandom();
  });

  server.on("/snake", []() {
    server.send(200, "text/plain", "snake");
    TurnOnSnake();
  });

  server.on("/snake2", []() {
    server.send(200, "text/plain", "snake");
    TurnOnSnake2();
  });

  server.on("/snake3", []() {
    server.send(200, "text/plain", "snake");
    TurnOnSnake3();
  });

  server.on("/rampouch", []() {
    server.send(200, "text/plain", "rampouch");
    TurnOnRampouch();
  });

  server.on("/stack", []() {
    server.send(200, "text/plain", "stack");
    TurnOnStack();
  });

  server.on("/xmass", []() {
    server.send(200, "text/plain", "xmass");
    TurnOnXmass();
  });

  server.on("/white", []() {
    server.send(200, "text/plain", "white");
    TurnOnWhiteLight();
  });



  server.begin();

  Serial.println("HTTP Server started");


}

void handleRoot() {
  //String message = "<html><head><style>.dropbtn {background-color: #4CAF50;color: white;padding: 8px;font-size: 16px;cursor: pointer;}</style></head><body style='font-family: sans-serif; font-size: 12px'>Following functions are available:<br><br>";
  String message = "<html><head><style>.dropbtn {height:32px;width:128px;background-color: #808080;border: 3px solid #A9A9A9;border-radius: 8px;color: #D3D3D3;padding: 5px 5px;text-align: center;text-decoration: none;display: inline-block;font-size: 16px;margin: 2px 2px;    cursor: pointer;}</style></head><body bgcolor=#000000 fgcolor=#ffffff style='font-family: sans-serif; font-size: 12px'>Following functions are available:<br><br>";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/turnon', true); xhr.send();\" value=\"Light ON\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/turnoff', true); xhr.send();\" value=\"Light OFF\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/rainbow', true); xhr.send();\" value=\"Rainbow\" /><p>";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/turnonred', true); xhr.send();\" value=\"Red\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/turnongreen', true); xhr.send();\" value=\"Green\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/turnonblue', true); xhr.send();\" value=\"Blue\" /><p>";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/turnonpink', true); xhr.send();\" value=\"Pink\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/random', true); xhr.send();\" value=\"Random\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/snake', true); xhr.send();\" value=\"Snake\" /><p>";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/snake2', true); xhr.send();\" value=\"Snake 2\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/snake3', true); xhr.send();\" value=\"Snake 3\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/rampouch', true); xhr.send();\" value=\"Rampouch\" /><p>";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/stack', true); xhr.send();\" value=\"Stack\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/xmass', true); xhr.send();\" value=\"Xmass\" />";
  message += "<input type=\"button\" class=\"dropbtn\" onclick=\"var xhr = new XMLHttpRequest();xhr.open('GET', '/white', true); xhr.send();\" value=\"White\" /><p>";
  //message += "<lablel for=\"fader\">Brightness</label><input type=\"range\" min=\"0\" max=\"100\" value=\"50\" id=\"fader\" step=\"1\" oninput=\"outputUpdate(value)\"><output for=\"fader\" id=\"volume\">50</output><script>function outputUpdate(vol) {document.querySelector('#volume').value = vol;}</script>";
  /*
    message += "<a href='/turnoff'>/turnoff</a>Turn OFF<br>";
    message += "<a href='/turnonpink'>/turnonpink</a>Pink<br>";
    message += "<a href='/turnongreen'>/turnongreen</a>Green<br>";
    message += "<a href='/turnonred'>/turnonred</a>Red<br>";
    message += "<a href='/turnonblue'>/turnonblue</a>Blue<br>";
    message += "<a href='/rainbow'>/rainbow</a>Blue<br>";
    message += "<a href='/random'>/radom</a>Blue<br>";

  */
  message += "</body></html>";
  server.send(200, "text/html", message);
}


void RainBowOn() {
  TurnOffEffects();
  rainbowActive = 1;
}

void TurnOnSnake() {
  TurnOffEffects();
  SnakeActive = 1;
  SnakeOffset = 0;
  SnakeDirection = 1;
  SnakeLength = 10;
  SnakeRotation = random(5, 20);
  SnakeRed = random(255);
  SnakeGreen = random(255);
  SnakeBlue = random(255);

  yield();
}

void TurnOnSnake2() {
  TurnOffEffects();
  Snake2Active = 1;
  SnakeOffset = 0;
  SnakeDirection = 1;
  SnakeLength = 10;
  SnakeRotation = random(5, 20);
  SnakeRed = random(255);
  SnakeGreen = random(255);
  SnakeBlue = random(255);

  yield();
}

void TurnOnSnake3() {
  TurnOffEffects();
  Snake3Active = 1;
  SnakeOffset = 0;
  SnakeDirection = 1;
  SnakeLength = 10;
  SnakeRotation = random(5, 20);
  SnakeRed = random(255);
  SnakeGreen = random(255);
  SnakeBlue = random(255);

  yield();
}

void TurnOnRampouch() {
  TurnOffEffects();
  RampouchActive = 1;
  SnakeOffset = 0;
  SnakeDirection = 1;
  SnakeLength = 10;
  SnakeRotation = random(5, 20);

  yield();
}

void TurnOnStack() {
  TurnOffEffects();
  StackActive = 1;
  SnakeOffset = 0;
  SnakeDirection = 1;
  SnakeLength = 10;
  SnakeRotation = random(5, 20);
  StackBottom = stripe.numPixels();

  yield();
}

void TurnOnXmass() {
  uint16_t tree_color, tree_brightness;
  TurnOffEffects();
  XmassActive = 1;

  for (uint16_t i = 0; i < stripe.numPixels(); i++) {
    tree_brightness = random(128, 256);
    tree_color = random(0, 3);
    if ( tree_color == 0 ) {
      stripe.setPixelColor(i, stripe.Color(tree_brightness, 0, 0, 255));
    } else if ( tree_color == 1 ) {
      stripe.setPixelColor(i, stripe.Color(0, tree_brightness, 0, 255));
    } else if ( tree_color == 2 ) {
      stripe.setPixelColor(i, stripe.Color(0, 0, tree_brightness, 255));
    }
    xmass_tree[i][0] = tree_color;
    xmass_tree[i][1] = tree_brightness;
  }

  stripe.show();

  yield();
}




void TurnOnRed() {
  TurnOffEffects();
  for (uint16_t i = 0; i < stripe.numPixels(); i++) {
    stripe.setPixelColor(i, stripe.Color(255, 0, 0, 255));
  }
  stripe.show();

  yield();
}


void TurnOnGreen() {
  TurnOffEffects();
  for (uint16_t i = 0; i < stripe.numPixels(); i++) {
    stripe.setPixelColor(i, stripe.Color(0, 255, 0, 255));
  }
  stripe.show();

}

void TurnOnBlue() {
  TurnOffEffects();
  for (uint16_t i = 0; i < stripe.numPixels(); i++) {
    stripe.setPixelColor(i, stripe.Color(0, 0, 255, 255));
  }
  stripe.show();

  yield();
}



void TurnOnPink() {
  TurnOffEffects();
  for (uint16_t i = 0; i < stripe.numPixels(); i++) {
    stripe.setPixelColor(i, stripe.Color(255, 0, 255, 255));
  }
  stripe.show();

  yield();
}


void TurnOff() {
  TurnOffEffects();
  for (uint16_t i = 0; i < stripe.numPixels(); i++) {
    stripe.setPixelColor(i, stripe.Color(0, 0, 0, 0));
  }
  stripe.show();

  yield();
}

void TurnOn() {
  TurnOff();
  TurnOffEffects();
  for (uint16_t i = 0; i < stripe.numPixels(); i++) {
    stripe.setPixelColor(i, stripe.Color(255, 255, 255, 255));
  }
  stripe.show();
}

void TurnOnRandom() {
  TurnOffEffects();
  Index = random(0, 255);
  for (int i = 0; i < stripe.numPixels(); i++) {
    stripe.setPixelColor(i, Wheel(((i * 256 / stripe.numPixels()) + Index) & 255));
  }
  stripe.show();

  yield();
}

void TurnOffEffects() {
  rainbowActive = 0;
  SnakeActive = 0;
  Snake2Active = 0;
  Snake3Active = 0;
  RampouchActive = 0;
  StackActive = 0;
  XmassActive = 0;
  WhiteStripe = 0;
}

void TurnOnWhiteLight() {
  TurnOffEffects();
  WhiteStripe = 1;
  for (uint16_t i = 0; i < stripe.numPixels(); i++) {
    white_stripe[i][0] = random(0, 256);
    white_stripe[i][1] = random(0, 2);
  }

  yield();
}

uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return stripe.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return stripe.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return stripe.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}



// Smycka loop se opakuje stale dokola
void loop() {

  timeClient.update();

  // Serial.println(timeClient.getFormattedTime());

  server.handleClient();

  /*
  int touchValue = digitalRead(TOUCH_PIN);
  if ( touchValue == HIGH ) {
    if (( millis() - lastSwitchUpdate ) > 300 ) {
      lastSwitchUpdate = millis();
      
      if ( LightBrightness > 0 ) {
        TurnOffEffects();
        uint16_t x = (LightBrightness * 64) - 1;
        for (uint16_t i = 0; i < stripe.numPixels(); i++) {
          stripe.setPixelColor(i, stripe.Color(x, x, x, 255));
        }
        stripe.show();
        LightBrightness--;
      } else {
        LightBrightness = 4;
        TurnOff();
      }
    }
  } // endif switch handle
  */
 
  if ( rainbowActive ) {
    if (( millis() - lastUpdate ) > 10 ) {
      lastUpdate = millis();
      for (int i = 0; i < stripe.numPixels(); i++) {
        stripe.setPixelColor(i, Wheel(((i * 256 / stripe.numPixels()) + Index) & 255));
      }
      stripe.show();
      //  Increment
      Index++;
      if ( Index >= 255 ) {
        Index = 0;
      }
    }
  } // endif rainbow

  if ( RampouchActive )
  {
    if (( millis() - lastUpdate ) > 10 ) {
      lastUpdate = millis();
      for (int i = 0; i < stripe.numPixels(); i++) {
        if ( i >= SnakeOffset && i < SnakeOffset + SnakeLength ) {
          stripe.setPixelColor(i, stripe.Color(rampouch_array[(i - SnakeOffset)][0], rampouch_array[(i - SnakeOffset)][1], rampouch_array[(i - SnakeOffset)][2] , 255));
        } else {
          stripe.setPixelColor(i, stripe.Color(0, 0 , 0, 0));
        }
      }
      stripe.show();

      if ( SnakeDirection == 1 ) {
        if ( SnakeOffset < stripe.numPixels() ) {
          SnakeOffset++;
        } else {
          SnakeDirection = 1;
          SnakeOffset = 0;
        }
      }

    }
  }

  if ( StackActive )
  {
    if (( millis() - lastUpdate ) > 10 ) {
      lastUpdate = millis();
      for (int i = 0; i < StackBottom; i++) {
        if ( i >= SnakeOffset && i < SnakeOffset + SnakeLength ) {
          stripe.setPixelColor(i, stripe.Color(rampouch_array[(i - SnakeOffset)][0], rampouch_array[(i - SnakeOffset)][1], rampouch_array[(i - SnakeOffset)][2], 255));
        } else {
          stripe.setPixelColor(i, stripe.Color(0, 0 , 0, 0));
        }
      }

      stripe.show();
      if ( SnakeDirection == 1 ) {
        if ( SnakeOffset < StackBottom ) {
          SnakeOffset++;
        } else {
          stripe.setPixelColor(StackBottom, stripe.Color(0, 0, 255, 255));
          stripe.show();
          if ( StackBottom > 1 ) {
            StackBottom--;
          } else {
            StackBottom = stripe.numPixels();

          }
          SnakeOffset = 0;
        }
      }

    }
  }

  if ( XmassActive ) {
    if (( millis() - lastUpdate ) > 10 ) {
      lastUpdate = millis();
      for (int i = 0; i < stripe.numPixels(); i++) {
        if ( xmass_tree[i][1] < 2 ) {
          xmass_tree[i][1] = random(128, 256);
          xmass_tree[i][0] = random(0, 3);
        }

        xmass_tree[i][1]--;
        if ( xmass_tree[i][0] == 0 ) {
          stripe.setPixelColor(i, stripe.Color(xmass_tree[i][1], 0, 0, 255));
        } else if ( xmass_tree[i][0] == 1 ) {
          stripe.setPixelColor(i, stripe.Color(0, xmass_tree[i][1], 0, 255));
        } else if ( xmass_tree[i][0] == 2 ) {
          stripe.setPixelColor(i, stripe.Color(0, 0, xmass_tree[i][1], 255));
        }

      } // end for
      stripe.show();
    }

  }


  if ( Snake2Active )
  {
    if (( millis() - lastUpdate ) > 30 ) {
      lastUpdate = millis();
      for (int i = 0; i < stripe.numPixels(); i++) {
        if ( i >= SnakeOffset && i < SnakeOffset + SnakeLength ) {
          stripe.setPixelColor(i, stripe.Color(255, snake_array[(i - SnakeOffset)][1], 255, 255));
        } else {
          stripe.setPixelColor(i, stripe.Color(0, 0 , 0, 0));
        }
      }

      stripe.show();
      if ( SnakeDirection == 1 ) {
        if ( SnakeOffset < stripe.numPixels() ) {
          SnakeOffset++;
        } else {
          SnakeDirection = 0;
        }
      } else {
        if ( SnakeOffset > 0 ) {
          SnakeOffset--;
        } else {
          SnakeDirection = 1;
        }
      }

    }
  }

  if ( SnakeActive ) {
    if (( millis() - lastUpdate ) > 30 ) {
      lastUpdate = millis();
      for (int i = 0; i < stripe.numPixels(); i++) {
        if ((( i + SnakeOffset) / SnakeLength) % 2) {
          //if ( (i%SnakeLength>=SnakeOffset) && (i%SnakeLength<=SnakeOffset+SnakeLength) ) {
          stripe.setPixelColor(i, stripe.Color(255, 0, 0, 255));
        } else {
          stripe.setPixelColor(i, stripe.Color(0, 255, 0, 255));
        }
      } // endfor

      stripe.show();
      if ( SnakeDirection == 1 ) {
        if ( SnakeOffset < SnakeRotation) {
          SnakeOffset++;
        } else {
          SnakeDirection = 0;
          SnakeRotation = random(3, 50);
        }
      } else {
        if ( SnakeOffset > 0 ) {
          SnakeOffset--;
        } else {
          SnakeDirection = 1;
          SnakeRotation = random(3, 50);
        }
      }

    }
  } // endif rainbow

  if ( Snake3Active ) {
    if (( millis() - lastUpdate ) > 50 ) {
      lastUpdate = millis();
      for (int i = 0; i < stripe.numPixels(); i++) {
        int iMode = ((i +  SnakeOffset) / SnakeLength) % 3;
        if (iMode == 0) {
          stripe.setPixelColor(i, stripe.Color(255, 0, 0, 255));
        } else if (iMode == 1) {
          stripe.setPixelColor(i, stripe.Color(0, 255, 0, 255));
        } else {
          stripe.setPixelColor(i, stripe.Color(0, 0, 255, 255));
        }
      } // endfor

      stripe.show();
      if ( SnakeDirection == 1 ) {
        if ( SnakeOffset < SnakeRotation) {
          SnakeOffset++;
        } else {
          SnakeDirection = 0;
          SnakeRotation = random(3, 50);
        }
      } else {
        if ( SnakeOffset > 0 ) {
          SnakeOffset--;
        } else {
          SnakeDirection = 1;
          SnakeRotation = random(3, 50);
        }
      }

    }
  } // endif rainbow

  if ( WhiteStripe ) {
    if (( millis() - lastUpdate ) > 10 ) {
      lastUpdate = millis();
      for (int i = 0; i < stripe.numPixels(); i++) {
        stripe.setPixelColor(i, stripe.Color(white_stripe[i][0], white_stripe[i][0], white_stripe[i][0], 255));
        if ( white_stripe[i][1] == 1 ) {
          if ( white_stripe[i][0] < 256 ) {
            white_stripe[i][0]++;
          } else {
            white_stripe[i][1] = 0;
          }
        } else {
          if ( white_stripe[i][0] > 0 ) {
            white_stripe[i][0]--;
          } else {
            white_stripe[i][1] = 1;
          }
        } // endif
      }

      stripe.show();
    }
  } // endif white stripe

  yield();
}
