#include <TimerOne.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <dht.h>
#include <SoftwareSerial.h>
#include <stdlib.h>

#define an_aux 5.0/1023

int v0 = 0, v1 = 0, v2 = 0, v3 = 0, v4 = 0, v5 = 0, v6 = 0, v7 = 0, v8 = 0, v9 = 0;               //Variables de medicion de tension del ADC
float v_0 = 0, v_1 = 0, v_2 = 0, v_3 = 0, v_4 = 0, v_5 = 0, v_6 = 0, v_7 = 0, v_8 = 0, v_9 = 0;   //Variables escaladas a tension de referencia, con decimales de precision
float pot1 = 0, pot2 = 0, pot3 = 0, R1 = 0, R2 = 0, R3 = 0;                                       //Valores de potencia calculados y resistencias medidas
int c1 = 0, c2 = 255, timer = 0, timer2 = 0;                                                      //Variables auxiliares para contar segundos y ciclos
bool error = false, blue = false, dia = false, calentar = true;                                   //Banderas de error bluetooth y cambio de archivo
bool  flag_lectura = false, flag_grabar = false;
int pow1 = 0, pow2 = 0, pow3 = 160;

int state = 0;
char sel;
int flag = 0, flag_8 = 1, flag_9 = 1, flag_19 = 1, flag_t=1;                                                //Banderas para "interrupciones"
int ciclo = 0;
bool flag_shinyei=1;
int total_10=0,total_25=0;

float mediciones[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float potencias[3] = {0, 0, 0};

unsigned long millis_end_10, millis_init_10, millis_t_10;                                         //Tiempo en milisegundos de LPO del shinyei
unsigned long millis_end_25, millis_init_25, millis_t_25;

dht DHT;
RTC_DS3231 rtc;

String com_blue, com_term;
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
  //Interrupciones de encendido de bluetooth y de LPO del shinyei
  pinMode(8, INPUT_PULLUP);       //P25
  pinMode(9, INPUT_PULLUP);       //P10
  pinMode(19, INPUT_PULLUP);      //Bluetooth

  pinMode(15, OUTPUT);          //LED bluetooth
  pinMode(6, OUTPUT);           //LED Rojo
  pinMode(7, OUTPUT);           //LED Verde

  pinMode(10, INPUT);           //Datos DHT
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(19), pin_blue, CHANGE);

  if (! rtc.begin()) {              //Inicio del reloj rtc
    Serial.println("RTC ERROR");
    error = true;
  }
  //rtc.adjust(DateTime(2017, 10, 29, 8, 40, 10));    //Para ajustar la fecha en caso de que se haya quedado sin bateria

  if (!SD.begin(chipSelect)) {      //Inicio de tarjeta SD
    Serial.println("Card failed, or not present");
    error = true;
  } else {
    Serial.println("SD card OK");
  }

  DateTime now_ = rtc.now();

  String encabezado = String(now_.day());          //nombre del archivo a utilizar
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
  if(flag_shinyei==0){    //Ciclo de lectura de sensor de particulas habilitado
    interrupciones();     //Polling de interrupcion de datos de LPO
  }
  
  if (flag_lectura == true) {
    leer();       //Lectura de entradas analogicas
    flag_lectura = false;
  }
  if (flag_grabar == true) {
    ciclo_lectura();
    flag_grabar = false;
  }

  //Si el bluetooth esta encendido verifica la recepcion de comandos
  if (blue == true) {
     if (Serial2.available()) {
        comandos_bluetooth(filename);
     }
  }
  if (Serial.available()) {
    comandos_terminal(filename);
  }
}

void leer() {

  //Lectura de Humedad,temperatura y entradas analogicas conectadas a los sensores
  
  chk = DHT.read22(10);
  v0 = analogRead(A0);
  v1 = analogRead(A1);
  v2 = analogRead(A2);
  v3 = analogRead(A3);
  v4 = analogRead(A4);
  v5 = analogRead(A5);
  v6 = analogRead(A6);
  v7 = analogRead(A7);
  v8 = analogRead(A8);
  v9 = analogRead(A9);

  v_0 = v0 * an_aux;    //Escalado de los valores medidos del DAC
  v_1 = v1 * an_aux;    //v_0 representa cuantos V tengo en la alimentacion para ajustar el rango de la Vref
  v_2 = v2 * an_aux;
  v_3 = v3 * an_aux;
  v_4 = v4 * an_aux;
  v_5 = v5 * an_aux;
  v_6 = v6 * an_aux;
  v_7 = v7 * an_aux;
  v_8 = v8 * an_aux;
  v_9 = v9 * an_aux;

  mediciones[0] = v_0;
  mediciones[1] = v_1;
  mediciones[2] = v_2;
  mediciones[3] = v_3;
  mediciones[4] = v_4;
  mediciones[5] = v_5;
  mediciones[6] = v_6;
  mediciones[7] = v_7;
  mediciones[8] = v_8;
  mediciones[9] = v_9;

  pot1 = (v_3 / 100.0) * v_1; //Potencias de calentamiento de los sensores
  pot2 = (v_6 / 100.0) * v_5;
  pot3 = (v_9 / 100.0) * v_7;

  potencias[0] = pot1;
  potencias[1] = pot2;
  potencias[2] = pot3;

  R1 = (22000.0 / (v_0 - v_2)) * v_2;      //Resistencias medidas de los sensores
  R2 = (2200.0 / (v_0 - v_4)) * v_4;       //Si se cambian los valores de resistencia de carga es necesario actualizar el '2200.0' por el valor correspondiente
  R3 = (200000.0 / (v_0 - v_8)) * v_8;     

  anterior = hora;
  hora = rtc.now();
  dataString = String(hora.hour());
  dataString += ":" + String(hora.minute()) + ":" + String(hora.second()) + ",";
  dataString += String(R1) + "," + String(R2) + "," + String(R3) + ",";
  dataString += String(v0) + "," +String(v1) + "," + String(v2) + ","; 
  dataString += String(v3) + "," +String(v4) + "," + String(v5) + ","; 
  dataString += String(v6) + "," + String(v7) + "," + String(v8) + "," + String(v9) + ",";
  dataString += String(DHT.humidity) + "," + String(DHT.temperature);
  dataString += "," + String(total_25) + "," + String(total_10);
}

void contador() {
  
  if (timer < 59) {
    timer++;
    if (timer == 30) {          
      flag_lectura = true;      
      calentar = !calentar;     
    }else if(timer>30){
      if(flag_shinyei==1){
        flag_shinyei=0;
        total_10=0;
        total_25=0;
        analogWrite(2, 255);
      }
    }else if(timer<30){
      //flag_lecutra=true;
      if(flag_shinyei==0){
        flag_shinyei=1;
      }
    }
  } else if (timer == 59) {
    timer = 0;
    analogWrite(2, 0);
    flag_grabar = true;
    calentar = !calentar;     
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
      Serial2.println(file);
      while (archivo.available()) {
        Serial2.write(archivo.read());
      }
      archivo.close();
      Serial2.println(millis());
    }
    else {
      Serial2.println("error opening TXT");
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
      String encabezado = "Hora,O3,NO2,CO,Humedad,Temperatura,P25,P10";
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
  
  while ( Serial2.available()) {
    sel2 =  Serial2.read();   //Se queda esperando un caracter enviado desde el celular
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
        Serial2.print(com_blue_char[i]);
      }
      flag = 0;
      break;
    }
  }
  if (flag == 0) {
    if (com_blue_char[0] == 'p') {
      Serial2.println(com_blue_char[1]);
      for(r=2;r<16;r++){
        if (com_blue_char[r] == '#') {
          break;
        }
        aux = aux * 10 + (com_blue_char[r] - '0');
        Serial2.println(aux);
        Serial2.println(r);
        r++;
      }
      if (com_blue_char[1] == '1') {
        pow1 = aux;
      }
      if (com_blue_char[1] == '2') {
        pow2 = aux;
      }
      if (com_blue_char[1] == '3') {
        pow3 = aux;
      }
    }
    if (com_blue == "a#") {
      Serial2.println(nombre);
      send_txt(nombre, false);
    } else if (com_blue == "b#") {
      Serial2.println("Eco");
    } else if (com_blue == "l#") {
      root = SD.open("/");
      printDirectory(root, 0);
      Serial2.println("Ok");
    } else if (com_blue == "i#") {
      leer();
      Serial2.println(dataString);
    } else if (com_blue == "v#") {
      Serial2.println(v_0);
    } else if (com_blue == "m#") {
      int y = 0;
      for (y = 0; y < 10; y++) {
        Serial2.println(mediciones[y], 5);
      }
    } else if (com_blue == "p#") {
      int y = 0;
      for (y = 0; y < 3; y++) {
        Serial2.println(potencias[y], 5);
      }
    } else if (com_blue == "t#") {
      Serial2.println(timer);
    } else if (com_blue == "ff#") {
      Serial2.println(filename);
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
//No son interrupciones propiamente dichas. Es un polling a 
//Rutinas de las 'interrupciones', ante flanco de bajada empieza a contar 
//y en flanco de subida para el contador, hace la resta y guarda el valor de milisengundos

void p9_on() {
  if (flag_9 == 1) {
    millis_init_10 = millis();
    flag_9 = 0;
  }
}

void p9_off() {
  if (flag_9 == 0) {
    millis_end_10 = millis();
    millis_t_10 = millis_end_10 - millis_init_10;
    total_10 += millis_t_10;
    flag_9 = 1;
  }
}
void p8_on() {
  if (flag_8 == 1) {
    millis_init_25 = millis();
    flag_8 = 0;
  }
}
void p8_off() {
  if (flag_8 == 0) {
    millis_end_25 = millis();
    millis_t_25 = millis_end_25 - millis_init_25;
    total_25 += millis_t_25;
    flag_8 = 1;
  }
}

//Encendido de bluetooth
void pin_blue() {
  if (digitalRead(19) == HIGH) {
    bluetooth_on();
  } else {
    bluetooth_off();
  }
}

void bluetooth_on() {
  Serial2.begin(9600);
  delay(100);
  Serial2.println("MACA");
  blue = true;
  Serial.println("Bluetooth ON");
  analogWrite(5, 255);
  analogWrite(15, 255);
}

void bluetooth_off() {
  
  Serial2.end();
  blue = false;
  Serial.println("Bluetooth OFF");
  analogWrite(5, 0);
  analogWrite(15, 0);
}

void interrupciones() {
  if (digitalRead(8) == HIGH) {
    p8_off();
  } else {
    p8_on();
  }
  if (digitalRead(9) == HIGH) {
    p9_off();
  } else {
    p9_on();
  }
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
      if (com_term_char[1] == '1') {
        pow1 = aux;
      }
      if (com_term_char[1] == '2') {
        pow2 = aux;
      }
      if (com_term_char[1] == '3') {
        pow3 = aux;
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
    } else if (com_term == "m#") {
      int y = 0;
      for (y = 0; y < 10; y++) {
        Serial.println(mediciones[y], 5);
      }
    } else if (com_term == "p#") {
      int y = 0;
      for (y = 0; y < 3; y++) {
        Serial.println(potencias[y], 5);
      }
    } else if (com_term == "t#") {
      Serial.println(timer);
    } else if (com_term == "ff#") {
      Serial.println(filename);
    }
    flag_t = 1;
  }
}


