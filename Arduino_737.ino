#include <LiquidCrystal_I2C.h>

#include <Wire.h>

LiquidCrystal_I2C lcd(0x27,16,2);

#define Select_GND 48
#define Select_OFF 47
#define Select_CONT 46
#define Select_FLT 45
#define POWER 50
#define IGN 37
#define VALVE 30
#define N1 4
#define N2 5
#define COMBUSTOR 7
#define AFTB 8

#include <Servo.h>
Servo Flaps;

int potservo = 0;
int val;
int Velocidad = 0;
int velocidadMedida;
int velocidadReal;
int velocidadDisplay =0;
int Intensidad = 0;

int estado;

const int INICIAL = 0;
const int OFF = 700;
const int GND = 100;
const int CONT = 800;
const int FLT = 200;
const int GND_O_CONT = 300;
const int OFF_O_IGNITION = 400;
const int OFF_O_FLT = 900;
const int CONT_O_IGNITION = 600;
const int IGNITION = 110;
const int IGNITION_O_ABORT = 120;
const int ESPERA = 121;
const int COMBUSTION = 122;
const int VELOCIDAD_MINIMA = 123;
const int OPERACION = 124;


// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  lcd.backlight();
  lcd.init();    
  lcd.setCursor(2,0);
  Serial.println("hola");
  lcd.print("hola");
  delay(1000);


  pinMode(Select_FLT, INPUT_PULLUP);
  pinMode(Select_CONT, INPUT_PULLUP);
  pinMode(Select_OFF, INPUT_PULLUP);
  pinMode(Select_GND, INPUT_PULLUP);
  pinMode(POWER, INPUT_PULLUP);
  pinMode(IGN, INPUT_PULLUP);
  pinMode(VALVE, INPUT_PULLUP);
  pinMode(N1, OUTPUT);
  pinMode(COMBUSTOR, OUTPUT);
  pinMode(N2, OUTPUT);
  pinMode(AFTB, OUTPUT);

  analogWrite(N1, 0); // apaga Motor N1
  analogWrite(COMBUSTOR, 0); // apaga LEDs
  analogWrite(N2, 0); // apaga Motor N2
  analogWrite(AFTB, 0); // apaga Afterburner

  Flaps.attach(11);

}

void loop() {
  val = analogRead(potservo);            // reads the value of the potentiometer (value between 0 and 1023)
  val = map(val, 0, 1023, 0, 180);     // scale it to use it with the servo (value between 0 and 180)
  Flaps.write(val);                  // sets the servo position according to the scaled value
  delay(15);

  switch (estado) {
    
    case INICIAL: //si cualquiera de las señales de la selectora estan apagadas, pasa al estado de espera al OFF
          
          if (digitalRead (Select_GND) == HIGH || digitalRead (Select_FLT) == HIGH || digitalRead (Select_OFF) == HIGH || digitalRead (Select_CONT) == HIGH) {
          estado = OFF;
          }
          break;

    case OFF:
    
          analogWrite(N2,0);
          analogWrite(N1,0);
          analogWrite(AFTB,0);
          if (digitalRead (Select_FLT) == HIGH && digitalRead (Select_GND) == HIGH && digitalRead (Select_OFF) == LOW && digitalRead (Select_CONT) == HIGH) { // si la selectora esta en OFF
          Serial.println("OFF");
          lcd.clear();
          lcd.print("OFF");
          estado = GND_O_CONT;
          }
          break;

    case GND_O_CONT: // Evalua el cambio en la selectora desde OFF
    
          analogWrite(N2, 0); 
          if (digitalRead (Select_GND) == LOW && digitalRead (Select_FLT) == HIGH && digitalRead (Select_OFF) == HIGH && digitalRead (Select_CONT) == HIGH) {
          estado = GND; // si la selectora esta en GND, estado GND
          }
          if (digitalRead (Select_FLT) == HIGH && digitalRead (Select_GND) == HIGH && digitalRead (Select_OFF) == HIGH && digitalRead (Select_CONT) == LOW) {
          estado = CONT; // si la selectora esta en CONT, estado CONT
          }
          break;
      
    case GND:
         
         Serial.println("GND");
         lcd.clear();
         lcd.print("GND");
         if (digitalRead (Select_GND) == LOW && digitalRead (Select_FLT) == HIGH && digitalRead (Select_OFF) == HIGH && digitalRead (Select_CONT) == HIGH) {
         estado = OFF_O_IGNITION;
         }
         break;

    case OFF_O_IGNITION: // Evalua el cambio en la selectora desde GND

          if (digitalRead (Select_FLT) == HIGH && digitalRead (Select_GND) == HIGH && digitalRead (Select_OFF) == LOW && digitalRead (Select_CONT) == HIGH) {
          estado = OFF; // si la selectora esta en OFF, estado OFF
          }
          if (digitalRead (IGN) == LOW) {
          estado = IGNITION; // si la selectora sigue en GND, y el Switch de ignición se prende, estado IGNITION
          }
          break;
            
    case IGNITION:
          
          Serial.println ("IGNITION");
          lcd.setCursor(5,0);
          lcd.print("IGNITION");
          if (digitalRead (IGN) == LOW) {
          estado = IGNITION_O_ABORT; // si el Switch sigue encendido, estado IGNITION_O_ABORT
          }
          break;

    case IGNITION_O_ABORT:
          
          if (digitalRead (IGN) == LOW) {
          for (int i = 0; i <= 129; i++) {  // si el Switch sigue encendido, arranca a girar el motor N2 hasta el 50%
          if (digitalRead (IGN) == HIGH){ //ABORT IGNITION, el estado GND_O_CONT apaga el motor N2 y espera a que la selectora vuelva a OFF y de ahi a GND
          estado = GND_O_CONT;
          break;
          }
          Velocidad = i;
          Velocidad = map(Velocidad, 0, 255, 0, 100);
          analogWrite(N2, Velocidad);
          delay(80);
          Serial.println(Velocidad);
          lcd.setCursor(10,1);
          lcd.print(Velocidad);
          lcd.setCursor(13,1);
          lcd.print("%");
          estado = ESPERA;
          }
          }
          break;

    case ESPERA:
          analogWrite(N2, 128);
          analogWrite(N1, 0);
          Serial.println(Velocidad);
          analogWrite(COMBUSTOR,0);
          digitalWrite(AFTB,0);
          if (digitalRead (IGN) == HIGH){ //si se corta la ignicion, el estado 300 apaga el motor N2 y espera a que la selectora marque GND
          estado = GND_O_CONT;
          }
          if (digitalRead (VALVE) == LOW){ //si se activa la valvula de combustible, estado COMBUSTION
          estado = COMBUSTION;
          }
          break;
    
    case COMBUSTION:
          
          lcd.setCursor(0,1);
          lcd.print("VALVE ON");
          analogWrite(COMBUSTOR,255);
          delay (300);
          for (int j = 255; j>=0; j--) { // los LEDs se encienden de repente y se apagan gradualmente
          Intensidad = j;
          analogWrite(COMBUSTOR, Intensidad);
          delay(1);
          Serial.println(Intensidad);
          }

          for (int i = 129; i <= 250; i++) { // Los dos motores aceleran gradualmente hasta 100%
          Velocidad = i;
          Velocidad = map(Velocidad, 0, 255, 0, 100);
          analogWrite(N2, Velocidad);
          analogWrite(N1, Velocidad);
          delay(20);
          Serial.println(Velocidad);
          lcd.setCursor(10,1);
          lcd.print(Velocidad);
          lcd.setCursor(13,1);
          lcd.print("%");
          }

          for (int i = 250; i >= 51; i--) { // Los dos motores deceleran gradualmente
          Velocidad = i;
          Velocidad = map(Velocidad, 0, 255, 0, 100);
          analogWrite(N2, Velocidad);
          analogWrite(N1, Velocidad);
          delay(20);
          Serial.println(Velocidad);
          lcd.setCursor(10,1);
          lcd.print(Velocidad);
          lcd.setCursor(13,1);
          lcd.print("%");
 
          if (digitalRead (VALVE) == HIGH){ // Si se corta el combustible, estado ESPERA
          estado = ESPERA;
          break;
          }
          
          estado = VELOCIDAD_MINIMA; // Si no pasa nada, estado VELOCIDAD_MINIMA
        
          }
          break;


    case VELOCIDAD_MINIMA:

          analogWrite(N2,10); //N2 al 20%
          delay(1500);
          lcd.clear();
          lcd.print("N2:");
          lcd.setCursor(4,0);
          lcd.print(velocidadDisplay);
          lcd.setCursor(7,0);
          lcd.print("%");
          lcd.setCursor(0,1);
          lcd.print("N1:");
          lcd.setCursor(4,1);
          lcd.print(velocidadDisplay);
          lcd.setCursor(7,1);
          lcd.print("%");

          estado = OPERACION;
          break;

    case OPERACION:

          velocidadMedida=analogRead(A7);
          velocidadReal = map (velocidadMedida, 0, 1023, 12, 255);
          velocidadDisplay = map (velocidadMedida, 0, 1023, 20, 100);
          analogWrite(N2, velocidadReal);
          analogWrite(N1, velocidadReal);
          delay(100);
          lcd.clear();
          lcd.print("N2:");
          lcd.setCursor(4,0);
          lcd.print(velocidadDisplay);
          lcd.setCursor(7,0);
          lcd.print("%");
          lcd.setCursor(0,1);
          lcd.print("N1:");
          lcd.setCursor(4,1);
          lcd.print(velocidadDisplay);
          lcd.setCursor(7,1);
          lcd.print("%");
          delay(100);
          if (velocidadDisplay >= 90){
            digitalWrite (AFTB,HIGH);
          }
          else{
            digitalWrite (AFTB,LOW);
          }
         
          if (digitalRead (VALVE) == LOW){
          estado = OPERACION;
          }
          if (digitalRead (VALVE) == HIGH){
          estado = ESPERA;
          }
          break;

    case CONT:

          Serial.println("CONT");
          lcd.clear();
          lcd.print("CONT");
          if (digitalRead (Select_FLT) == HIGH && digitalRead (Select_GND) == HIGH && digitalRead (Select_OFF) == HIGH && digitalRead (Select_CONT) == LOW) {
          estado = OFF_O_FLT;
          }
          break;

    case OFF_O_FLT:
    
          if (digitalRead (Select_FLT) == LOW && digitalRead (Select_GND) == HIGH && digitalRead (Select_OFF) == HIGH && digitalRead (Select_CONT) == HIGH) {
          estado = FLT;
          }
          if (digitalRead (Select_FLT) == HIGH && digitalRead (Select_GND) == HIGH && digitalRead (Select_OFF) == LOW && digitalRead (Select_CONT) == HIGH) {
          estado = OFF;
          }
          break;
    
    case FLT:

          Serial.println("FLT");
          lcd.clear();
          lcd.print("FLT");
          if (digitalRead (Select_FLT) == LOW && digitalRead (Select_GND) == HIGH && digitalRead (Select_OFF) == HIGH && digitalRead (Select_CONT) == HIGH) {
          estado = CONT_O_IGNITION; //si se queda en FLT pasa al estado CONT_O_IGNITION
          }
          break;

    
    case CONT_O_IGNITION: //evalua si la selectora pasa al estado CONT
          if (digitalRead (IGN) == LOW) {
          estado = IGNITION;
          }
          if (digitalRead (Select_FLT) == HIGH && digitalRead (Select_GND) == HIGH && digitalRead (Select_OFF) == HIGH && digitalRead (Select_CONT) == LOW) {
          estado = CONT; 
          }
          break;

  

    default:

    break;
 
  }
}


