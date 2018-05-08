/*
Este MACA posee un sensor de humedad y temperatura DHT022 y tres sensores de particulado: SHINYEI PPD42 (con ventilacion forzada), NOVA SDS011 y PLANTOWER PMS7003.
La medicion es permanente y los datos se cargan en una SD cada un minuto. 
Posee un reloj de tiempo real RTC para proveer la fecha y hora a la cual se registran las mediciones.
Posee ademas bluetooth para comuunicar con un celular y descargar los datos del dia sin desarmar el equipo. 
Sería bueno agregar un comando que permita seleccionar los archivos a descargar por bluetooth y que no tenga que ser diariamente.

*/

#include <SDS011-select-serial.h>
#include <PMS.h>
#include <TimerOne.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <dht.h>
#include <SoftwareSerial.h>
#include <stdlib.h>


int  timer = 0, timer2 = 0;                                                      //Variables auxiliares para contar segundos y ciclos
bool error = false, blue = false, dia = false, calentar = true;                  //Banderas de error bluetooth y cambio de archivo
bool  flag_lectura = false, flag_grabar = false;

float p10,p25;
int SDSerror;

int state = 0;
char sel;
int flag = 0, flag_blue=0,flag_t=0;                                                //Banderas para "interrupciones"
int ciclo = 0;
bool flag_shinyei=1;
unsigned long total_10_1=0,total_25_1=0,total_10_2=0,total_25_2=0;

unsigned long micros_end_10, micros_init_10, micros_t_10;                                         //Tiempo en microsegundos de LPO del shinyei
unsigned long micros_end_25, micros_init_25, micros_t_25;
unsigned long millis_end_10, millis_init_10, millis_t_10;                                         //Tiempo en milisegundos de LPO del shinyei
unsigned long millis_end_25, millis_init_25, millis_t_25;

dht DHT;                      //Sensor de humedad y temperatura
RTC_DS3231 rtc;               //Rtc clock
PMS pms(Serial2);             //Plantower sensor de particulado
PMS::DATA data;
SDS011 my_sds(Serial1);       //Nova sensor de particulado

String com_blue, com_term;    //Cadena de caracterese recibida de terminal o bluetooth
int j = 0, aux = 0, r = 2;
char com_term_char[16];
char com_blue_char[16];

String filename;
String dataString;
File dataFile, root;
DateTime hora, anterior;
const int chipSelect = 53;                                //SS de SPI
int chk;

void setup() {
  
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);

  //Interrupciones de encendido de bluetooth y de LPO del shinyei
  pinMode(4,INPUT_PULLUP);       //Bluetooth
  pinMode(2,INPUT_PULLUP);
  pinMode(3,INPUT_PULLUP);
  pinMode(5,INPUT);
  pinMode(6,OUTPUT);
  attachInterrupt(digitalPinToInterrupt(2), low_p10, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), low_p25, CHANGE);


  if (! rtc.begin()) {                                  //Inicio del reloj rtc
    Serial.println("RTC ERROR");
    error = true;
  }
  
  //rtc.adjust(DateTime(2018, 3, 20, 20, 32, 10));      //Para ajustar la fecha en caso de que se haya quedado sin bateria

  if (!SD.begin(chipSelect)) {                          //Inicio de tarjeta SD
    Serial.println("Card failed, or not present");
    error = true;
  } else {
    Serial.println("SD card OK");
  }

  DateTime now_ = rtc.now();

  String encabezado = String(now_.day());           //nombre del archivo a utilizar
  encabezado += "_";
  encabezado += String(now_.month());
  encabezado += "_";
  encabezado += String(now_.year() - 2000);
  encabezado += ".txt";

  char fileNameCharArray[encabezado.length() + 1];
  encabezado.toCharArray(fileNameCharArray, encabezado.length() + 1);

  filename = String(now_.day());
  filename += "_";
  filename += String(now_.month());
  filename += "_";
  filename += String(now_.year() - 2000);
  filename += ".txt";

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(contador);

  hora = rtc.now();
  anterior = hora.day();
}

void loop() {
  
  
  if(digitalRead(4)==HIGH && flag_blue==false){
    bluetooth_on();
  }else if(digitalRead(4)==LOW && flag_blue==true){
    bluetooth_off();
  }

  
  if (flag_lectura == true) {
    leer();                   //Lectura de entradas analogicas
    flag_lectura = false;
  }
  if (flag_grabar == true) {
    ciclo_lectura();          //Cargado de datos a SD
    flag_grabar = false;
  }

  
  if (flag_blue == true) {
     if (Serial3.available()) {
        comandos_bluetooth(filename);
     }
  }
  
  if (Serial.available()) {
    comandos_terminal(filename);
  }

}

void leer() {

  anterior = hora;
  hora = rtc.now();
  chk = DHT.read22(5);
  
  SDSerror = my_sds.read(&p25,&p10);
  if (! SDSerror) {
    Serial.println("P2.5: "+String(p25));
    Serial.println("P10:  "+String(p10));
  }
  if (pms.read(data, 2000)) {
    Serial.println("PMS");
    Serial.println(data.PM_AE_UG_1_0);
    Serial.println(data.PM_AE_UG_2_5);
    Serial.println(data.PM_AE_UG_10_0);
  } else {
    Serial.println("NoPMS");
  }
  dataString = String(hora.day()) + "/" + String(hora.month()) + "/" + String(hora.year() - 2000) + "-";
  dataString += String(hora.hour())+ ":" + String(hora.minute()) + ":" + String(hora.second()) + ",";
  dataString +=  String(total_25_1) + "," + String(total_10_1) + "," + String(total_25_2) + "," + String(total_10_2)+ ",";
  dataString +=  String(data.PM_AE_UG_1_0) + "," + String(data.PM_AE_UG_2_5) + "," + String(data.PM_AE_UG_10_0)+ ",";
  dataString +=  String(p25) + "," + String(p10)+ ",";
  dataString += String(DHT.humidity) + "," + String(DHT.temperature);
  
  total_25_1=0;
  total_10_1=0;
  total_25_2=0;
  total_10_2=0;
  
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
  } else if (serial == false) {
    if (archivo) {
      Serial3.println(file);
      while (archivo.available()) {
        Serial3.write(archivo.read());
      }
      archivo.close();
      Serial3.println(millis());
    }
    else {
      Serial3.println("error opening TXT");
    }
  }
}

void ciclo_lectura() {

  //Si Se ha cumplido un ciclo, abre el TXT correspondiente a la fecha y escribe los datos
  
  if (anterior.day() != hora.day()) {
    dia = true;
    filename = String(hora.day());
    filename += "_";
    filename += String(hora.month());
    filename += "_";
    filename += String(hora.year() - 2000);
    filename += ".txt";
  }
  char fileNameCharArray[filename.length() + 1];
  filename.toCharArray(fileNameCharArray, filename.length() + 1);
  dataFile = SD.open(fileNameCharArray, FILE_WRITE);

  if (dataFile) {
    if (dia == true) {
      String encabezado = "Fecha-Hora, Shinyei P25 - P10 (us) (ms), PTWR P1-P25-P10, SDS P25-P10";
      dataFile.println(encabezado);
      dia = false;
    }
    dataFile.println(dataString);
    dataFile.close();
  }

}


void comandos_bluetooth(String nombre) {
  /*
  El caracter '*' delimita el inicio de mensaje mientras que '#' delimita el fin de comando.
  por ejemplo para indicar que se quiere obtener una medicion por unica vez debe mandarse '*i#'.
  */
  char sel2, arreglo[nombre.length() + 1];
  nombre.toCharArray(arreglo, nombre.length() + 1);
  
  while ( Serial3.available()) {
    sel2 =  Serial3.read();   //Se queda esperando un caracter enviado desde el celular
    com_blue.concat(sel2);
    com_blue_char[j] = sel2;
    j++;

    if (sel2 == '*') {
      j = 0;
      aux = 0;
      r = 2;
      com_blue = "";              //Reset de string de comandos
      int i = 0;
      for (i = 0; i < 16; i++) {
        com_blue_char[i] = ' ';
      }
    }

    if (sel2 == '#') {
      int i = 0;
      for (i = 0; i < 16; i++) {
        Serial3.print(com_blue_char[i]);
      }
      flag = 0;
      break;
    }
  }
  if (flag == 0) {
    if (com_blue_char[0] == 'p') {
      Serial3.println(com_blue_char[1]);
      for(r=2;r<16;r++){
        if (com_blue_char[r] == '#') {
          break;
        }
        aux = aux * 10 + (com_blue_char[r] - '0');
        Serial3.println(aux);
        Serial3.println(r);
        r++;
      }

    }
    if (com_blue == "a#") {
      Serial3.println(nombre);
      send_txt(nombre, false);
    } else if (com_blue == "b#") {
      Serial3.println("Eco");
    } else if (com_blue == "l#") {
      root = SD.open("/");
      printDirectory(root, 0);
      Serial3.println("Ok");
    } else if (com_blue == "i#") {
      leer();
      Serial3.println(dataString);
    } else if (com_blue == "t#") {
      Serial3.println(timer);
    } else if (com_blue == "ff#") {
      Serial3.println(filename);
    }
    flag = 1;
  }
}

//Esta funcion esta para ver el contenido de ls SD, quizas despues no se use
void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // No hay mas archivos
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }

}
unsigned int c_aux=0;
void low_p10() {
  if(digitalRead(2)==LOW){
    c_aux++;
    micros_init_10 = micros();
    millis_init_10 = millis();
  }else{
    high_p10();
  }
}
void high_p10() {
    micros_end_10 = micros();
    millis_end_10 = millis();
    micros_t_10 = micros_end_10 - micros_init_10;
    millis_t_10 = millis_end_10 - millis_init_10;

    // total_10_1 += micros_end_10;
    // total_10_2 += millis_end_10;

    total_10_1 += micros_t_10;
    total_10_2 += millis_t_10;
}

void low_p25() {
  if(digitalRead(3)==LOW){
    millis_init_25 = millis();
    micros_init_25 = micros();
  }else{
    high_p25();
  }
}
void high_p25() {
    millis_end_25 = millis();
    millis_t_25 = millis_end_25 - millis_init_25;
    micros_end_25 = micros();
    micros_t_25 = micros_end_25 - micros_init_25;

   // total_25_1 += micros_end_25;
   // total_25_2 += millis_end_25;

    total_25_1 += micros_t_25;
    total_25_2 += millis_t_25;
}



void bluetooth_on() {
  Serial3.begin(9600);
  delay(100);
  Serial3.println("MACA");
  Serial.println("Bluetooth ON");
  analogWrite(6, 255);
  flag_blue=true;
}

void bluetooth_off() {
  
  Serial3.end();
  Serial.println("Bluetooth OFF");
  analogWrite(6, 0);
  flag_blue=false;
}



void comandos_terminal(String nombre) {

  char sel_t, arreglo[nombre.length() + 1];
  nombre.toCharArray(arreglo, nombre.length() + 1);

 while(Serial.available()) {
    sel_t =  Serial.read();   //Se queda esperando un caracter enviado desde el celular
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
     for(r=2;r<16;r++){
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
    } else if (com_term == "l#") {
      root = SD.open("/");
      printDirectory(root, 0);
      Serial.println("Ok");
    } else if (com_term == "i#") {
      leer();
      Serial.print(dataString);
//      Serial.print(" .. ");
//      Serial.println(c_aux);
      c_aux=0;
    }  else if (com_term == "t#") {
      Serial.println(timer);
    } else if (com_term == "ff#") {
      Serial.println(filename);
    }
    flag_t = 1;
  }
}


