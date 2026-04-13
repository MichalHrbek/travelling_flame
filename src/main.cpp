#include <Arduino.h>

#define LIGHTER_PIN 6
#define FLAME_SENSOR_PIN 7
#define N_THERMISTORS 4
#define THERMISTOR_PINS {A3,A1,A2,A0}
#define NORM_VOLTAGE 5.0
#define HEATER_PIN 3

#define FLAME_SENSOR_ERROR_TIMER 2 // Multiple of the lighter on time, specifying the period (from the moment the lighter starts) when data from the flame sensor will be ignored 

#define DEFAULT_LIGHTER_MILIS 200UL // Used when 'f' is pressed

#define N_RELEVANT_THERMISTORS (N_THERMISTORS-1) // The first N thermistors are relevant, the last is ambient temp
const int thermistorPins[N_THERMISTORS] = THERMISTOR_PINS;
double temperatureValues[N_THERMISTORS] = {};
double avgTemp;

#define MAX_N_DELAYS 24 // Arduino UNO ma jenom 2kb RAM, takze to moc nezvedejte
long delaysMs[MAX_N_DELAYS] = {}; // ms
int nDelays = 0;

#define MIN_AUTO_DELAY_MS 15000
#define AUTO_TEMP_RANGE 1.0
uint8_t nAutoFlames = 0;

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
  pinMode(FLAME_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LIGHTER_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.setTimeout(0xffffff);
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
  double output_voltage = ( (thermistor_adc_val * NORM_VOLTAGE) / 1023.0 ); // Toto predpoklada ADC rozliseni 1024, coz je na Ardiuno UNO myslim vzdycky
  double thermistor_resistance = (9100.0*output_voltage/(NORM_VOLTAGE-output_voltage));
  return resistanceToCelsius(thermistor_resistance);
}

void updateTemps()
{
  avgTemp = 0.0;
  for (size_t i = 0; i < N_THERMISTORS; i++)
  {
    temperatureValues[i] = readCelsiusTemp(thermistorPins[i]);
    if (i < N_RELEVANT_THERMISTORS)
    {
      avgTemp += temperatureValues[i];
    }
  }
  avgTemp /= (double)N_RELEVANT_THERMISTORS;
}

void printTime()
{
  Serial.print(millis());
}

Timer lighterTimer;
bool sensorVal = true;
int delayIndex = 0;
unsigned long lighterMicros = DEFAULT_LIGHTER_MILIS*1000UL;
bool heaterOn = false;

bool thermostatOn = false;
double goalTemp = 40.0;

int tempTesting = 0;
Timer tempTestingTimer;

void printThermistors(Print* s)
{
  s->print("T ");
  for (size_t i = 0; i < N_THERMISTORS; i++)
  {
    s->print(temperatureValues[i]);
    s->print(" ");
  }
  if (heaterOn) s->print("H");
  s->println();
}

void printThermistorsInstant(Print* s)
{
  s->print("I ");
  s->print(millis());
  s->print(" ");
  for (size_t i = 0; i < N_THERMISTORS; i++)
  {
    s->print(temperatureValues[i]);
    s->print(" ");
  }
  s->println();
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
  digitalWrite(LIGHTER_PIN, HIGH);
  state = LIGHTER_ON;

  Serial.println();
  Serial.print("S ");
  printTime();
  Serial.println();
  printThermistors(&Serial);
}

void turnLighterOff()
{
  if (state == LIGHTER_OFF) return;
  digitalWrite(LIGHTER_PIN, LOW);
  state = LIGHTER_OFF;
  
  Serial.print("E ");
  Serial.print(lighterTimer.elapsedMicros());
  Serial.print(" ");
  printTime();
  Serial.println();
}

void keepTemp()
{
  if (avgTemp < goalTemp) heaterOn = true;
  else heaterOn = false;
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

long readInt()
{
  while (true)
  {
    long val = Serial.parseInt();
    if (val) return val;
  }
}

bool readInts(long* value) // Reads an int and checks if the following character was a newline or a space
{
  while (true)
  {
    long read = Serial.parseInt();
    if (!read) continue;
    *value = read;
    while (true)
    {
      int b = Serial.read();
      if (b == -1) continue;
      if (b == ' ') return true;
      if (b == '\n') return false;
    }
  }
}

void sendSingleFlame()
{
  nDelays = 0;
  lighterMicros = DEFAULT_LIGHTER_MILIS*1000;
  begin();
}

void processSerialInput()
{
  if (Serial.available() > 0)
  {
    tempTesting = 0;

    int b = Serial.read();
    Serial.print("O Recieved: ");
    if (b != '\n' && b != '\r') Serial.print((char)b);
    Serial.print(" - ");
    printTime();
    Serial.println();

    switch ((char)b)
    {
    case 't':
      printThermistors(&Serial);
      break;
    
    case 'h':
      heaterOn = !heaterOn;
      if (heaterOn) digitalWrite(HEATER_PIN, HIGH);
      else digitalWrite(HEATER_PIN, LOW);
      Serial.print("O Heater ");
      if (heaterOn) Serial.println("ON");
      else Serial.println("OFF");
      break;
    
    case 'f':
      sendSingleFlame();
      break;
    
    case 'g':
      thermostatOn = !thermostatOn;
      if (thermostatOn) Serial.println("O thermostat ON");
      else 
      {
        Serial.println("O thermostat OFF");
        heaterOn = false;
      }
      break;

    case 's':
      goalTemp = readInt();
      while (Serial.read() != '\n');
      Serial.print(thermostatOn ? "O thermostat (ON) set to " : "O thermostat (OFF) set to ");
      Serial.println(goalTemp);
      break;
    
    case 'a':
      nAutoFlames = readInt();
      while (Serial.read() != '\n');
      Serial.print("O nAutoFlames: ");
      Serial.println(nAutoFlames);
      break;
    
    case 'd':
    {
      String s = "";
      nDelays = 0;
      
      bool shouldContinue = readInts((long*)&lighterMicros);
      lighterMicros *= 1000UL;
      Serial.print("O lighter ");
      Serial.println(lighterMicros);
      while (shouldContinue)
      {
        shouldContinue = readInts(&delaysMs[nDelays++]);
        Serial.print("O delay no. ");
        Serial.print(nDelays-1);
        Serial.print(" = ");
        Serial.println(delaysMs[nDelays-1]);
        if (nDelays == MAX_N_DELAYS)
        {
          Serial.println("O Too much data!!");
          break;
        }
      }

      begin();
      break;
    }
    
    case '1':
      tempTesting = 1;
      tempTestingTimer.start();
      break;
    }
  }
}

void loop() {
  updateTemps();
  if (tempTesting)
  {
    printThermistorsInstant(&Serial);
    if (tempTesting == 1 && tempTestingTimer.elapsedMillis() > 3000)
    {
      tempTesting = 2;
      sendSingleFlame();
    }
  }
  if (thermostatOn) keepTemp();
  if (heaterOn) digitalWrite(HEATER_PIN, HIGH);
  else digitalWrite(HEATER_PIN, LOW);


  if (state == WAITING_FOR_INPUT)
  {
    processSerialInput();

    if (nAutoFlames)
    {
      if (lighterTimer.elapsedMillis() > MIN_AUTO_DELAY_MS)
      {
        if (goalTemp-AUTO_TEMP_RANGE < avgTemp && avgTemp < goalTemp+AUTO_TEMP_RANGE)
        {
          nAutoFlames--;
          sendSingleFlame();
          Serial.print("O nAutoFlames: ");
          Serial.print(nAutoFlames);
          Serial.print(", avgTemp: ");
          Serial.println(avgTemp);
        }
      }
    }
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
        turnLighterOn();
        delayIndex++;
      }
    }
  }
}