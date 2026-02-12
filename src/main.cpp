#include <Arduino.h> // Tohle je mozna potreba odstranit pri spousteni v Arduino IDE

// Dratky:
//  zelena - zem
//  zluta - input do arduina
//  oranzova - output z arduina

// zelena - u prijmace - A2 - oranzova
// modra - u zapalovace - A3 - zluta
// oranzova - uprostred - A1 - hneda
// vlevo zapalovac, vpravo prijmac

/*
---- Vstupy ----

Vytisknout teploty:
  't'

Zapnout zapalovac na <DEFAULT_LIGHTER_MILIS>:
  'f'

Zapnout zapalovac na <lighterMs> n-krat s prodlevama d (ms):
  'd <lighterMs> <d_1> <d_2> ... <d_n>'


---- Vystupy ----
Teploty thermistoru v C, kde n je N_THERMISTORS:
  'T <t_1> <t_2> ... <t_n>'

Zapalovac zapnut:
  'S <cTimeMs>'

Zapalovac vypnut:
  'E <elapsedTimeUs> <cTimeMs>'

Zmena na senzoru:
  'F <0|1> <cTimeMs>'

Chybova hlaska:
  'O <text>'
*/

#ifdef CONFIG_UNO
#define LIGHTER_PIN 6
#define FLAME_SENSOR_PIN 7
#define FLAME_SENSOR_PIN_MODE INPUT_PULLUP
#define N_THERMISTORS 4
#define THERMISTOR_PINS {A3,A1,A2,A0}
#endif
#ifdef CONFIG_ESP_TEST
#define LIGHTER_PIN 1
#define FLAME_SENSOR_PIN 0
#define FLAME_SENSOR_PIN_MODE INPUT
#define N_THERMISTORS 4
#define THERMISTOR_PINS {4,5,6,7}
#endif

#define FLAME_SENSOR_ERROR_TIMER 1.5 // Multiple of the lighter on time, specifying the period (from the moment the lighter starts) when data from the flame sensor will be ignored 

#define DEFAULT_LIGHTER_MILIS 10 // Used when 'f' is pressed

#define N_RELEVANT_THERMISTORS (N_THERMISTORS-1) // The first N thermistors are relevant, the last is ambient temp
const int thermistorPins[N_THERMISTORS] = THERMISTOR_PINS;

#define MAX_N_DELAYS 32 // Arduino UNO ma jenom 2kb RAM, takze to moc nezvedejte
unsigned int delaysMs[MAX_N_DELAYS] = {}; // ms
int nDelays = 0;

#define DIODES_PIN 3

#ifdef CONFIG_ESP_TEST
#include <Adafruit_NeoPixel.h>
#endif

class Timer {
  public:
    unsigned long startMicros;
  
    unsigned long elapsedMicros()
    {
      return micros()-startMicros;
    }

    unsigned long elapsedMillis()
    {
      return (micros()-startMicros)/1000;
    }

    double elapsedSeconds()
    {
      return (micros()-startMicros)/1E06;
    }

    unsigned long start()
    {
      return startMicros = micros();
    }
};

void setup() {
  pinMode(FLAME_SENSOR_PIN, FLAME_SENSOR_PIN_MODE);
  pinMode(LIGHTER_PIN, OUTPUT);
  pinMode(DIODES_PIN, OUTPUT);
  Serial.begin(9600);
}

double resistanceToCelsius(double r)
{
  // Hodnoty z ts datasheetu https://img.gme.cz/files/eshop_data/eshop_data/2/118-042/dsh.118-042.1.pdf
  const double A_1 = 3.354016E-03;
  const double B_1 = 2.569850E-04;
  const double C_1 = 2.620131E-06;
  const double D_1 = 6.383091E-08;
  const double R_ref = 10000;
  
  double LN = log(r/R_ref);

  double T_k = 1.0/(A_1 + B_1*LN + C_1*LN*LN + D_1*LN*LN*LN);

  return T_k-273.15;
}

double readCelsiusTemp(int pin)
{
  int thermistor_adc_val = analogRead(pin);
  double output_voltage = ( (thermistor_adc_val * 5.0) / 1023.0 ); // Toto predpoklada ADC rozliseni 1024, coz je na Ardiuno UNO myslim vzdycky
  double thermistor_resistance = (9100.0*output_voltage/(5-output_voltage));
  return resistanceToCelsius(thermistor_resistance);
}

void printTime()
{
  Serial.print(millis());
}

Timer lighterTimer;
bool sensorVal = true;
int delayIndex = 0;
unsigned long lighterMicros = DEFAULT_LIGHTER_MILIS*1000;
bool diodesOn = false;
bool thermostatOn = false;

void printThermistors()
{
  Serial.print("T ");
  for (size_t i = 0; i < N_THERMISTORS; i++)
  {
    Serial.print(readCelsiusTemp(thermistorPins[i]));
    Serial.print(" ");
  }
  if (diodesOn) Serial.print("D");
  Serial.println();
}


enum State { 
  WAITING_FOR_INPUT, // The lighting sequence finnished, but still might be waiting for the last flame to arrive
  LIGHTER_ON, // The lighter is on
  LIGHTER_OFF, // The lighter is off, but it will be turned on again
};
State state = WAITING_FOR_INPUT;

void turnLighterOn()
{
  if (state == LIGHTER_ON) return;
  lighterTimer.start();
  #ifdef CONFIG_UNO
  digitalWrite(LIGHTER_PIN, HIGH);
  #endif
  #ifdef CONFIG_ESP_TEST
  neopixelWrite(LED_BUILTIN, 100, 0, 0);
  #endif
  state = LIGHTER_ON;

  Serial.println();
  Serial.print("S ");
  printTime();
  Serial.println();
  printThermistors();
}

void turnLighterOff()
{
  if (state == LIGHTER_OFF) return;
  #ifdef CONFIG_UNO
  digitalWrite(LIGHTER_PIN, LOW);
  #endif
  #ifdef CONFIG_ESP_TEST
  neopixelWrite(LED_BUILTIN, 0, 0, 0);
  #endif
  state = LIGHTER_OFF;

  Serial.print("E ");
  Serial.print(lighterTimer.elapsedMicros());
  Serial.print(" ");
  printTime();
  Serial.println();
}

void keepTemp(double goalTemp)
{
  double avg;
  for (size_t i = 0; i < N_RELEVANT_THERMISTORS; i++)
  {
    avg += readCelsiusTemp(thermistorPins[i]);
  }
  avg = avg/N_RELEVANT_THERMISTORS;
  if (avg < goalTemp) diodesOn=true;
  else diodesOn=false;
}

void begin()
{
  delayIndex = 0;
  turnLighterOn();
}

void end()
{
  turnLighterOff();
  state = WAITING_FOR_INPUT;
}

void onWaitingForInput() // Checking for serial input
{
  if (Serial.available() > 0)
  {
    int b = Serial.read();
    Serial.print("O Recieved: ");
    if (b != '\n' && b != '\r') Serial.print((char)b);
    Serial.print(" - ");
    printTime();
    Serial.println();

    if ((char)b == 't')
    {
      printThermistors();
    }

    if ((char)b == 'b')
    {
      diodesOn = !diodesOn;
      if (diodesOn) digitalWrite(DIODES_PIN, HIGH);
      else digitalWrite(DIODES_PIN, LOW);
      Serial.print("O Diodes ");
      if (diodesOn) Serial.println("ON");
      else Serial.println("OFF");
    }

    // Starting the lighter
    if ((char)b == 'f')
    {
      nDelays = 0;
      lighterMicros = DEFAULT_LIGHTER_MILIS*1000;
      turnLighterOn();
      begin();
    }

    if ((char)b == 'g')
    {
      thermostatOn = !thermostatOn;
      if (thermostatOn) Serial.println("O thermostat ON");
      else 
      {
        Serial.println("O thermostat OFF");
        diodesOn = false;
      }
    }

    if ((char)b == 'd')
    {
      String s = "";
      unsigned long lighterMs = 0;
      int spaces = 0;
      while (true)
      {
        b = Serial.read();
        if (b == -1) continue;
        if (b != (int)' ' && b != (int)'\n')
        {
          s += (char)b;
          continue;
        }
        
        
        if (spaces == 0) lighterMs = s.toInt();
        else delaysMs[spaces-1] = s.toInt();
        spaces++;
        s = "";

        if (b == (int)'\n') break;
        if (spaces > MAX_N_DELAYS) 
        {
          Serial.println("O Too much data!!");
          break;
        }

      }
      
      lighterMicros = lighterMs*1000;
      nDelays = spaces - 1;
      
      // Serial.print("O spaces ");
      // Serial.print(spaces);
      // Serial.print(" | ");
      // Serial.print(nDelays);
      // Serial.print(" | ");
      // Serial.println(delaysMs[0]);

      begin();
    }
  }
}

void loop() {
  if (thermostatOn) keepTemp(40);
  if (diodesOn) digitalWrite(DIODES_PIN, HIGH);
  else digitalWrite(DIODES_PIN, LOW);


  if (state == WAITING_FOR_INPUT)
  {
    onWaitingForInput();
  }

  // ---- Reading flame sensor ----
  if (lighterTimer.elapsedMicros() > lighterMicros*FLAME_SENSOR_ERROR_TIMER)
  {
    bool newSensorVal = digitalRead(FLAME_SENSOR_PIN);
    if (newSensorVal != sensorVal)
    {
      sensorVal = newSensorVal;
      if (sensorVal) Serial.print("F 1 ");
      else Serial.print("F 0 ");
      printTime();
      Serial.println();
    }
  }

  // ---- Checking if the lighter timer finnished ----
  if (state == LIGHTER_ON)
  {
    if (lighterTimer.elapsedMicros() > lighterMicros)
    {
      turnLighterOff();
    }
  }

  if (state == LIGHTER_OFF)
  {
    if (delayIndex >= nDelays) end(); // All shots fired
    else
    {
      if (lighterTimer.elapsedMicros() > (unsigned long)delaysMs[delayIndex]*1000UL)
      {
        // Serial.print("O ELAPSED ");
        // Serial.print(lighterTimer.elapsedMicros());
        // Serial.print(" Delay ");
        // Serial.print(delaysMs[delayIndex]);
        // Serial.print(" Delay*1000 ");
        // Serial.print(delaysMs[delayIndex]*1000);
        // Serial.print(" UL-Delay*1000 ");
        // Serial.print((unsigned long)delaysMs[delayIndex]*1000UL);
        // Serial.print(" INDEX: ");
        // Serial.println(delayIndex);
        turnLighterOn();
        delayIndex++;
      }
    }
  }
}