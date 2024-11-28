#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Configuración para SoftwareSerial
SoftwareSerial Bluetooth(10, 11); // RX en 10, TX en 11

// Configuración de pines
const int trigPin = 8;  // TRIG del primer sensor
const int echoPin = 9;  // ECHO del primer sensor
const int trigPin2 = 5; // TRIG del segundo sensor
const int echoPin2 = 6; // ECHO del segundo sensor

// Pines del LED RGB
const int pinRed = 2;
const int pinGreen = 3;
const int pinBlue = 4;

// Variables globales de tiempo
unsigned long tiempoEstado = 0; // Almacena el tiempo en el que se activó el estado
const unsigned long TIEMPO_LIMITE = 2000; // Tiempo límite (en milisegundos) para completar la secuencia

// Pin del buzzer
const int buzzerPin = 11;

// Variables globales
long duration, duration2;
int distance1, distance2, initialDistance1, initialDistance2;
int personas = 0;

// Configuración del LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Dirección I2C 0x27, 16 columnas y 2 filas

// Estados de la máquina
enum Estado {
  IDLE,
  SENSOR1_ACTIVATED,
  SENSOR2_ACTIVATED
};

Estado estadoActual = IDLE;

// Prototipos de funciones
int medirDistancia(int trigPin, int echoPin);
void apagarLed();
void encenderLedPorTiempo(int pinLed);
void sonarBuzzerEntrada();
void sonarBuzzerSalida();
void actualizarPantalla();
void enviarPersonas();

void setup() {
  // Inicializar monitor serie
  Serial.begin(9600);
  Bluetooth.begin(9600); // Comunicación con el módulo Bluetooth
  Serial.println("Sistema iniciado...");
  apagarLed();

  // Configuración de pines ultrasónicos
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  // Configuración de pines de LEDs
  pinMode(pinRed, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinBlue, OUTPUT);

  // Configuración del buzzer
  pinMode(buzzerPin, OUTPUT);

  // Inicializar pantalla LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();

  // Calcular distancias iniciales
  initialDistance1 = medirDistancia(trigPin, echoPin);
  initialDistance2 = medirDistancia(trigPin2, echoPin2);
  Serial.print("Distancia inicial 1: ");
  Serial.println(initialDistance1);
  Serial.print("Distancia inicial 2: ");
  Serial.println(initialDistance2);
  digitalWrite(buzzerPin, LOW);
}

void loop() {
  // Medir distancias actuales
  distance1 = medirDistancia(trigPin, echoPin);
  distance2 = medirDistancia(trigPin2, echoPin2);

  // Máquina de estados
  switch (estadoActual) {
    case IDLE:
      // Si el sensor 1 detecta algo, activar estado de entrada
      if (distance1 < 50 && distance1 < initialDistance1 - 30) {
        estadoActual = SENSOR1_ACTIVATED;
        tiempoEstado = millis(); // Guardar tiempo de activación
        Serial.print(distance1);
        Serial.println("Sensor 1 activado, esperando sensor 2...");
      } else if (distance2 < 50 && distance2 < initialDistance2 - 30) {
        estadoActual = SENSOR2_ACTIVATED;
        tiempoEstado = millis(); // Guardar tiempo de activación
        Serial.print(distance2);
        Serial.println("Sensor 2 activado, esperando sensor 1...");
      }
      break;

    case SENSOR2_ACTIVATED:
      // Confirmar entrada si el sensor 2 detecta algo
      if (distance1 < 50 && distance1 < initialDistance1 - 30) {
        personas++;
        Serial.print("Entrada detectada. Total personas: ");
        Serial.println(personas);
        enviarPersonas();
        encenderLedPorTiempo(pinGreen); // Encender LED verde por 2 segundos
        sonarBuzzerEntrada();
        actualizarPantalla();
        estadoActual = IDLE; // Reiniciar a IDLE
        delay(200);
      } else if (millis() - tiempoEstado > TIEMPO_LIMITE) {
      Serial.println("Tiempo agotado para entrada, regresando a IDLE...");
      estadoActual = IDLE;
    }
      break;

    case SENSOR1_ACTIVATED:
      // Confirmar salida si el sensor 1 detecta algo
      if (distance2 < 50 && distance2 < initialDistance2 - 30) {
        if (personas > 0) {
          personas--;
          Serial.print("Salida detectada. Total personas: ");
          Serial.println(personas);
          enviarPersonas();
          encenderLedPorTiempo(pinRed); // Encender LED rojo por 2 segundos
          sonarBuzzerSalida();
          actualizarPantalla();
        } else {
          Serial.println("No hay personas para registrar salida.");
        }
        estadoActual = IDLE; // Reiniciar a IDLE
        delay(200);
      } else if (millis() - tiempoEstado > TIEMPO_LIMITE) {
      Serial.println("Tiempo agotado para entrada, regresando a IDLE...");
      estadoActual = IDLE;
    }
      break;
  }
}

// Función para medir distancia con un sensor ultrasónico
int medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duracion = pulseIn(echoPin, HIGH);
  return duracion * 0.034 / 2; // Calcular la distancia en cm
}

// Función para apagar los LEDs
void apagarLed() {
  digitalWrite(pinRed, LOW);
  digitalWrite(pinGreen, LOW);
  digitalWrite(pinBlue, LOW);
}

// Función para encender un LED por un tiempo específico
void encenderLedPorTiempo(int pinLed) {
  digitalWrite(pinLed, LOW); // Encender LED
  delay(200);                // Mantener encendido por 2 segundos
  digitalWrite(pinLed, HIGH);  // Apagar LED
}

// Función para sonar el buzzer en una entrada
void sonarBuzzerEntrada() {
  digitalWrite(buzzerPin, HIGH); // Encender el buzzer
  delay(200);                // Mantener encendido por 2 segundos
  digitalWrite(buzzerPin, LOW);  // Apagar el buzzer
}

// Función para sonar el buzzer en una salida
void sonarBuzzerSalida() {
  digitalWrite(buzzerPin, HIGH); // Encender el buzzer
  delay(200);                // Mantener encendido por 2 segundos
  digitalWrite(buzzerPin, LOW);  // Apagar el buzzer
}

// Función para actualizar la pantalla LCD
void actualizarPantalla() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Aforo");
  lcd.setCursor(8, 1);
  lcd.print(personas);
}

// Función para enviar datos al módulo Bluetooth
void enviarPersonas() {
  Bluetooth.print("Personas actuales: ");
  Bluetooth.println(personas);
}
