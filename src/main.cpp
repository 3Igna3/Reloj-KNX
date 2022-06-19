//Librerias
#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include "Reloj_Ign.hpp"

//Declarar la clase RTC
RTC_DS3231 rtc;

bool Puntos = true,Parpadeo = true; //Variables para el parpadeo de los 2 puntos
byte S = -1;   //Segundos para calculo de parpadeo

const byte Dias_Mes [12] = {31,28,31,30,31,30,31,31,30,31,30,31}; //Ultimos dias del mes para cambio de dia

// Modos
// 0 - Hora
// 1 - Fecha
// 2 - Temperatura
// 3 - Cambiar Hora
// 4 - Alarma
// 5 - KNX
//-------------------
// 100 - Seleccionar Hora alarma
// 101 - Seleccionar Fecha
// 102 - Seleccionar Temperatura
// 103 - Seleccionar Cambiar Hora
// 104 - Seleccionar Alarma
// 105 - Seleccionar KNX
// 106 - Cambiar brillo
short int Modo = 0;

byte Selec_Dia_Alarma = 0;  //Numerador del selector de los dias en las que se activa la alerma
unsigned long int old_millis = -1;  //Marca para temporizador asincrono
bool Blink = true;  //Variable de parpadeo
bool Shift = true;  //Cambio para otras opciones en el menu
bool Alarm_Act_1 = false, Alarm_Act_2 = false;  //Marcas de activacion de las 2 alarmas
bool Old_Lamp_1 = false, Old_Lamp_2 = false, Old_Persiana_Arriba = false; //Marcas anteriores para receptores KNX
byte Seleccion_Prog_KNX_1 = 0, Seleccion_Prog_KNX_2 = 0;  //Marcas para el tipo de programacion de las 2 alarmas en KNX
bool Old_Puntos = true; //Marca anterio para reducir la actualizacion de la pantalla en modo hora
bool Act_LED_Est = false; //Marca para desactivar el Led de estado trasero

byte Brillo = 5;  //Marca brillo pantallas

//Void llamadas alarmas
void Handler_Porgramacion_KNX (byte _Seleccion_Prog_KNX); 

void setup () {
  //Iniciar puerto serie
  Serial.begin (115200);
  Serial.println ("\nPuerto iniciado a 9600 baudios\n\n\tReloj V2.1\n\tDiseñado y programado por Ignacio Salinas Gigorro\n");

  //Declarar los botones como entradas
  pinMode (Boton1, INPUT);
  pinMode (Boton2, INPUT);
  pinMode (Boton3, INPUT);
  pinMode (Boton4, INPUT);
  pinMode (Boton5, INPUT);

  //Declarar el LED de estado como salida
  pinMode (LED_BUILTIN,OUTPUT);

  //Ajustar el brillo
  Init(Brillo);

  //Iniciar el RTC
  rtc.begin ();
  //rtc.adjust(DateTime(F("Jan 01 2022"), F("00:00:00")));

  //Leer los datos de la EEPROM para guardarlos en variables
  Alarma_1_Activada = Alarma_1_Activada_address;
  Alarma_2_Activada = Alarma_2_Activada_address;

  Alarma_Hora_1 = eeprom.read (Alarma_Hora_1_address);
  Alarma_Minutos_1 = eeprom.read (Alarma_Minutos_1_address);
  Alarma_Hora_2 = eeprom.read (Alarma_Hora_2_address);
  Alarma_Minutos_2 = eeprom.read (Alarma_Minutos_2_address);

  Leer_Dias_Alarma (1);
  Leer_Dias_Alarma (2);

  //Debug
  if (digitalRead (Boton1) &&!digitalRead (Boton2) &&digitalRead (Boton3) &&!digitalRead (Boton4) &&digitalRead (Boton5)) Modo = 9999;  //Debug
}

void loop () {
  switch (Modo) {
    case (0): {                                //Mostrar Hora
      DateTime now = rtc.now();
      if (Shift) {
        if (Old_Puntos != Puntos){
          Mostrar_Hora(now.hour(),now.minute(),Puntos,true,true);
          Old_Puntos = Puntos;
        }
      }
      else {
        switch (now.dayOfTheWeek ()) {        //Mostrar dia de la semana
          case (0): Mostrar_Text ("DOMI",false,false);  break;
          case (1): Mostrar_Text ("LUNE",false,false);  break;
          case (2): Mostrar_Text ("MART",false,false);  break;
          case (3): Mostrar_Text ("MIER",false,false);  break;
          case (4): Mostrar_Text ("JUEV",false,false);  break;
          case (5): Mostrar_Text ("VIER",false,false);  break;
          case (6): Mostrar_Text ("SABA",false,false);  break;
        }
      }
      if (Handler_Boton (Boton4, 4)) Shift = !Shift;  //Parpadeo de puntos
      if (S != now.second()){
        Puntos = !Puntos;
        S = now.second();
      }
      Modo = Standart_Menu (0,100,false,false);
    }
    break;
    case (1): {                                //Mostrar Fecha
      DateTime now = rtc.now();
      if (Shift) Mostrar_Fecha (now.day(),now.month(),true,true);
      else Mostrar_Year (now.year());
      if (Handler_Boton (Boton4, 4)) Shift = !Shift;
      Modo = Standart_Menu (1,101,false,false);
    }
    break;
    case (2): {                                //Mostrar Temperatura
      if (Shift) Mostrar_Temperatura (rtc.getTemperature(),true);
      else Mostrar_Temperatura (((rtc.getTemperature () * 9) / 5) + 32,false);
      if (Handler_Boton (Boton4, 4)) Shift = !Shift;
      Modo = Standart_Menu (2,102,false,false);
    }
    break;
    case (3): {                                //Cambiar Hora
      DateTime now = rtc.now();
      if (Blink) Mostrar_Hora (now.hour(),now.minute(),true,true,true);
      else Mostrar_Hora (99,now.minute(),true,false,true);
      if (Handler_Boton (Boton1,1)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.hour() == 0) rtc.adjust (DateTime(now.year(),now.month(),now.day(),23,now.minute(),now.second()));
        else rtc.adjust (DateTime(now.year(),now.month(),now.day(),now.hour() - 1,now.minute(),now.second()));
      }
      if (Handler_Boton (Boton2,2)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.hour() == 23) rtc.adjust (DateTime(now.year(),now.month(),now.day(),0,now.minute(),now.second()));
        else rtc.adjust (DateTime(now.year(),now.month(),now.day(),now.hour() + 1,now.minute(),now.second()));
      }
      if (Handler_Boton (Boton5,5)) Modo = 4;
    }
    break;
    case (4): {                                //Cambiar Minutos
      DateTime now = rtc.now();
      if (Blink) Mostrar_Hora (now.hour(),now.minute(),true,true,true);
      else Mostrar_Hora (now.hour (),99,true,true,false);
      if (Handler_Boton (Boton1,1)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.minute() == 0) rtc.adjust (DateTime(now.year(),now.month(),now.day(),now.hour (),59,now.second()));
        else rtc.adjust (DateTime(now.year(),now.month(),now.day(),now.hour(),now.minute() - 1,now.second()));
      }
      if (Handler_Boton (Boton2,2)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.minute() == 59) rtc.adjust (DateTime(now.year(),now.month(),now.day(),now.hour (),0,now.second()));
        else rtc.adjust (DateTime(now.year(),now.month(),now.day(),now.hour(),now.minute() + 1,now.second()));
      }
      if (Handler_Boton (Boton5,5)) Modo = 0;
    }
    break;
    case (5) : {                                //Cambiar Dia
      DateTime now = rtc.now();
      if (Blink) Mostrar_Fecha (now.day(),now.month(),true,true);
      else Mostrar_Fecha (00,now.month(),false,true);
      if (Handler_Boton (Boton1,1)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.day() == 1) rtc.adjust (DateTime(now.year(),now.month(),Dias_Mes [now.month() - 1],now.hour (),now.minute(),now.second()));
        else rtc.adjust (DateTime(now.year(),now.month(),now.day() - 1,now.hour(),now.minute(),now.second()));
      }
      if (Handler_Boton (Boton2,2)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.day() == Dias_Mes [now.month() - 1]) rtc.adjust (DateTime(now.year(),now.month(),1,now.hour (),now.minute(),now.second()));
        else rtc.adjust (DateTime(now.year(),now.month(),now.day() + 1,now.hour(),now.minute(),now.second()));
      }
      if (Handler_Boton (Boton5,5)) Modo = 6;

    }
    break;
    case (6) : {                                //Cambiar Mes
      DateTime now = rtc.now();
      if (Blink) Mostrar_Fecha (now.day(),now.month(),true,true);
      else Mostrar_Fecha (now.day(),00,true,false);
      if (Handler_Boton (Boton1,1)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.month() == 1) rtc.adjust (DateTime(now.year(),12,now.day(),now.hour (),now.minute(),now.second()));
        else rtc.adjust (DateTime(now.year(),now.month() - 1,now.day(),now.hour(),now.minute(),now.second()));
      }
      if (Handler_Boton (Boton2,2)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.month() == 12) rtc.adjust (DateTime(now.year(),1,now.day(),now.hour (),now.minute(),now.second()));
        else rtc.adjust (DateTime(now.year(),now.month() + 1,now.day(),now.hour(),now.minute(),now.second()));
      }
      if (Handler_Boton (Boton5,5))  Modo = 7;
    }
    break;
    case (7) : {                                //Cambiar año
      DateTime now = rtc.now();
      if (Blink) Mostrar_Year (now.year());
      else {
        Mostrar_Vacio (0);
        Mostrar_Vacio (1);
        Mostrar_Vacio (2);
        Mostrar_Vacio (3);
      }
      
      if (Handler_Boton (Boton1,1)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.year() != 1) rtc.adjust (DateTime(now.year() - 1,now.month(),now.day(),now.hour(),now.minute(),now.second()));
      }
      if (Handler_Boton (Boton2,2)){
        Blink = true;
        old_millis = millis () / 250;
        if (now.year() != 9999) rtc.adjust (DateTime(now.year() + 1,now.month(),now.day(),now.hour (),now.minute(),now.second()));
      }
      if (Handler_Boton (Boton5,5)) Modo = 1;
    }
    break;
    case (8): {                                 //Cambiar hora alarma 1
      if (Blink)  Mostrar_Hora (Alarma_Hora_1,Alarma_Minutos_1,true,true,true);
      else Mostrar_Hora (0,Alarma_Minutos_1,true,false,true);
      if (Handler_Boton (Boton1,1)){
        Blink = true;
        old_millis = millis () / 250;
        if (Alarma_Hora_1 == 0) Alarma_Hora_1 = 23;
        else Alarma_Hora_1 = Alarma_Hora_1 - 1;
      }
      if (Handler_Boton (Boton2,2)){
        Blink = true;
        old_millis = millis () / 250;
        if (Alarma_Hora_1 == 23)  Alarma_Hora_1 = 0;
        else Alarma_Hora_1 = Alarma_Hora_1 + 1;
      }
      if (Handler_Boton (Boton5,5)) {
        eeprom.write (Alarma_Hora_1_address,Alarma_Hora_1);
        Modo = 9;
      }
    }
    break;
    case (9): {                                 //Cambiar minutos alarma 1
      if (Blink) Mostrar_Hora (Alarma_Hora_1,Alarma_Minutos_1,true,true,true);
      else Mostrar_Hora (Alarma_Hora_1,0,true,true,false);
      if (Handler_Boton (Boton1,1)) {
        Blink = true;
        old_millis = millis () / 250;
        if (Alarma_Minutos_1 == 0) Alarma_Minutos_1 = 59;
        else Alarma_Minutos_1 = Alarma_Minutos_1 - 1;
      }
      if (Handler_Boton (Boton2,2)) {
        Blink = true;
        old_millis = millis () / 250;
        if (Alarma_Minutos_1 == 59) Alarma_Minutos_1 = 0;
        else Alarma_Minutos_1 = Alarma_Minutos_1 + 1;
      }
      if (Handler_Boton (Boton5,5)) {
        eeprom.write (Alarma_Minutos_1_address,Alarma_Minutos_1);
        Modo = 10;
      }
    }
    break;
    case (10): {                                //Cambiar dia semana alarma 1
      Handler_Selc_Dias_Alarma (Selec_Dia_Alarma,Blink,1);
      if (Handler_Boton(Boton1,1) && Selec_Dia_Alarma != 0) Selec_Dia_Alarma = Selec_Dia_Alarma - 1;
      if (Handler_Boton(Boton2,2) && Selec_Dia_Alarma != 6) Selec_Dia_Alarma = Selec_Dia_Alarma + 1;
      if (Handler_Boton(Boton4,4)) {
        if (Selec_Dia_Alarma == 6){
          Dias_Alarma_1 [0] = !Dias_Alarma_1 [0];
        }
        if (Selec_Dia_Alarma <= 6){
          Dias_Alarma_1 [Selec_Dia_Alarma + 1] = !Dias_Alarma_1 [Selec_Dia_Alarma + 1];
        }
      }
      if (Handler_Boton(Boton5,5)) Modo = 11;
    }
    break;
    case (11): {                                //Activar / desactivar alarma 1
      if (Alarma_1_Activada == 1){
        Mostrar_Datos_Char ('O',0,0);
        Mostrar_Datos_Char ('N',1,0);
        Mostrar_Vacio (2);
        Mostrar_Vacio (3);
      }
      else {
        Mostrar_Datos_Char ('O',0,0);
        Mostrar_Datos_Char ('F',1,0);
        Mostrar_Datos_Char ('F',2,0);
        Mostrar_Vacio (3);
      }

      if (Handler_Boton (Boton4,4)){
        if (Alarma_1_Activada == 0) Alarma_1_Activada = 1;
        else Alarma_1_Activada = 0;
      }
      if (Handler_Boton (Boton5,5)) {
        Guardar_dias_alarma (1);
        Modo = 105;
      }
    }
    break;case (12): {                                 //Cambiar hora alarma 2
      if (Blink) Mostrar_Hora (Alarma_Hora_2,Alarma_Minutos_2,true,true,true);
      else Mostrar_Hora (0,Alarma_Minutos_2,true,false,true);
      if (Handler_Boton (Boton1,1)){
        Blink = true;
        old_millis = millis () / 250;
        if (Alarma_Hora_2 == 0) Alarma_Hora_2 = 23;
        else Alarma_Hora_2 = Alarma_Hora_2 - 1;
      }
      if (Handler_Boton (Boton2,2)){
        Blink = true;
        old_millis = millis () / 250;
        if (Alarma_Hora_2 == 23) Alarma_Hora_2 = 0;
        else Alarma_Hora_2 = Alarma_Hora_2 + 1;
      }
      if (Handler_Boton (Boton5,5)) {
        eeprom.write (Alarma_Hora_2_address,Alarma_Hora_2);
        Modo = 13;
      }
    }
    break;
    case (13): {                                 //Cambiar minutos alarma 2
      if (Blink) Mostrar_Hora (Alarma_Hora_2,Alarma_Minutos_2,true,true,true);
      else Mostrar_Hora (Alarma_Hora_2,0,true,true,false);
      if (Handler_Boton (Boton1,1)){
        Blink = true;
        old_millis = millis () / 250;
        if (Alarma_Minutos_2 == 0) Alarma_Minutos_2 = 59;
        else Alarma_Minutos_2 = Alarma_Minutos_2 - 1;
      }
      if (Handler_Boton (Boton2,2)){
        Blink = true;
        old_millis = millis () / 250;
        if (Alarma_Minutos_2 == 59) Alarma_Minutos_2 = 0;
        else  Alarma_Minutos_2 = Alarma_Minutos_2 + 1;
      }
      if (Handler_Boton (Boton5,5)) {
        eeprom.write (Alarma_Minutos_2_address,Alarma_Minutos_2);
        Modo = 14;
      }
    }
    break;
    case (14): {                                //Cambiar dia semana alarma 2
      Handler_Selc_Dias_Alarma (Selec_Dia_Alarma,Blink,2);
      if (Handler_Boton(Boton1,1) && Selec_Dia_Alarma != 0) Selec_Dia_Alarma = Selec_Dia_Alarma - 1;
      if (Handler_Boton(Boton2,2) && Selec_Dia_Alarma != 6) Selec_Dia_Alarma = Selec_Dia_Alarma + 1;
      if (Handler_Boton(Boton4,4)) {
        if (Selec_Dia_Alarma == 6){
          Dias_Alarma_2 [0] = !Dias_Alarma_2 [0];
        }
        if (Selec_Dia_Alarma <= 6){
          Dias_Alarma_2 [Selec_Dia_Alarma + 1] = !Dias_Alarma_2 [Selec_Dia_Alarma + 1];
        }
      }
      if (Handler_Boton(Boton5,5)) Modo = 15;
    }
    break;
    case (15): {                                //Activar / desactivar alarma 2
      if (Alarma_2_Activada == 1){
        Mostrar_Datos_Char ('O',0,0);
        Mostrar_Datos_Char ('N',1,0);
        Mostrar_Vacio (2);
        Mostrar_Vacio (3);
      }
      else {
        Mostrar_Datos_Char ('O',0,0);
        Mostrar_Datos_Char ('F',1,0);
        Mostrar_Datos_Char ('F',2,0);
        Mostrar_Vacio (3);
      }
      if (Handler_Boton (Boton4,4)){
        if (Alarma_2_Activada == 0) Alarma_2_Activada = 1;
        else Alarma_2_Activada = 0;
      }
      if (Handler_Boton (Boton5,5)) {
        Guardar_dias_alarma (2);
        Modo = 106;
      }
    }
    break;
    case (16): {                              //Menu KNX, Controlar lamparas
      Mostrar_Text ("LAMP",false,false);      //Ocupa 2 (20,21)
      Modo = Standart_Menu (16,20,false,true);
    }
    break;
    case (17): {                              //Menu KNX, Contorlar persiana
      Mostrar_Text ("PERS",false,false);      //Ocupa 1 (22)
      Modo = Standart_Menu (17,22,true,true);
    }
    break;
    case (18): {                              //Menu KNX, Programar acciones
      Mostrar_Text ("PROG",false,false);
      Modo = Standart_Menu (18,23,true,true); //Ocupa x (23, , , )
    }
    break;
    case (19): Modo = 107; break;               //Salir a menu KNX

    case (20):{                                 //Encender/apagar Lampara 1
      Mostrar_Datos_Char ('L',0,0);
      Mostrar_Datos (1,1,0,false);
      Mostrar_Vacio (2);
      if (Lamp_1) Mostrar_Datos (37,3,0,false);
      else Mostrar_Datos (36,3,0,false);
      if (Handler_Boton (Boton4,4)) Lamp_1 = !Lamp_1;
      if (Handler_Boton (Boton5,5)) Modo = 21;
    }
    break;
    case (21):{                                 //Encender/apagar Lampara 2
      Mostrar_Datos_Char ('L',0,0);
      Mostrar_Datos (2,1,0,false);
      Mostrar_Vacio (2);
      if (Lamp_2) Mostrar_Datos (37,3,0,false);
      else Mostrar_Datos (36,3,0,false);
      if (Handler_Boton (Boton4,4)) Lamp_2 = !Lamp_2;
      if (Handler_Boton (Boton5,5)) Modo = 16;
    }
    break;
    case (22): {                                //Subir/bajar persiana
      if (Handler_Boton (Boton1,1) && Posicion_Perisana > 0) Posicion_Perisana --;
      if (Handler_Boton (Boton2,2) && Posicion_Perisana < 5) Posicion_Perisana ++;
      Anim_Persiana (Posicion_Perisana);
      if (Handler_Boton (Boton5,5)) Modo = 17;
    }
    break;
    case (23):{                                 //Programacion Alarma 1
      Mostrar_Text ("ALR1",false,true);         //
      Modo = Standart_Menu (23,26,false,true);
    }
    break;
    case (24):{                                 //Programacion Alarma 2
      Mostrar_Text ("ALR2",false,true);         //
      Modo = Standart_Menu (24,27,true,true);
    }
    break;
    case (25): Modo = 107; break;
    case (26):{                                 //Programacion actuacion Alarma 1
      switch (Seleccion_Prog_KNX_1) {
        case (0): Mostrar_Text ("NADA",false,false); break;
        case (1): Mostrar_Text (" L1 ",false,false); break;
        case (2): Mostrar_Text (" L2 ",false,false); break;
        case (3): Mostrar_Text ("L1L2",false,false); break;
        case (4): Mostrar_Text ("RERS",false,false); break;
        case (5): Mostrar_Text ("P+L1",false,false); break;
        case (6): Mostrar_Text ("P+L2",false,false); break;
        case (7): Mostrar_Text ("TODO",false,false); break;
      }
      if (Handler_Boton (Boton1,1) && Seleccion_Prog_KNX_1 != 0) --Seleccion_Prog_KNX_1;
      if (Handler_Boton (Boton2,2) && Seleccion_Prog_KNX_1 != 7) ++Seleccion_Prog_KNX_1;
      if (Handler_Boton (Boton5,5)) Modo = 23;
    }
    break;
    case (27):{
      switch (Seleccion_Prog_KNX_2) {           //Programacion actuacion Alarma 2
        case (0): Mostrar_Text ("NADA",false,false); break;
        case (1): Mostrar_Text (" L1 ",false,false); break;
        case (2): Mostrar_Text (" L2 ",false,false); break;
        case (3): Mostrar_Text ("L1L2",false,false); break;
        case (4): Mostrar_Text ("RERS",false,false); break;
        case (5): Mostrar_Text ("P+L1",false,false); break;
        case (6): Mostrar_Text ("P+L2",false,false); break;
        case (7): Mostrar_Text ("TODO",false,false); break;
      }
      if (Handler_Boton (Boton1,1) && Seleccion_Prog_KNX_2 != 0) --Seleccion_Prog_KNX_2;
      if (Handler_Boton (Boton2,2) && Seleccion_Prog_KNX_2 != 7) ++Seleccion_Prog_KNX_2;
      if (Handler_Boton (Boton5,5)) Modo = 24;
    }
    break;
    case (28): {                                      //Cambiar brillo
      Mostrar_Datos (37,0);
      Mostrar_Datos (Separar_decena (Brillo),1,0);
      Mostrar_Datos (Separar_unidad (Brillo),2,0);
      Mostrar_Datos (37,3);

      if (Handler_Boton (Boton1,0) && Brillo != 0) {
        Brillo--;
        Cambiar_Brillo (Brillo);
      }
      if (Handler_Boton (Boton2,1) && Brillo != 15) {
        Brillo++;
        Cambiar_Brillo (Brillo);
      }
      
      if (Handler_Boton (Boton5,5)) Modo = 108;
    }
    break;

    //Menu
    case (99): Modo = 108; break;
    case (100): {                               //Titulo Hora -> HORA
      Mostrar_Text ("HORA",false,false);              //
      Shift = true;                             //
      Modo = Standart_Menu (100,0,true,true);  //Ocupa 1 (0)
    }
    break;
    case (101): {                               //Titulo Fecha -> FECH
      Mostrar_Text ("FECH",false,false);
      Shift = true;                             //
      Modo = Standart_Menu (101,1,true,true);   //Ocupa 1 (1)
    }
    break;
    case 102: {                                 //Titulo Temperatura -> TEMP
      Mostrar_Text ("TEMP",false,false);
      Modo = Standart_Menu (102,2,true,true);   //Ocupa 1 (2)
    }
    break;
    case 103: {                                 //Titulo Cambiar Hora -> C.HOR
      Mostrar_Text ("CHOR",true,false);
      Modo = Standart_Menu (103,3,true,true);   //Ocupa 2 (3,4)
    }
    break;
    case 104: {                                 //Titulo Cambiar Fecha -> C.FEC
      Mostrar_Text ("CFEC",true,false);
      Modo = Standart_Menu (104,5,true,true);   //Ocupa 3 (5,6,7)
    }
    break;
    case 105: {                                 //Titulo Alarma -> ALR1
      Mostrar_Text ("ALR1",false,true);         //
      Modo = Standart_Menu (105,8,true,true);   //Ocupa 4 (8,9,10,11)
    }
    break;
    case 106: {                                 //Titulo Alarma -> ALR2
      Mostrar_Text ("ALR2",false,true);         //
      Modo = Standart_Menu (106,12,true,true);  //Ocupa 4 (12,13,14,15)
    }
    break;
    case 107: {                                 //Titulo KNX -> KNX
      Mostrar_Datos_Char ('K',0,0);             //
      Mostrar_Datos_Char ('N',1,0);             //
      Mostrar_Datos_Char ('X',2,0);             //
      Mostrar_Vacio (3);                        //
      Modo = Standart_Menu (107,16,true,true);  //Ocupa 12 (16 - 27)
    }
    break;
    case (108): {
      Mostrar_Text ("BRIL");                    //Titulo Brillo -> BRIL
      Modo = Standart_Menu (108,28,true,true);  //
      if (Handler_Boton (Boton3,3)){            //
        Act_LED_Est = !Act_LED_Est;             //Ocupa 1 (28)
      }
    }
    break;
    case (109): Modo = 100; break;
    case (9999): {                              //Debug
      Mostrar_Text ("DEBG");
      Serial.print ("\n\n---DEBUG---\n");
      //Hora actual
      DateTime now = rtc.now();
      Serial.print ("Hora ");
      Serial.print (now.hour());
      Serial.print (" : ");
      Serial.print (now.minute());
      Serial.print (" : ");
      Serial.print (now.second());
      Serial.print ("\nFecha ");
      Serial.print (now.day());
      Serial.print (" / ");
      Serial.print (now.month());
      Serial.print (" / ");
      Serial.print (now.year());
      //Alarma 1
      Serial.print ("\nAlarma 1: h");
      Serial.print (Alarma_Hora_1);
      Serial.print (" m");
      Serial.print (Alarma_Minutos_1);
      Serial.print (" ,Activada ");
      Serial.print (Alarma_1_Activada);
      Serial.print (" ,Dias Activa L ");
      Serial.print (eeprom.read (100));
      Serial.print (" M ");
      Serial.print (eeprom.read (101));
      Serial.print (" X ");
      Serial.print (eeprom.read (102));
      Serial.print (" J ");
      Serial.print (eeprom.read (103));
      Serial.print (" V ");
      Serial.print (eeprom.read (104));
      Serial.print (" S ");
      Serial.print (eeprom.read (105));
      Serial.print (" D ");
      Serial.print (eeprom.read (106));
      //Alarma 2
      Serial.print ("\nAlarma 2: h");
      Serial.print (Alarma_Hora_2);
      Serial.print (" m");
      Serial.print (Alarma_Minutos_2);
      Serial.print (" ,Activada ");
      Serial.print (Alarma_2_Activada);
      Serial.print (" ,Dias Activa L ");
      Serial.print (eeprom.read (110));
      Serial.print (" M ");
      Serial.print (eeprom.read (111));
      Serial.print (" X ");
      Serial.print (eeprom.read (112));
      Serial.print (" J ");
      Serial.print (eeprom.read (113));
      Serial.print (" V ");
      Serial.print (eeprom.read (114));
      Serial.print (" S ");
      Serial.print (eeprom.read (115));
      Serial.print (" D ");
      Serial.print (eeprom.read (116));
      delay (5000);
    }
    break;
    default: {                                  //NULL (En caso de error)
      Mostrar_Datos_Char ('N',0,0);
      Mostrar_Datos_Char ('U',1,0);
      Mostrar_Datos_Char ('L',2,0);
      Mostrar_Datos_Char ('L',3,0);
      if (Handler_Boton (Boton5,5)){
        Modo = 0;
      }
    }
    break;
  }
  
  //Alarmas
  DateTime now = rtc.now();
  if (!Alarm_Act_1 && now.hour() == Alarma_Hora_1 && now.minute() == Alarma_Minutos_1 && now.second() == 0 && Alarma_1_Activada && Dias_Alarma_1[now.dayOfTheWeek()]) Alarm_Act_1 = true;
  if (!Alarm_Act_2 && now.hour() == Alarma_Hora_2 && now.minute() == Alarma_Minutos_2 && now.second() == 0 && Alarma_2_Activada && Dias_Alarma_2[now.dayOfTheWeek()]) Alarm_Act_2 = true;
    
  if (Alarm_Act_1){
    if (Blink) tone (Altavoz,880,100);
    if (Handler_Boton (Boton3,3)) Alarm_Act_1 = false;
    Handler_Porgramacion_KNX (Seleccion_Prog_KNX_1);
  }

  if (Alarm_Act_1){
    if (Blink) tone (Altavoz,880,100);
    if (Handler_Boton (Boton3,3)) Alarm_Act_2 = false;
    Handler_Porgramacion_KNX (Seleccion_Prog_KNX_2);
  }


  //Timers
  if (old_millis != millis() / 250) {
    old_millis = millis () / 250;
    Blink = !Blink;
  }

  //Enviar telegramas KNX
  if (Old_Lamp_1 != Lamp_1){
    Mandar_Telegrama_KNX (1,Lamp_1);
    Old_Lamp_1 = Lamp_1;
  }

  if (Old_Lamp_2 != Lamp_2){
    Mandar_Telegrama_KNX (2,Lamp_2);
    Old_Lamp_2 = Lamp_2;
  }

  if (Old_Posicion_Perisana != Posicion_Perisana){
    Mandar_Telegrama_KNX (3,Posicion_Perisana);
    Old_Posicion_Perisana = Posicion_Perisana;
  }

  //Control LED estado
  if (Act_LED_Est)
    digitalWrite (LED_BUILTIN,!digitalRead (LED_BUILTIN));
  else digitalWrite (LED_BUILTIN, LOW);
}