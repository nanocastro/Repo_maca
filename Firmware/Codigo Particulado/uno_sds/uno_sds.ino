/*
Equipo MACA de particulado constituido por un arduino Uno, un sensor de humedad y temperatura DHT22 y un sensor de particulado NOVA SDS011.
Se registran datos cada un minuto y se almacenan en una tarjeta SD con fecha y hora provista por un RTC.

*/

#include <SDS011.h>
#include <TimerOne.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <dht.h>

#define an_aux 5.0/1024

int v0 = 0, v1 = 0, v2 = 0;//, v3 = 0, v4 = 0, v5 = 0, v6 = 0, v7 = 0, v8 = 0, v9 = 0;               //Variables de medicion de tension del ADC
float v_0 = 0, v_1 = 0, v_2 = 0;//, v_3 = 0, v_4 = 0, v_5 = 0, v_6 = 0, v_7 = 0, v_8 = 0, v_9 = 0;   //Variables escaladas a tension de referencia, con decimales de precision
//float pot1 = 0, pot2 = 0, pot3 = 0, R1 = 0, R2 = 0, R3 = 0,R4=0,R5=0,R6=0;                                       //Valores de potencia calculados y resistencias medidas
int c1 = 0, c2 = 255, timer = 0, timer2 = 0;                                                      //Variables auxiliares para contar segundos y ciclos
bool error = false, blue = false, dia = false, calentar = true;                                   //Banderas de error bluetooth y cambio de archivo
bool  flag_lectura = false, flag_grabar = false;
//int pow1 = 0, pow2 = 0, pow3 = 160;

float p10,p25;
int SDSerror;

int state = 0;
char sel;
int flag = 0, flag_8 = 1, flag_9 = 1, flag_19 = 1, flag_t=1;                                                //Banderas para "interrupciones"
int ciclo = 0;
bool flag_shinyei=1;
int total_10=0,total_25=0;

dht DHT;
RTC_DS3231 rtc;


String com_blue, com_term;
int j = 0, aux = 0, r = 2;
char com_term_char[16];


String filename;
String dataString;
File dataFile, root;
DateTime hora, anterior;
const int chipSelect = 10;         //53                       //SS de SPI
int chk;

SDS011 my_sds;


void setup() {
  my_sds.begin(2,3);
  Serial.begin(9600);
  //Interrupciones de encendido de bluetooth y de LPO del shinyei
  pinMode(8, INPUT_PULLUP);       //P25
  pinMode(9, INPUT_PULLUP);       //P10

  if (! rtc.begin()) {              //Inicio del reloj rtc
    Serial.println("RTC ERROR");
    //serialPc.println("RTC ERROR");
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
  filename += String(now_.month());
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
  

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(contador);

  hora = rtc.now();
  anterior = hora.day();
  Serial.println(filename);
}

void loop() {
  
  if (flag_lectura == true) {
    leer();       //Lectura de entradas analogicas
    flag_lectura = false;
  }
  if (flag_grabar == true) {
    ciclo_lectura();
    flag_grabar = false;
  }

  if (Serial.available()) {
    comandos_terminal(filename);
  }
}

void leer() {

  //Lectura de Humedad,temperatura
  
  chk = DHT.read22(9);
  
  SDSerror = my_sds.read(&p25,&p10);
  
  if (! SDSerror) {
    Serial.println("P2.5: "+String(p25));
    Serial.println("P10:  "+String(p10));
  }
  

  anterior = hora;
  hora = rtc.now();
  dataString = String(hora.day()) + "/" + String(hora.month()) + "/" + String(hora.year() - 2000) + "-";
  dataString += String(hora.hour());
  dataString += ":" + String(hora.minute()) + ":" + String(hora.second()) + ",";
  dataString += String(v0) + "," + String(v1)+ ","  + String(v2) + ",";
  dataString += String(DHT.humidity) + "," + String(DHT.temperature);
  dataString += "," + String(p25) + "," + String(p10);
}

void contador() {
  
  if (timer < 60) {
    timer++;
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
    }
    else {
      Serial.println("error opening TXT");
    }
  }
}

void ciclo_lectura() {

  //Si Se ha cumplido un ciclo, abre el TXT correspondiente a la fecha y escribe los datos
  char fncArray[filename.length() + 1];
  if (anterior.day() != hora.day()) {
    dia = true;
    filename = String(hora.day());
    //filename += "_";
    filename += String(hora.month());
    //filename += "_";
    filename += String(hora.year() - 2000);
    filename += ".txt";
  }
  
  filename.toCharArray(fncArray, filename.length() + 1);
  File dFile = SD.open(fncArray, FILE_WRITE);
  Serial.println(dFile);
   Serial.println("-");
   Serial.println(filename);
    Serial.println("..");
  if (dFile) {
    if (dia == true) {
      String encabezado = "Hora,O3,NO2,CO,Humedad,Temperatura,P25,P10";
      dFile.println(encabezado);
      dia = false;
    }
    dFile.println(dataString);
    dFile.close();
    Serial.println("Datos SD");
    Serial.println(dataString);
  }
}

void comandos_terminal(String nombre) {

  char sel_t, arreglo[nombre.length() + 1];
  nombre.toCharArray(arreglo, nombre.length() + 1);

 while(Serial.available()) {
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
       //serialPc.print(com_term_char[i]);
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
      Serial.println(dataString);
    } else if (com_term == "v#") {
      Serial.println(v_0);
    } else if (com_term == "p#") {
      int y = 0;
      for (y = 0; y < 3; y++) {
        Serial.println(p25);
        Serial.println(p10);
      }
    } else if (com_term == "t#") {
      Serial.println(timer);
    } else if (com_term == "ff#") {
      Serial.println(filename);
    }
    flag_t = 1;
  }
}


