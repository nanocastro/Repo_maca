/*
MACA conformado por un Arduino nano, un sensor de humedad y temperatura DHT11 y sensor de particulado Plantower PMS7003.
Ademas reliza la medicion análogica de 5 sensores de gases y guarda directamente estos valores digitales para un post-procesamiento de los mismos. 
Registro de datos cada un minuto y almacenamiento en SD con fecha y hora provista por un RTC.
*/

#include <PMS.h>
#include <TimerOne.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <dht.h>
#include <SoftwareSerial.h>

#define an_aux 5.0/1024

int timer = 0, timer2 = 0;                                                      //Variables auxiliares para contar segundos y ciclos
bool error = false, blue = false, dia = false, calentar = true;                 //Banderas de error bluetooth y cambio de archivo
bool  flag_lectura = false, flag_grabar = false, flag_t = false;

int state = 0;
int ciclo = 0;

dht DHT;          //Sensor de humedad y termperatura
RTC_DS3231 rtc;   //Reloj RTC

String com_term;            //Cadena de caracteres recibida de terminal
int j = 0, aux = 0, r = 2;
char com_term_char[16];


String filename;
String dataString;

File dataFile;
DateTime hora, anterior;
SoftwareSerial serialPc(2, 3);  //Emular puerto serie

PMS pms(serialPc);      //Plantower sensor particulado
PMS::DATA data;

const int chipSelect = 10;         //53                       //SS de SPI
int chk;

void setup() {

  Serial.begin(9600);
  serialPc.begin(9600);
  //Interrupciones de encendido de bluetooth y de LPO del shinyei
  pinMode(8, INPUT_PULLUP);       //P25
  pinMode(9, INPUT_PULLUP);       //P10


  if (! rtc.begin()) {              //Inicio del reloj rtc
    serialPc.println("RTC ERROR");
    error = true;
  }
  //rtc.adjust(DateTime(2018, 1, 02,11, 18, 10));    //Para ajustar la fecha en caso de que se haya quedado sin bateria

  if (!SD.begin(chipSelect)) {      //Inicio de tarjeta SD
    Serial.println("Card failed, or not present");
    error = true;
  } else {
    Serial.println("SD card OK");
  }

  DateTime now_ = rtc.now();

  filename = String(now_.day());
  filename += "_";
  filename += String(now_.month());
  filename += "_";
  filename += String(now_.year() - 2000);
  filename += ".txt";

  char fileNameCharArray[filename.length() + 1];
  filename.toCharArray(fileNameCharArray, filename.length() + 1);

  File archivo = SD.open(fileNameCharArray, FILE_WRITE);
  if (archivo) {
    archivo.println(filename);
    archivo.close();
    Serial.println("SD data");
  }

  pms.passiveMode();
  pms.wakeUp();
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(contador);

  hora = rtc.now();
  anterior = hora.day();

}

void loop() {

  if (flag_lectura == true) {
    leer();                   //Lectura de entradas analogicas
    flag_lectura = false;
  }
  if (flag_grabar == true) {
    ciclo_lectura();          //Carga de datos a SD
    flag_grabar = false;
  }

  if (Serial.available()) {
    comandos_terminal(filename);
  }
}

void leer() {

  //Lectura de Humedad,temperatura y entradas analogicas conectadas a los sensores

  int v0 = analogRead(A0);    
  int v1 = analogRead(A1);
  int v2 = analogRead(A2);
  int v3 = analogRead(A3);
  int v4 = analogRead(A6);
  int v5 = analogRead(A7);

  chk = DHT.read11(9);
  anterior = hora;
  hora = rtc.now();

  dataString = String(hora.day()) + "/" + String(hora.month()) + "/" + String(hora.year() - 2000) + "-";
  dataString += String(hora.hour());
  dataString += ":" + String(hora.minute()) + ":" + String(hora.second()) + ",";
  dataString += String(v0) + "," + String(v1) + ","  + String(v2) + ",";
  dataString += String(v3) + "," + String(v4) + ","  + String(v5) + ",";
  dataString += String(DHT.humidity) + "," + String(DHT.temperature) + ",";
  dataString += String(data.PM_AE_UG_1_0) + "," + String(data.PM_AE_UG_2_5) + "," + String(data.PM_AE_UG_10_0);
}

void contador() {

  if (timer < 60) {
    timer++;
    if (timer == 1) {
      pms.requestRead();
    }
    if (timer == 57) {
      flag_lectura = true;
    }
  } else if (timer == 60) {
    timer = 0;
    flag_grabar = true;
  }
}

//Funcion para enviar el contenido completo del TXT del dia actual
void send_txt(String file, bool serial) {
  char arreglo[file.length() + 1];
  file.toCharArray(arreglo, file.length() + 1);

  File archivo = SD.open(arreglo);

  if (serial == true) {
    if (archivo) {
      Serial.println(file);
      while (archivo.available()) {     //envia todos los caracteres contenidos en el txt
        Serial.write(archivo.read());
      }
      archivo.close();
      Serial.println(millis());         //Tiempo que tardo en enviar el archivo caracter por caracter
    }
    else {
      Serial.println("error opening TXT");
    }
  }

}

void ciclo_lectura() {

  //Si Se ha cumplido un ciclo, abre el TXT correspondiente a la fecha y escribe los datos
  char fileNameCharArray[filename.length() + 1];
  if (anterior.day() != hora.day()) {
    dia = true;
    filename = String(hora.day());
    filename += "_";
    filename += String(hora.month());
    filename += "_";
    filename += String(hora.year() - 2000);
    filename += ".txt";
  }

  filename.toCharArray(fileNameCharArray, filename.length() + 1);
  dataFile = SD.open(fileNameCharArray, FILE_WRITE);
  Serial.println(dataFile);
  if (dataFile) {
    if (dia == true) {
      String encabezado = "Hora,O3,NO2,CO,Humedad,Temperatura,P25,P10";
      dataFile.println(encabezado);
      dia = false;
    }
    dataFile.println(dataString);
    dataFile.close();
    Serial.println("Datos SD");
    Serial.println(dataString);
  }

}

void comandos_terminal(String nombre) {

  char sel_t, arreglo[nombre.length() + 1];
  nombre.toCharArray(arreglo, nombre.length() + 1);

  while (Serial.available()) {
    sel_t =  Serial.read();   //Se queda esperando un caracter enviado
    com_term.concat(sel_t);
    com_term_char[j] = sel_t;
    j++;

    if (sel_t == '*') {
      j = 0;
      aux = 0;
      r = 2;
      com_term = "";
      int i = 0;
      for (i = 0; i < 16; i++) {
        com_term_char[i] = ' ';
      }
    }

    if (sel_t == '#') {
      int i = 0;
      for (i = 0; i < 16; i++) {
        Serial.print(com_term_char[i]);
      }
      flag_t = 0;
    }
  }

  if (flag_t == 0) {
    if (com_term_char[0] == 'p') {
      Serial.println(com_term_char[1]);
      for (r = 2; r < 16; r++) {
        if (com_term_char[r] == '#') {
          break;
        }
        aux = aux * 10 + (com_term_char[r] - '0');
        Serial.println(aux);
        Serial.println(r);
        r++;
      }

    }
    if (com_term == "a#") {
      Serial.println(nombre);
      send_txt(nombre, true);
    } else if (com_term == "b#") {
      Serial.println("Eco");
    } else if (com_term == "i#") {
      leer();
      Serial.println(dataString);
    }
    //     else if (com_term == "v#") {
    //      Serial.println(v_0);
    //    {
    else if (com_term == "t#") {
      Serial.println(timer);
    } else if (com_term == "ff#") {
      Serial.println(filename);
    }
    flag_t = 1;
  }
}



