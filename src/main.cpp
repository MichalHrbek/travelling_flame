#include <Arduino.h> // Tohle je mozna potreba odstranit pri spousteni v Arduino IDE

// Dratky:
//  zelena - zem
//  zluta - input do arduina
//  oranzova - output z arduina

#define FLAME_SENSOR_PIN 7

#define LIGHTER_PIN 6
#define LIGHTER_MILIS 10

#define N_THERMISTORS 1
const int thermistorPins[N_THERMISTORS] = {A1};

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
  Serial.begin(9600);
}

double readCelsiusTemp(int pin)
{
  int thermistor_adc_val = analogRead(pin);
  double output_voltage = ( (thermistor_adc_val * 5.0) / 1023.0 ); // Toto predpoklada ADC rozliseni 1024, coz je na Ardiuno UNO myslim vzdycky
  double thermistor_resistance = ( ( 5.0 * ( 10.0 / output_voltage ) ) - 10.0 ); // Resistance in kilo ohms
  thermistor_resistance = thermistor_resistance * 1000 ; // Resistance in ohms
}

double resistanceToCelsius(double r)
{
  // Hodnoty z ts datasheetu https://img.gme.cz/files/eshop_data/eshop_data/2/118-042/dsh.118-042.1.pdf
  const double A_1 = 3.354016E-03;
  const double B_1 = 3.495020E-04;
  const double C_1 = 2.095959E-06;
  const double D_1 = 4.260615E-07;
  const double R_ref = 10000;
  
  double LN = log(r/R_ref);

  double T_k = 1.0/(A_1 + B_1*LN + C_1*LN*LN + D_1*LN*LN*LN);

  return T_k-273.15;
}

void printTime()
{
  Serial.print(millis());
}

void printThermistors()
{
  for (size_t i = 0; i < N_THERMISTORS; i++)
  {
    Serial.print("T");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(readCelsiusTemp(thermistorPins[i]));
  }
}

bool lighterOn = false;
unsigned long lighterStartMicros = 0;
bool sensorVal = true;

void loop() {
  // ---- Reading flame sensor ----
  bool newSensorVal = digitalRead(FLAME_SENSOR_PIN);
  if (newSensorVal != sensorVal)
  {
    sensorVal = newSensorVal;
    printTime();
    if (sensorVal) Serial.println(" - Flame sensor: ON");
    else Serial.println(" - Flame sensor: OFF");
  }
  

  // ---- Checking for serial input ----
  if (Serial.available() > 0)
  {
    int b = Serial.read();
    printTime();
    Serial.print(" - Recieved: ");
    Serial.println((char)b);

    if ((char)b == 't')
    {
      printThermistors();
    }

    // Starting the lighter
    if ((char)b == 'f')
    {
      Serial.println();
      printTime();
      Serial.println(" - Starting lighter");
      lighterOn = true;
      lighterStartMicros = micros();
      digitalWrite(LIGHTER_PIN, HIGH);
    }
  }

  // ---- Checking if the lighter timer finnished ----
  if (lighterOn)
  {
    unsigned long currentMicros = micros();
    if (lighterStartMicros+LIGHTER_MILIS*1000 < currentMicros)
    {
      lighterOn = false;
      digitalWrite(LIGHTER_PIN, LOW);
      printTime();
      Serial.print(" - Ending lighter (ran for ");
      Serial.print(currentMicros-lighterStartMicros);
      Serial.println("us)");
    }
  }
}