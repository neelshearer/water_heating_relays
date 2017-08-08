
/*********************************************************************
 * 
 * Water_Heating.ino
 * 
 * This is the code for running a Solar hot water system, with backup Rocket 
 * stove water heating for the greyer days! It also runs a light sensor to turn 
 * on campsite stair lighting when it gets dark. 
 * Using a loop counter it activates a relay to control a grey water recycling system. 
 * 
 * It makes use of several libraries:
 * 
 * OneWire
 * Dallas Temperature
 * SPI
 * Adafruit Graphics library
 * TSL2561 Light Sensor
 * 
 * (You will need to install all of these if you want the code below to compile!)
 * 
 * The system uses 4 onewire temperature sensors, a TSL2561 Light sensor from 
 * Sparkfun and an Adafruit 1306 Monochrome OLED display, as well as 6 relays.
 * 
 * It should be fairly easy to tweak this to your needs - code is mostly commented, 
 * makes use of only trivial constructs (if, for, do etc) and basic glyph building 
 * capabilities of the graphics library.
 * 
 * No liability or responsibility can be taken for those who use this code - it is
 * supplied without warranty or guarantee of suitability. Proper care and 
 * attention must be taken when designing your system - there are multiple 
 * possible dangers, for example:
 * 
 * Water
 * Electricity
 * Heat
 * Pressure
 * 
 * Do your homework, design with safety in mind, and build carefully.
 * 
 * Above all, have fun and good luck!
 * 
 *********************************************************************/

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SFE_TSL2561.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Create an SFE_TSL2561 object, here called "light":

SFE_TSL2561 light;

// Global variables:

boolean gain;     // Gain setting, 0 = X1, 1 = X16;
unsigned int ms;  // Integration ("shutter") time in milliseconds

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Using software SPI:
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Declare the glyph for the Tank loop 

#define LOGO16_TANK_HEIGHT 16 
#define LOGO16_TANK_WIDTH  16 
static const unsigned char PROGMEM logo16_tank_bmp[] =
{ 
  B00000000, B00000000,
  B00000001, B10000000,
  B00001110, B01110000,
  B00110000, B00001100,
  B01000000, B00000010,
  B01000000, B00000010,
  B01000000, B00000010,
  B01000011, B11111110,
  B01001100, B00000010,
  B01000011, B11000010,
  B01000000, B00110010,
  B01000011, B11000010,
  B01001100, B00000010,
  B01000011, B11111110,
  B01000000, B00000010,
  B01111111, B11111110 };

// Declare the glyph for the Solar loop

#define LOGO16_SOL_HEIGHT 16 
#define LOGO16_SOL_WIDTH  16 
static const unsigned char PROGMEM logo16_sol_bmp[] =
{ 
  B00000000, B00000000,
  B01111111, B11111111,
  B01000000, B00000001,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01000000, B00000001,
  B01111111, B11111111,
  B00000000, B00000000 };

// Declare the glyph for the Rocket loop

#define LOGO16_ROCK_HEIGHT 16 
#define LOGO16_ROCK_WIDTH  16 
static const unsigned char PROGMEM logo16_rock_bmp[] =
{ 
  B00000010, B00000000,
  B00000011, B00000000,
  B00000001, B10000000,
  B00000001, B11000000,
  B00000001, B11100000,
  B00000111, B11110000,
  B00001111, B11110000,
  B00001111, B11111000,
  B00111111, B11111100,
  B00111111, B11111100,
  B00111111, B10111110,
  B00111111, B10111110,
  B00111111, B10011110,
  B00111011, B10001110,
  B00111001, B00001110,
  B00011000, B00011100 };

// Declare the glyph for the Evacuated Tubes loop 

#define LOGO16_SOLT_HEIGHT 16 
#define LOGO16_SOLT_WIDTH  16 
static const unsigned char PROGMEM logo16_solt_bmp[] =
{ 
  B00000000, B00000000,
  B01111111, B11111111,
  B01001000, B10001001,
  B01011101, B11011101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01011101, B11011101,
  B00001000, B10001000 };


#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

boolean tank = false;
boolean rocket = false;
boolean solpan = false;
boolean soltub = false;
boolean lights = false;
boolean grow = false;

int loopno = 5;
int loopRelay = 0;
int errcount = 0;

const int diffoff = 5;
const int diffon = 10;

#define relayGrow    4
#define relayTank    5
#define relayRocket  6
#define relaySolPan  7
#define relaySolTub  8
#define relayLights  1

// End preamble

void setup()
{

  // initialize the light sensor
  light.begin();

  // set gain to low
  gain = 0;

  // integration time of light sensor set to 402ms
  unsigned char time = 2;

  // setTiming() will set the third parameter (ms) to the
  // requested integration time in ms (this will be useful later):

  light.setTiming(gain,time,ms);

  // To start taking measurements, power up the sensor:

  light.setPowerUp();

  // Start up the library
  sensors.begin();

  // by default, we'll generate the high voltage from the 3.3v line internally!
  //  (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done
  display.clearDisplay();

  // declare the relay pin4 an output for the grey water pump
  pinMode(relayGrow, OUTPUT);

  // declare the relay pin5 an output for the LLH -> Tank pump
  pinMode(relayTank, OUTPUT);

  // declare the relay pin6 an output for the Rocket -> LLH pump
  pinMode(relayRocket, OUTPUT);

  // declare the relay pin7 an output for the Solar Panels -> LLH pump
  pinMode(relaySolPan, OUTPUT);

  // declare the relay pin8 an output for the Solar Tubes -> LLH pump
  pinMode(relaySolTub, OUTPUT);

  // declare the relay pin1 an output for the Lights
  pinMode(relayLights, OUTPUT);

  // set relays to off initially
  digitalWrite(relayGrow,HIGH);
  digitalWrite(relayTank,HIGH);
  digitalWrite(relayRocket,HIGH);
  digitalWrite(relaySolPan,HIGH);
  digitalWrite(relaySolTub,HIGH);
  digitalWrite(relayLights,HIGH);

  // Test all pumps, and the lights
  delay(1500);
  digitalWrite(relayGrow,LOW);
  delay(1000);
  digitalWrite(relayGrow,HIGH);
  delay(250);
  digitalWrite(relayTank,LOW);
  delay(1000);
  digitalWrite(relayTank,HIGH);
  delay(250);
  digitalWrite(relayRocket,LOW);
  delay(1000);
  digitalWrite(relayRocket,HIGH);
  delay(250);
  digitalWrite(relaySolPan,LOW);
  delay(1000);
  digitalWrite(relaySolPan,HIGH);
  delay(250);
  digitalWrite(relaySolTub,LOW);
  delay(1000);
  digitalWrite(relaySolTub,HIGH);
  delay(250);
  digitalWrite(relayLights,LOW);
  delay(1000);
  digitalWrite(relayLights,HIGH);
  delay(250);

  // End setup loop
}

void loop(void)
{

  //////////////////////////  
  // Start with the logic  
  //////////////////////////

  // There are two light sensors on the device, one for visible light
  // and one for infrared. Both sensors are needed for lux calculations.

  // Retrieve the data from the device:

  unsigned int data0, data1;
  double lux;    // Resulting lux value

  if (light.getData(data0,data1))
  {
    // getData() returned true, communication was successful

    // To calculate lux, pass all your settings and readings
    // to the getLux() function.

    // The getLux() function will return 1 if the calculation
    // was successful, or 0 if one or both of the sensors was
    // saturated (too much light). If this happens, you can
    // reduce the integration time and/or gain.
    // For more information see the hookup guide at: https://learn.sparkfun.com/tutorials/getting-started-with-the-tsl2561-luminosity-sensor

    boolean good;  // True if neither sensor is saturated

      // Perform lux calculation:

    good = light.getLux(gain,ms,data0,data1,lux);

    if (good == true && lux < 20) 
    {
      digitalWrite(relayLights,LOW); // switch the lights on 
      lights = true;
    }
    else
    {
      digitalWrite(relayLights,HIGH); // switch the lights off
      lights = false;
    }
  }
  else
  {
    // getData() returned false because of an I2C error, inform the user.

    byte error = light.getError();
    printError(error);
    lights = false;
  }

  // Set up the Grow timing loop - water pump should be on 3 times within the 75 cycles.

  // loop counter should be reset after 75 loops.

  if (loopno >= 75) {
    loopno = 0;
  }

  // add 1 to the loop counter, and return new value

  ++loopno;

  // Now check if we are at the first 4 cycles, or between 24 and 28 cycle, or between 
  //   50 and 54 cycle and if so, turn relay on. 
  // Otherwise, leave grow pump switched off.

  if (loopno <= 4 || (loopno > 24 && loopno <= 28) || (loopno > 50 && loopno <= 54)) {
    digitalWrite(relayGrow,LOW); // switch the relay and run the pump
    grow = true;
  }
  else
  {
    digitalWrite(relayGrow,HIGH); // don't switch the relay (no pump)
    grow = false;
  }


  //********************************************************************
  // Main water heating routines
  //********************************************************************

  // Get temperatures from all sensors on the bus
  sensors.requestTemperatures(); 

  //
  // Set variables for referring to each sensor on the bus
  //

  float tempSolar = sensors.getTempCByIndex(0);
  float tempTank = sensors.getTempCByIndex(1);
  float tempSolTub = sensors.getTempCByIndex(3);
  float tempLLH = sensors.getTempCByIndex(2);
  float tempRocket = sensors.getTempCByIndex(4);

  ///////////////////////////////  
  // Error detection algorithm
  ///////////////////////////////

  //
  // Check all sensors - if any are exactly the same value, declare an error 
  //  state (can happen if there is a bad connection to a sensor)
  // Error state of the onewire temp sensor is 85 degrees, however, I have also 
  //  seen temps of -127 reported: check for both and declare an error state
  // Error state declared when the tank temperature reaches above 65degrees
  // Error state declared when the rocket stove, Solar panel or Solar Tubes are 
  //  reporting a temperature above 95degrees
  // Error state is the activation of all pumps.
  // Even in an error state, pumps are allowed to rest for roughly 30 seconds in
  //  every 5 minutes (95% duty cycle) using the loop counter.
  //

  if (tempTank==-127.00 || 
    tempLLH==-127.00 || 
    tempSolar==-127.00 || 
    tempRocket==-127.00 || 
    tempSolTub==-127.00 ||
    tempLLH==85.00 || 
    tempSolar==85.00 || 
    tempRocket==85.00 ||
    tempSolTub==85.00 ||
    tempTank==tempLLH || 
    tempTank==tempSolar || 
    tempTank==tempRocket || 
    tempTank==tempSolTub ||
    tempLLH==tempSolar || 
    tempLLH==tempRocket ||
    tempLLH==tempSolTub ||
    tempSolar==tempRocket ||
    tempSolar==tempSolTub ||
    tempRocket==tempSolTub)
  {
    ++errcount; // error state is counted if any of the sensors report an error temperature
  } 
  else { 
    errcount=0; // if no sensors are reporting an error, reset counter
  };

  if (errcount>=3 ||   // once a temperature error state has existed for 3 cycles or more, 
  tempRocket>=95.00 || // start motors, otherwise ignore and treat as transient error 
  tempSolar>=95.00 ||  // unless rocket or panels, or tubes or tank are overheated then 
  tempSolTub>=95.00 || // immediately pump for all you're worth!
  tempTank>=65.00)
  {
    if (loopno < 5) {  // let pumps rest for a few cycles even in an error state

      delay(250);
      digitalWrite(relayTank,HIGH);   // switch the tank pump off
      delay(250);
      digitalWrite(relayRocket,HIGH); // switch the rocket pump off
      delay(250);
      digitalWrite(relaySolPan,HIGH); // switch the solar panel pump off
      delay(250);
      digitalWrite(relaySolTub,HIGH); // switch the solar tubes pump off

      tank = false;
      solpan = false;
      rocket = false;
      soltub = false;

    }
    else
    {

      delay(250);
      digitalWrite(relayTank,LOW);   // switch the tank pump on
      delay(250);
      digitalWrite(relayRocket,LOW); // switch the rocket pump on
      delay(250);
      digitalWrite(relaySolPan,LOW); // switch the solar panel pump on
      delay(250);
      digitalWrite(relaySolTub,LOW); // switch the solar tubes pump on

      tank = true;
      solpan = true;
      rocket = true;
      soltub = true;
    }
    // Briefly display current error state

      display.setTextWrap(false);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("Oops! We've");
    display.println("a problem.");
    display.println("Please call");
    display.println("@farmhouse");
    display.display();  

    delay(3000);

    display.setTextWrap(false);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.print("LLH: ");
    display.println(tempLLH);
    display.print("SPan:");
    display.println(tempSolar);
    display.print("STub:");
    display.println(tempSolTub);
    display.print("Rock:");
    display.println(tempRocket);
    display.display();  

    delay(750);

    display.setTextWrap(false);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.print("Tank:");
    display.println(tempTank);
    display.print("Err: ");
    display.println(errcount);
    display.print("Loop:");
    display.println(loopno);
    display.print("Relay:");
    display.println(loopRelay);
    display.display();  

    delay(750);

    // Jump to end of loop

    goto bailloop;

  };

  ///////////////////////////////  
  // End of Error detection 
  ///////////////////////////////

  ///////////////////////////////  
  // Start of Pump logic 
  ///////////////////////////////

  // loop counter should be reset after 8 loops.

  if (loopRelay >= 9) {
    loopRelay = 0;
  }

  // add 1 to the loop counter, and return new value

  ++loopRelay;

  //
  // If LLH is warmer than Tank+5 degrees, then start the Tank pump
  //

  delay(250);
  if (tempTank>=63)
  {
    if ((loopRelay == 1 || 
      loopRelay == 2 || 
      loopRelay == 4 || 
      loopRelay == 5 || 
      loopRelay == 7 || 
      loopRelay == 8 )) 
    {
      digitalWrite(relayTank,LOW); // switch the tank pump on 
      tank = true;
    }
    else
    {
      digitalWrite(relayTank,HIGH); // switch the tank pump off
      tank = false;
    }
  }
  else if ((tempTank<25 && tempLLH>(tempTank+5)) ||
    (tempTank<35 && tempLLH>(tempTank+10)) || 
    (tempTank>35 && tempTank<55 && tempLLH>(tempTank+15)) ||
    (tempTank>55 && tempLLH>(tempTank+10)))
  {
    if (tempLLH < 70)
    {
      if ((loopRelay == 2 || 
        loopRelay == 6 )) 
      {
        digitalWrite(relayTank,LOW); // switch the tank pump on
        tank = true;
      }
      else
      {
        digitalWrite(relayTank,HIGH); // switch the tank pump off
        tank = false;
      }
    }
    else if (tempLLH >= 70 && tempLLH < 75)
    {
      if ((loopRelay == 2 || 
        loopRelay == 5 || 
        loopRelay == 8 )) 
      {
        digitalWrite(relayTank,LOW); // switch the tank pump on
        tank = true;
      }
      else
      {
        digitalWrite(relayTank,HIGH); // switch the tank pump off
        tank = false;
      }
    }
    else if (tempLLH>=90)
    {
      if ((loopRelay == 2 || 
        loopRelay == 3 || 
        loopRelay == 5 || 
        loopRelay == 6 || 
        loopRelay == 8 || 
        loopRelay == 9 )) 
      {
        digitalWrite(relayTank,LOW); // switch the tank pump on
        tank = true;
      }
      else
      {
        digitalWrite(relayTank,HIGH); // switch the tank pump off
        tank = false;
      }
    }
    else
    {
      if ((loopRelay == 2 || 
        loopRelay == 4 || 
        loopRelay == 6 || 
        loopRelay == 8 )) 
      {
        digitalWrite(relayTank,LOW); // switch the tank pump on
        tank = true;
      }
      else
      {
        digitalWrite(relayTank,HIGH); // switch the tank pump off
        tank = false;
      }
    }

  }
  else
  {
    digitalWrite(relayTank,HIGH); // switch the tank pump off
    tank = false;
  };


  //
  // If Solar is warmer than LLH+10 degrees, then start the Solar pump
  //

  delay(250);
  if (tempTank>=63)
  {
    if ((loopRelay == 1 || 
      loopRelay == 2 || 
      loopRelay == 4 || 
      loopRelay == 5 || 
      loopRelay == 7 || 
      loopRelay == 8 )) 
    {
      digitalWrite(relaySolPan,LOW); // switch the solar panel pump on 
      solpan = true;
    }
    else
    {
      digitalWrite(relaySolPan,HIGH); // switch the solar panel pump off
      solpan = false;
    }
  }
  else if (tempSolar>(tempLLH+5))
  {
    if (tempSolar<65)
    {
      if (loopRelay == 1) 
      {
        digitalWrite(relaySolPan,LOW); // switch the solar panel pump on 
        solpan = true;
      }
      else
      {
        digitalWrite(relaySolPan,HIGH); // switch the solar panel pump off
        solpan = false;
      }
    }
    else if (tempSolar>=90)
    {
      if ((loopRelay == 1 || 
        loopRelay == 2 || 
        loopRelay == 4 || 
        loopRelay == 5 || 
        loopRelay == 7 || 
        loopRelay == 8 )) 
      {
        digitalWrite(relaySolPan,LOW); // switch the solar panel pump on 
        solpan = true;
      }
      else
      {
        digitalWrite(relaySolPan,HIGH); // switch the solar panel pump off
        solpan = false;
      }
    }
    else
    {
      if ((loopRelay == 1 || 
        loopRelay == 3 || 
        loopRelay == 5 || 
        loopRelay == 7 )) 
      {
        digitalWrite(relaySolPan,LOW); // switch the solar panel pump on 
        solpan = true;
      }
      else
      {
        digitalWrite(relaySolPan,HIGH); // switch the solar panel pump off
        solpan = false;
      }
    }
  }
  else 
  {
    digitalWrite(relaySolPan,HIGH); // switch the solar panel pump off
    solpan = false;
  };

  //
  // If Solar Tube is warmer than LLH+5 degrees, then start the Solar Tube pump
  //

  delay(250);
  if (tempTank>=63)
  {
    if ((loopRelay == 1 || 
      loopRelay == 2 || 
      loopRelay == 4 || 
      loopRelay == 5 || 
      loopRelay == 7 || 
      loopRelay == 8 )) 
    {
      digitalWrite(relaySolTub,LOW); // switch the solar tubes pump on 
      soltub = true;
    }
    else
    {
      digitalWrite(relaySolTub,HIGH); // switch the solar tubes pump off
      soltub = false;
    }
  }
  else if (tempSolTub>(tempLLH+5))
  {
    if (tempSolTub<65)
    {
      if (loopRelay == 1) 
      {
        digitalWrite(relaySolTub,LOW); // switch the solar tubes pump on
        soltub = true;
      }
      else
      {
        digitalWrite(relaySolTub,HIGH); // switch the solar tubes pump off
        soltub = false;
      }
    }
    else if (tempSolTub>=65 && tempSolTub<75)
    {
      if ((loopRelay == 1 || 
        loopRelay == 5 )) 
      {
        digitalWrite(relaySolTub,LOW); // switch the solar tubes pump on
        soltub = true;
      }
      else
      {
        digitalWrite(relaySolTub,HIGH); // switch the solar tubes pump off
        soltub = false;
      }
    }
    else if (tempSolTub>=75 && tempSolTub<85)
    {
      if ((loopRelay == 1 || 
        loopRelay == 4 || 
        loopRelay == 7 )) 
      {
        digitalWrite(relaySolTub,LOW); // switch the solar tubes pump on
        soltub = true;
      }
      else
      {
        digitalWrite(relaySolTub,HIGH); // switch the solar tubes pump off
        soltub = false;
      }
    }
    else if (tempSolTub>=90)
    {
      if ((loopRelay == 1 || 
        loopRelay == 2 || 
        loopRelay == 4 || 
        loopRelay == 5 || 
        loopRelay == 7 || 
        loopRelay == 8 )) 
      {
        digitalWrite(relaySolTub,LOW); // switch the solar tubes pump on
        soltub = true;
      }
      else
      {
        digitalWrite(relaySolTub,HIGH); // switch the solar tubes pump off
        soltub = false;
      }
    }
    else
    {
      if ((loopRelay == 1 || 
        loopRelay == 3 || 
        loopRelay == 5 || 
        loopRelay == 7 )) 
      {
        digitalWrite(relaySolTub,LOW); // switch the solar tubes pump on
        soltub = true;
      }
      else
      {
        digitalWrite(relaySolTub,HIGH); // switch the solar tubes pump off
        soltub = false;
      }
    }

  }
  else
  {
    digitalWrite(relaySolTub,HIGH); // switch the solar tubes pump off
    soltub = false;
  };


  //
  // If Rocket is warmer than LLH+5 degrees, then start the Rocket pump
  //

  delay(250);
  if (tempTank>=63)
  {
    if ((loopRelay == 1 || 
      loopRelay == 2 || 
      loopRelay == 4 || 
      loopRelay == 5 || 
      loopRelay == 7 || 
      loopRelay == 8 )) 
    {
      digitalWrite(relayRocket,LOW); // switch the rocket pump on 
      rocket = true;
    }
    else
    {
      digitalWrite(relayRocket,HIGH); // switch the rocket pump off
      rocket = false;
    }
  }
  else if (tempRocket>(tempLLH+5))
  {
    if (tempRocket<65)
    {
      if (loopRelay == 1) 
      {
        digitalWrite(relayRocket,LOW); // switch the rocket pump on
        rocket = true;
      }
      else
      {
        digitalWrite(relayRocket,HIGH); // switch the rocket pump off
        rocket = false;
      }
    }
    else if (tempRocket>=65 && tempRocket<75)
    {
      if ((loopRelay == 1 || 
        loopRelay == 5 )) 
      {
        digitalWrite(relayRocket,LOW); // switch the rocket pump on
        rocket = true;
      }
      else
      {
        digitalWrite(relayRocket,HIGH); // switch the rocket pump off
        rocket = false;
      }
    }
    else if (tempRocket>=70 && tempRocket<80)
    {
      if ((loopRelay == 1 || 
        loopRelay == 4 || 
        loopRelay == 7 )) 
      {
        digitalWrite(relayRocket,LOW); // switch the rocket pump on
        rocket = true;
      }
      else
      {
        digitalWrite(relayRocket,HIGH); // switch the rocket pump off
        rocket = false;
      }
    }
    else if (tempRocket>=85)
    {
      if ((loopRelay == 1 || 
        loopRelay == 2 || 
        loopRelay == 4 || 
        loopRelay == 5 || 
        loopRelay == 7 || 
        loopRelay == 8 )) 
      {
        digitalWrite(relayRocket,LOW); // switch the rocket pump on
        rocket = true;
      }
      else
      {
        digitalWrite(relayRocket,HIGH); // switch the rocket pump off
        rocket = false;
      }
    }
    else
    {
      if ((loopRelay == 1 || 
        loopRelay == 3 || 
        loopRelay == 5 || 
        loopRelay == 7 )) 
      {
        digitalWrite(relayRocket,LOW); // switch the rocket pump on
        rocket = true;
      }
      else
      {
        digitalWrite(relayRocket,HIGH); // switch the rocket pump off
        rocket = false;
      }
    }

  }
  else
  {
    digitalWrite(relayRocket,HIGH); // switch the rocket pump off
    rocket = false;
  };


  ///////////////////////////////  
  // Main Display Loop
  ///////////////////////////////

  //
  // Display temperature of the water tank
  //

  display.setTextWrap(false);
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(tempTank,0);
  display.write(9);
  display.println("C");
  display.setTextSize(2);
  display.setCursor(4,48);
  display.print("Water Temp.");
  display.display();

  delay(2000);

  //
  // Display campsite lights state
  //

  if (lights == true) {
    display.setTextWrap(false);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("Campsite");
    display.println("lights are");
    display.print("on @ ");
    display.print(round(lux));
    display.println("lux");
    display.display();
    delay(500);  
  }
  else
  {
    display.setTextWrap(false);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("Campsite");
    display.println("lights are");
    display.println("off @ ");
    display.print(round(lux));
    display.println("lux");
    display.display();
    delay(500);  
  };

  //
  // Show all temperatures as reported (2 decimal places)
  //

  display.setTextWrap(false);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print("LLH: ");
  display.println(tempLLH);
  display.print("SPan:");
  display.println(tempSolar);
  display.print("STub:");
  display.println(tempSolTub);
  display.print("Rock:");
  display.println(tempRocket);
  display.display();  

  delay(1000);

  //
  // Detect if any pumps are currently running
  //


  // Animated display routine
  // Display the pump running animation for the relevant pump(s) and pump
  //   glyphs for 10 cycles


  for (int i=0; i <= 10; i++){
    display.setTextWrap(false);
    display.setTextSize(5);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.print(tempTank,0);
    display.write(9);
    display.println("C");
    display.setTextSize(2);
    if (tank == true){
      display.setCursor(18,48);
      display.print("/");
    }
    else{
      display.setCursor(18,48);
      display.print(" "); 
    };
    if (solpan == true){
      display.setCursor(50,48);
      display.print("-");
    }
    else{
      display.setCursor(50,48);
      display.print(" "); 
    };
    if (soltub == true){
      display.setCursor(82,48);
      display.print("\\");
    }
    else{
      display.setCursor(82,48);
      display.print(" "); 
    };
    if (rocket == true){
      display.setCursor(114,48);
      display.print("|");
    }
    else{
      display.setCursor(114,48);
      display.print(" "); 
    };
    display.drawBitmap(0, 46,  logo16_tank_bmp, 16, 16, 1);
    display.drawBitmap(32, 46,  logo16_sol_bmp, 16, 16, 1);
    display.drawBitmap(64, 46,  logo16_solt_bmp, 16, 16, 1);
    display.drawBitmap(96, 46,  logo16_rock_bmp, 16, 16, 1);
    display.display();

    delay(50);

    display.setTextWrap(false);
    display.setTextSize(5);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.print(tempTank,0);
    display.write(9);
    display.println("C");
    display.setTextSize(2);
    if (tank == true){
      display.setCursor(18,48);
      display.print("-");
    }
    else{
      display.setCursor(18,48);
      display.print(" "); 
    };
    if (solpan == true){
      display.setCursor(50,48);
      display.print("\\");
    }
    else{
      display.setCursor(50,48);
      display.print(" "); 
    };
    if (soltub == true){
      display.setCursor(82,48);
      display.print("|");
    }
    else{
      display.setCursor(82,48);
      display.print(" "); 
    };
    if (rocket == true){
      display.setCursor(114,48);
      display.print("/");
    }
    else{
      display.setCursor(114,48);
      display.print(" "); 
    };
    display.drawBitmap(0, 46,  logo16_tank_bmp, 16, 16, 1);
    display.drawBitmap(32, 46,  logo16_sol_bmp, 16, 16, 1);
    display.drawBitmap(64, 46,  logo16_solt_bmp, 16, 16, 1);
    display.drawBitmap(96, 46,  logo16_rock_bmp, 16, 16, 1);
    display.display();

    delay(50);

    display.setTextWrap(false);
    display.setTextSize(5);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.print(tempTank,0);
    display.write(9);
    display.println("C");
    display.setTextSize(2);
    if (tank == true){
      display.setCursor(18,48);
      display.print("\\");
    }
    else{
      display.setCursor(18,48);
      display.print(" "); 
    };
    if (solpan == true){
      display.setCursor(50,48);
      display.print("|");
    }
    else{
      display.setCursor(50,48);
      display.print(" "); 
    };
    if (soltub == true){
      display.setCursor(82,48);
      display.print("/");
    }
    else{
      display.setCursor(82,48);
      display.print(" "); 
    };
    if (rocket == true){
      display.setCursor(114,48);
      display.print("-");
    }
    else{
      display.setCursor(114,48);
      display.print(" "); 
    };
    display.drawBitmap(0, 46,  logo16_tank_bmp, 16, 16, 1);
    display.drawBitmap(32, 46,  logo16_sol_bmp, 16, 16, 1);
    display.drawBitmap(64, 46,  logo16_solt_bmp, 16, 16, 1);
    display.drawBitmap(96, 46,  logo16_rock_bmp, 16, 16, 1);
    display.display();

    delay(50);

    display.setTextWrap(false);
    display.setTextSize(5);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.print(tempTank,0);
    display.write(9);
    display.println("C");
    display.setTextSize(2);
    if (tank == true){
      display.setCursor(18,48);
      display.print("|");
    }
    else{
      display.setCursor(18,48);
      display.print(" "); 
    };
    if (solpan == true){
      display.setCursor(50,48);
      display.print("/");
    }
    else{
      display.setCursor(50,48);
      display.print(" "); 
    };
    if (soltub == true){
      display.setCursor(82,48);
      display.print("-");
    }
    else{
      display.setCursor(82,48);
      display.print(" "); 
    };
    if (rocket == true){
      display.setCursor(114,48);
      display.print("\\");
    }
    else{
      display.setCursor(114,48);
      display.print(" "); 
    };
    display.drawBitmap(0, 46,  logo16_tank_bmp, 16, 16, 1);
    display.drawBitmap(32, 46,  logo16_sol_bmp, 16, 16, 1);
    display.drawBitmap(64, 46,  logo16_solt_bmp, 16, 16, 1);
    display.drawBitmap(96, 46,  logo16_rock_bmp, 16, 16, 1);
    display.display();

  };

  //
  // Hack to ensure smooth looping of the animation and no fixed frame while 
  //  restarting arduino main loop - set display to show main tank temperature
  //

  display.setTextWrap(false);
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(tempTank,0);
  display.write(9);
  display.println("C");
  display.setTextSize(2);
  display.setCursor(4,48);
  display.print("Water Temp.");
  display.display();


bailloop:
  // End Main loop
  ;
};

void printError(byte error)
// If there's an I2C error, this function will
// print out an explanation.
{
  //  Serial.print("I2C error: ");
  //  Serial.print(error,DEC);
  //  Serial.print(", ");

  switch(error)
  {
  case 0:
    //      Serial.println("success");
    break;
  case 1:
    //      Serial.println("data too long for transmit buffer");
    break;
  case 2:
    //      Serial.println("received NACK on address (disconnected?)");
    break;
  case 3:
    //      Serial.println("received NACK on data");
    break;
  case 4:
    //      Serial.println("other error");
    break;
  default:
    //      Serial.println("unknown error")
    ;
  }
}
