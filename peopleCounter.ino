#include <Wire.h>
#include <LiquidCrystal_I2C.h>

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
void mostrarAforoLleno();
void enviarTotalEntradas();
void procesarComando(String comando);

// Variables globales adicionales
int limiteAforo = 100;       // Límite de aforo predeterminado
bool aforoLleno = false;     // Estado de aforo lleno

void setup() {
  // Inicializar monitor serie
  Serial.begin(9600);
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

  // Leer comandos desde Serial
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    procesarComando(comando);
  }

  // Máquina de estados
  switch (estadoActual) {
    case IDLE:
      if (distance1 < 50 && distance1 < initialDistance1 - 30) {
        estadoActual = SENSOR1_ACTIVATED;
        tiempoEstado = millis(); // Guardar tiempo de activación
      } else if (distance2 < 50 && distance2 < initialDistance2 - 30) {
        estadoActual = SENSOR2_ACTIVATED;
        tiempoEstado = millis(); // Guardar tiempo de activación
      }
      break;

    case SENSOR2_ACTIVATED:
      if (distance1 < 50 && distance1 < initialDistance1 - 30) {
        if (!aforoLleno) {
          personas++;
          Serial.print("Entrada. Personas: ");
          Serial.print(personas);
          Serial.println(";");
          encenderLedPorTiempo(pinGreen);
          sonarBuzzerEntrada();
          if (personas >= limiteAforo) {
            aforoLleno = true;
            mostrarAforoLleno();
          } else {
            actualizarPantalla();
          }
        } else {
          mostrarAforoLleno();
          Serial.println("Intento de entrada con aforo lleno.");
        }
        estadoActual = IDLE;
        delay(200);
      } else if (millis() - tiempoEstado > TIEMPO_LIMITE) {
        estadoActual = IDLE;
      }
      break;

    case SENSOR1_ACTIVATED:
      if (distance2 < 50 && distance2 < initialDistance2 - 30) {
        if (personas > 0) {
          personas--;
          Serial.print("Salida. Personas: ");
          Serial.print(personas);
          Serial.println(";");
          encenderLedPorTiempo(pinRed);
          sonarBuzzerSalida();
          if (personas < limiteAforo) {
            aforoLleno = false;
            actualizarPantalla();
          }
        } else {
          Serial.println("No hay personas para registrar salida.");
        }
        estadoActual = IDLE;
        delay(200);
      } else if (millis() - tiempoEstado > TIEMPO_LIMITE) {
        estadoActual = IDLE;
      }
      break;
  }
}

void procesarComando(String comando) {
  if (comando.startsWith("SET_AFORO")) {
    int nuevoLimite = comando.substring(9).toInt();
    if (nuevoLimite > 0 && nuevoLimite > personas) {
      limiteAforo = nuevoLimite;
      Serial.print("Nuevo límite de aforo establecido: ");
      Serial.println(limiteAforo);
    } else if (nuevoLimite <= personas) {
      Serial.println("Error: El límite de aforo debe ser mayor al número actual de personas.");
    } else {
      Serial.println("Error: Límite de aforo no válido.");
    }
  } else if (comando == "RESET") {
    enviarTotalEntradas();
  }
}

void mostrarAforoLleno() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Aforo lleno");
  lcd.setCursor(0, 1);
  lcd.print("Personas: ");
  lcd.print(personas);
}

void enviarTotalEntradas() {
    personas = 0;
    aforoLleno = false;
    actualizarPantalla();
    Serial.print("Entrada. Personas: ");
    Serial.print(personas);
    Serial.println(";");
}


int medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duracion = pulseIn(echoPin, HIGH);
  return duracion * 0.034 / 2;
}

void apagarLed() {
  digitalWrite(pinRed, LOW);
  digitalWrite(pinGreen, LOW);
  digitalWrite(pinBlue, LOW);
}

void encenderLedPorTiempo(int pinLed) {
  digitalWrite(pinLed, HIGH);
  delay(300);
  digitalWrite(pinLed, LOW);
}

void sonarBuzzerEntrada() {
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
}

void sonarBuzzerSalida() {
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
}

void actualizarPantalla() {
  if (!aforoLleno) {
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("Aforo");
    lcd.setCursor(8, 1);
    lcd.print(personas);
  }
}
