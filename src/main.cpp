#include <Arduino.h>
#include <Wire.h>

// zelena zem
// zluta - input do arduina
// oranzova - output z arduina

#define FLAME_SENSOR_PIN 7
#define LIGHTER_PIN 6
#define N_THERMISTORS 1
const int thermistorPins[1] = {A1};
#define LIGHTER_MILIS 10

void setup() {
  pinMode(FLAME_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LIGHTER_PIN, OUTPUT);
  Serial.begin(9600);
}

double readCelsiusTemp(int pin)
{
  int thermistor_adc_val;
  double output_voltage, thermistor_resistance, therm_res_ln, temperature; 
  thermistor_adc_val = analogRead(pin);
  output_voltage = ( (thermistor_adc_val * 5.0) / 1023.0 );
  thermistor_resistance = ( ( 5 * ( 10.0 / output_voltage ) ) - 10 ); /* Resistance in kilo ohms */
  thermistor_resistance = thermistor_resistance * 1000 ; /* Resistance in ohms   */
  therm_res_ln = log(thermistor_resistance);
  /*  Steinhart-Hart Thermistor Equation: */
  /*  Temperature in Kelvin = 1 / (A + B[ln(R)] + C[ln(R)]^3)   */
  /*  where A = 0.001129148, B = 0.000234125 and C = 8.76741*10^-8  */
  temperature = ( 1 / ( 0.001129148 + ( 0.000234125 * therm_res_ln ) + ( 0.0000000876741 * therm_res_ln * therm_res_ln * therm_res_ln ) ) ); /* Temperature in Kelvin */
  temperature = temperature - 273.15; /* Temperature in degree Celsius */
  return temperature;
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