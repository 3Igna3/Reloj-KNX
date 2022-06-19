#include <LedController.hpp>
#include <AT24CX.h>

//Asignacion Pantallas
#define DIN 12
#define CLK 11
#define CS 10
#define Segments 4
#define delayTime 200

//Puntos
#define Puntos_Der B00000001
#define Puntos_Izq B10000000

//Salidas
#define Altavoz 9
#define Boton1 2
#define Boton2 3
#define Boton3 4
#define Boton4 5
#define Boton5 6

//Direcciones

//Diercciones Alarmas
#define Alarma_Hora_1_address 10
#define Alarma_Minutos_1_address 11
#define Alarma_Hora_2_address 12
#define Alarma_Minutos_2_address 13
#define Alarma_1_Activada_address 14
#define Alarma_2_Activada_address 15

//Direcciones I2C
//  RTC     > 0x68
//  EEPROM  > 0x57

AT24C32 eeprom;


LedController<Segments,1> lcd = LedController<Segments,1>();

bool _Estado [6] = {0,0,0,0,0,0};

//Inicializar vaiables Alarmas
byte Alarma_Hora_1 = 0;
byte Alarma_Minutos_1 = 0;
byte Alarma_Hora_2 = 0;
byte Alarma_Minutos_2 = 0;
int Alarma_1_Activada = 0;
int Alarma_2_Activada = 0;
bool Dias_Alarma_1 [7] = {1,0,1,0,1,0,1};   //D,L,M,X,J,V,S
bool Dias_Alarma_2 [7] = {0,1,0,1,0,1,0};   //D,L,M,X,J,V,S

//Marcas para el estado de las lamparas y persianaKNX
bool Lamp_1 = false, Lamp_2 = false;  
byte Posicion_Perisana = 0, Old_Posicion_Perisana = 0;

char Text_Encabezado_KNX [] =     "0E 01";
char Text_Direccion_L1 [] =       " 02 ";
char Text_Direccion_L2 [] =       " 03 ";
char Text_Direccion_Persiana [] = " 04 ";

byte Separar_decimal (double _Numero);
byte Separar_decena (int _Numero);
byte Separar_unidad (int _Numero);
int Standart_Menu (int _Modo, int _Vuelta,bool _MovIzq, bool _MovDer);
bool Handler_Anulador (int _Selec,int _Posicion, bool _Dato, bool _Anulador);
bool Handler_Cursor (int _Select, int _Posicion);

const byte Chr [41][8] = {
  {B00111100,B01100110,B01100110,B01100110,B01100110,B01100110,B01100110,B00111100},    //0 - 0
  {B00111100,B00011000,B00011000,B00011000,B00011000,B00011000,B00011100,B00011000},    //1 - 1
  {B01111110,B00000110,B00001100,B00011000,B00110000,B01100000,B01100110,B00111100},    //2 - 2
  {B00111100,B01100110,B01100000,B01100000,B00111000,B01100000,B01100110,B00111100},    //3 - 3
  {B00110000,B00110000,B01111110,B00110110,B00000110,B00001100,B00011000,B00110000},    //4 - 4
  {B00111100,B01100110,B01100000,B01100000,B00111110,B00000110,B00000110,B01111110},    //5 - 5
  {B00111100,B01100110,B01100110,B01100110,B00111110,B00000110,B00000110,B01111100},    //6 - 6
  {B00011000,B00011000,B00011000,B00011000,B00110000,B01100000,B01100000,B01111110},    //7 - 7
  {B00111100,B01100110,B01100110,B01100110,B00111100,B01100110,B01100110,B00111100},    //8 - 8
  {B00111100,B01100110,B01100000,B01111100,B01100110,B01100110,B01100110,B00111100},    //9 - 9
  {B01100110,B01100110,B01100110,B01111110,B01100110,B01100110,B01100110,B00111100},    //10 - A
  {B00111110,B01100110,B01100110,B01100110,B00111110,B01100110,B01100110,B00111110},    //11 - B
  {B00111100,B01100110,B00000110,B00000110,B00000110,B00000110,B01100110,B00111100},    //12 - C
  {B00111110,B01100110,B01100110,B01100110,B01100110,B01100110,B01100110,B00111110},    //13 - D
  {B01111110,B00000110,B00000110,B00000110,B00111110,B00000110,B00000110,B01111110},    //14 - E
  {B00000110,B00000110,B00000110,B00000110,B00111110,B00000110,B00000110,B01111110},    //15 - F
  {B01111100,B01100110,B01100110,B01110110,B00000110,B00000110,B00000110,B01111100},    //16 - G
  {B01100110,B01100110,B01100110,B01100110,B01111110,B01100110,B01100110,B01100110},    //17 - H
  {B00011000,B00011000,B00011000,B00011000,B00011000,B00011000,B00011000,B00011000},    //18 - I
  {B00011100,B00110110,B00110000,B00110000,B00110000,B00110000,B00110000,B00110000},    //19 - J
  {B01100110,B01100110,B00110110,B00011110,B00011110,B00110110,B01100110,B01100110},    //20 - K
  {B01111110,B01111110,B00000110,B00000110,B00000110,B00000110,B00000110,B00000110},    //21 - L
  {B01100011,B01100011,B01100011,B01100011,B01100011,B01101011,B01111111,B01110111},    //22 - M
  {B01100110,B01100110,B01110110,B01110110,B01101110,B01101110,B01100110,B01100110},    //23 - N
  {B00111100,B01100110,B01100110,B01100110,B01100110,B01100110,B01100110,B00111100},    //24 - O
  {B00000110,B00000110,B00000110,B00111110,B01100110,B01100110,B01100110,B00111110},    //25 - P
  {B01011100,B00100110,B01100110,B01100110,B01100110,B01100110,B01100110,B00111100},    //26 - Q
  {B01100110,B01100110,B00110110,B00111110,B01100110,B01100110,B01100110,B00111110},    //27 - R
  {B00111100,B01100110,B01100000,B01111100,B00111110,B00000110,B01100110,B00111100},    //28 - S
  {B00011000,B00011000,B00011000,B00011000,B00011000,B00011000,B01111110,B01111110},    //29 - T
  {B00111100,B01100110,B01100110,B01100110,B01100110,B01100110,B01100110,B01100110},    //30 - U
  {B00011000,B00111100,B01100110,B01100110,B01100110,B01100110,B01100110,B01100110},    //31 - V
  {B01100011,B01110111,B01111111,B01101011,B01100011,B01100011,B01100011,B01100011},    //32 - W
  {B01100110,B01100110,B00111100,B00011000,B00011000,B00111100,B01100110,B01100110},    //33 - X
  {B00011000,B00011000,B00011000,B00111100,B01100110,B01100110,B01100110,B01100110},    //34 - Y
  {B01111110,B01111110,B00001100,B00011000,B00110000,B01100000,B01111110,B01111110},    //35 - Z
  {B00011000,B00100100,B00111100,B01000010,B01000010,B01000010,B01000010,B00111100},    //36 - Luz apagada
  {B00011000,B00100100,B00111100,B01111110,B01111110,B01111110,B01111110,B00111100},    //37 - Luz encendida
  {B00000000,B01111110,B01111110,B01111110,B01111110,B01111110,B11111111,B11111111},    //38 - Persiana abajo
  {B00000000,B00000000,B00000000,B00000000,B00000000,B01111110,B11111111,B11111111},    //39 - Persiana arriba
  {B00000000,B00011000,B00011000,B01111110,B01111110,B00011000,B00011000,B00000000}     //40 - Simbolo +
};

static const byte Chr_Dias [4] [3] = {
    {B01010110,B01110010,B01010010},
    {B00101010,B01000100,B11101010},
    {B01100100,B01001110,B11001010},
    {B00000110,B00001010,B00000110}
};

void Init (int _Intensidad) {
    lcd.init(DIN,CLK,CS);
    lcd.setIntensity (_Intensidad);
}

void Cambiar_Brillo (byte& _Intensidad){
    lcd.setIntensity (_Intensidad);
}

void Mostrar_Chr_Dias (int _Pantalla) {
    for (int F = 6;F >= 4; F--){
        lcd.setRow (_Pantalla,F,Chr_Dias [_Pantalla] [F - 4]);
    }
    lcd.setRow (_Pantalla,3,0);
    lcd.setRow (_Pantalla,7,0);
}

void Handler_CHDDLS (int _Pantalla, bool _Var1, bool _Var2, bool _Selec1, bool _Selec2) {
    volatile int RAW = 0;
    volatile int RAW_Selector = 0;
    if (_Var1) RAW = B00000110;
    if (_Var2) RAW = RAW + B01100000;
    
    if (_Selec1) RAW_Selector = B00000110;
    else if (_Selec2) RAW_Selector = B01100000;
    
    lcd.setRow (_Pantalla,1,RAW);
    lcd.setRow (_Pantalla,2,RAW);
    lcd.setRow (_Pantalla,0,RAW_Selector);
}

void Handler_Selc_Dias_Alarma (int _Selec, bool _Parpadeo, int _NumAlarma) {
    Mostrar_Chr_Dias (0);
    Mostrar_Chr_Dias (1);
    Mostrar_Chr_Dias (2);
    Mostrar_Chr_Dias (3);

    volatile bool Dias_Temp [7] = {1,1,1,1,1,1,1};
    
    if (_NumAlarma == 1) {
        for (int N = 0;N <= 6;N++){
            Dias_Temp [N] = Dias_Alarma_1 [N];
        }
    }
    if (_NumAlarma == 2) {
        for (int N = 0;N <= 6;N++){
            Dias_Temp [N] = Dias_Alarma_2 [N];
        }
    }
    
    Handler_CHDDLS (0,Handler_Anulador(_Selec,0,Dias_Temp[1],_Parpadeo),Handler_Anulador(_Selec,1,Dias_Temp[2],_Parpadeo),Handler_Cursor(_Selec,0),Handler_Cursor(_Selec,1));
    Handler_CHDDLS (1,Handler_Anulador(_Selec,2,Dias_Temp[3],_Parpadeo),Handler_Anulador(_Selec,3,Dias_Temp[4],_Parpadeo),Handler_Cursor(_Selec,2),Handler_Cursor(_Selec,3));
    Handler_CHDDLS (2,Handler_Anulador(_Selec,4,Dias_Temp[5],_Parpadeo),Handler_Anulador(_Selec,5,Dias_Temp[6],_Parpadeo),Handler_Cursor(_Selec,4),Handler_Cursor(_Selec,5));
    Handler_CHDDLS (3,Handler_Anulador(_Selec,6,Dias_Temp[0],_Parpadeo),false,Handler_Cursor(_Selec,6),false);
}

bool Handler_Anulador (int _Selec,int _Posicion, bool _Dato, bool _Anulador) {
    if (_Selec == _Posicion){
        if (_Anulador) return 0;
        else return _Dato;
    }
    else return _Dato;
}

void Guardar_dias_alarma (int _NumAlarma){
    if (_NumAlarma == 1){
        int Addres = 100;
        for (int N = 0;N <= 6;N++){
            eeprom.write(Addres, Dias_Alarma_1 [N]);
            Addres++;
        }
        eeprom.write (Alarma_1_Activada_address,Alarma_1_Activada);
    }
    if (_NumAlarma == 2){
        int Addres = 110;
        for (int N = 0;N <= 6;N++){
            eeprom.write(Addres, Dias_Alarma_2 [N]);
            Addres++;
        }
        eeprom.write (Alarma_2_Activada_address,Alarma_2_Activada);
    }
}

void Leer_Dias_Alarma (int _NumAlarma) {
    if (_NumAlarma == 1){
        int Addres = 100;
        for (int N = 0;N <= 6;N++){
            Dias_Alarma_1 [N] = eeprom.read (Addres);
            Addres++;
        }
        Alarma_1_Activada = eeprom.read (Alarma_1_Activada_address);
    }
    if (_NumAlarma == 2){
        int Addres = 110;
        for (int N = 0;N <= 6;N++){
            Dias_Alarma_2 [N] = eeprom.read (Addres);
            Addres++;
        }
        Alarma_2_Activada = eeprom.read (Alarma_2_Activada_address);
    }
}

bool Handler_Cursor (int _Select, int _Posicion) {
    if (_Select == _Posicion)   return true;
    else return false;
}

void Mostrar_Datos (int _Num, int _Pantalla, int _Mod = 0, bool _Puntos = false) {
    for (int N = 0;N <= 7;N++){
        switch (_Mod) {                                                     //0
        case 0:                                                             //0000
        lcd.setRow (_Pantalla,N,Chr[_Num][N]);                              //
        break;
        case 1:                                                             //1
        if ((N == 1 || N == 2 || N == 5 || N == 6) && _Puntos)              //00:00
            lcd.setRow (_Pantalla,N,Chr[_Num][N] / 2 + Puntos_Izq);         //___|_
        else
            lcd.setRow (_Pantalla,N,Chr[_Num][N] / 2);
        break;
        case 2:                                                             //2
        if ((N == 1 || N == 2 || N == 5 || N == 6) && _Puntos)              //00:00
            lcd.setRow (_Pantalla,N,Chr[_Num][N] * 2 + Puntos_Der);         //_|___
        else
            lcd.setRow (_Pantalla,N,Chr[_Num][N] * 2);
        break;
        case 3:                                                             //3
            if (N == 1 || N == 0)                                           //00.0C
            lcd.setRow (_Pantalla,N,Chr[_Num][N] / 2 + Puntos_Izq);         //___|_
        else
            lcd.setRow (_Pantalla,N,Chr[_Num][N] / 2);
        break;
        case 4:                                                             //4
            if (N == 1 || N == 0)                                           //00.0C
            lcd.setRow (_Pantalla,N,Chr[_Num][N] * 2 + Puntos_Der);         //_|___
        else
            lcd.setRow (_Pantalla,N,Chr[_Num][N] * 2);
        break;
        case 5:
            lcd.setRow (_Pantalla,N,0);                                     //Mostrar vacio
        break;
        case 6:                                                             //6
            if ((N == 1 || N == 2 || N == 5 || N == 6) && _Puntos)          //  :  
                lcd.setRow (_Pantalla,N,Puntos_Izq);                        //___|_
            else
                lcd.setRow (_Pantalla,N,0);
        break;
        case 7:                                                             //7
            if ((N == 1 || N == 2 || N == 5 || N == 6) && _Puntos)          //  :  
                lcd.setRow (_Pantalla,N,Puntos_Der);                        //_|___
            else
                lcd.setRow (_Pantalla,N,0);
        break;
        case 8:                                                             //8
            if (N != 7)                                                     //00/00
                lcd.setRow (_Pantalla,N,Chr[_Num][N] / 2 + Puntos_Izq);     //___|_
            else
                lcd.setRow (_Pantalla,N,Chr[_Num][N] / 2);
            break;
        case 9:                                                             //9
            if (N != 0)                                                     //00/00
                lcd.setRow (_Pantalla,N,Chr[_Num][N] * 2 + Puntos_Der);     //_|___
            else
                lcd.setRow (_Pantalla,N,Chr[_Num][N] * 2);
            break;
        case 10:                                                            //10
            if (N != 7)                                                     //  /  
                lcd.setRow (_Pantalla,N,Puntos_Izq);                        //___|_
            else
                lcd.setRow (_Pantalla,N,0);
        break;
        case 11:                                                            //11
            if (N != 0)                                                     //  /  
                lcd.setRow (_Pantalla,N,Puntos_Der);                        //_|___
            else
                lcd.setRow (_Pantalla,N,0);
        break;
        default:                                                            //Error
            lcd.setRow (_Pantalla,N,B01010101);
        break;
        }
    }
};

void Mostrar_Datos_Char (char _Letra, int _Pantalla, int _Mod) {
    volatile int Caracter = _Letra;
    for (int N = 0;N <= 7;N++){switch (_Mod) {                                      //0
        case 0:                                                                     //0000
            lcd.setRow (_Pantalla,N,Chr[Caracter - 55][N]);                         //
        break;
        case 1:                                                                     //1
        if ((N == 1 || N == 2 || N == 5 || N == 6))                                 //00:00
            lcd.setRow (_Pantalla,N,Chr[Caracter - 55][N] / 2 + Puntos_Izq);        //___|_
        else
            lcd.setRow (_Pantalla,N,Chr[Caracter - 55][N] / 2);
        break;
        case 2:                                                                     //2
        if ((N == 1 || N == 2 || N == 5 || N == 6))                                 //00:00
            lcd.setRow (_Pantalla,N,Chr[Caracter - 55][N] * 2 + Puntos_Der);        //_|___
        else
            lcd.setRow (_Pantalla,N,Chr[Caracter - 55][N] * 2);
        break;
        case 3:                                                                     //3
            if (N == 1 || N == 0)                                                   //00.0C
            lcd.setRow (_Pantalla,N,Chr[Caracter - 55][N] / 2 + Puntos_Izq);        //_|___
        else
            lcd.setRow (_Pantalla,N,Chr[Caracter - 55][N] / 2);
        break;
        case 4:                                                                     //4
            if (N == 1 || N == 0)                                                   //00.0C
            lcd.setRow (_Pantalla,N,Chr[Caracter - 55][N] * 2 + Puntos_Der);        //___|_
        else
            lcd.setRow (_Pantalla,N,Chr[Caracter - 55][N] * 2);
        break;
        default:
            lcd.setRow (_Pantalla,N,B01010101);
        break;
        }
    }
};

void Mostrar_Text (const char _Text [4], bool _ConPunto1 = false, bool _ConPunto2 = false) {
    for (int N = 0;N <= 3;N++){
        int char2int = _Text [N];
        if (_ConPunto1 && (N == 0 || N == 1)) {
            if (N == 0){
                if (char2int < 57) Mostrar_Datos (char2int - 48,N,3,false);
                if (char2int > 57) Mostrar_Datos_Char (_Text [N],N,3);
            }
            if (N == 1){
                if (char2int < 57) Mostrar_Datos (char2int - 48,N,4,false);
                if (char2int > 57) Mostrar_Datos_Char (_Text [N],N,4);
            }
        }
        else if (_ConPunto2 && (N == 2 || N == 3)) {
            if (N == 2){
                if (char2int < 57) Mostrar_Datos (char2int - 48,N,3,false);
                if (char2int > 57) Mostrar_Datos_Char (_Text [N],N,3);
            }
            if (N == 3){
                if (char2int < 57) Mostrar_Datos (char2int - 48,N,4,false);
                if (char2int > 57) Mostrar_Datos_Char (_Text [N],N,4);
            }
        }
        else {
            if (char2int != 32 && char2int != 43){
                if (char2int < 57) Mostrar_Datos (char2int - 48,N,0,false);
                if (char2int > 57) Mostrar_Datos_Char (_Text [N],N,0);
            }
            else {
                if (char2int == 32) Mostrar_Datos (0,N,5,false);
                if (char2int == 43) Mostrar_Datos (40,N,0,false);
            }
        }
    }
}

void Mostrar_Temperatura (double _Temperatura, bool _Celsius) {
    if (Separar_decena (_Temperatura) == 0)
        Mostrar_Datos (0,0,5,0);
    else
        Mostrar_Datos (Separar_decena (_Temperatura),0,0,0);

    Mostrar_Datos (Separar_unidad (_Temperatura),1,3,0);
    Mostrar_Datos (Separar_decimal (_Temperatura),2,4,0);
    if (_Celsius)
        Mostrar_Datos (12,3,0,0);
    else
        Mostrar_Datos (15,3,0,0);
};

void Mostrar_Hora (int _Hora, int _Minutos, bool _Puntos, bool _Mostr_Hora, bool _Mostr_Min) {
    if (_Mostr_Hora) {
        if (Separar_decena (_Hora) == 0)
            Mostrar_Datos (0,0,5,_Puntos);
        else
            Mostrar_Datos (Separar_decena (_Hora),0,0,_Puntos);
        Mostrar_Datos (Separar_unidad (_Hora),1,1,_Puntos);
    }
    else {
        Mostrar_Datos (0,0,5,_Puntos);
        Mostrar_Datos (0,1,6,_Puntos);
    }
    if (_Mostr_Min){
        Mostrar_Datos (Separar_decena (_Minutos),2,2,_Puntos);
        Mostrar_Datos (Separar_unidad (_Minutos),3,0,_Puntos);
    }
    else {
        Mostrar_Datos (0,2,7,_Puntos);
        Mostrar_Datos (0,3,5,_Puntos);
    }
    
};

void Mostrar_Fecha (int _Dia, int _Mes,bool _Mostrar_Dia, bool _Mostrar_Mes) {
    if (_Mostrar_Dia){
        if (Separar_decena (_Dia) == 0)
            Mostrar_Datos (0,0,5,false);
        else 
            Mostrar_Datos (Separar_decena (_Dia),0,0,false);
        Mostrar_Datos (Separar_unidad(_Dia),1,8,false);
    }
    else{
        Mostrar_Datos (0,0,5,false);
        Mostrar_Datos (0,1,10,false);
    }
    if (_Mostrar_Mes){
        Mostrar_Datos (Separar_decena (_Mes),2,9,false);
        Mostrar_Datos (Separar_unidad(_Mes),3,0,false);
    }
    else {
        Mostrar_Datos (0,2,11,false);
        Mostrar_Datos (0,3,5,false);
    }
};

void Mostrar_Year (int _Year) {
    volatile short int Year_mil = _Year /1000;
    volatile short int Year_centena = (_Year /100) - (Year_mil * 10);
    volatile short int Year_decena = (_Year /10) - (Year_centena * 10) - (Year_mil * 100);
    volatile short int Year_unidad = _Year - (Year_decena * 10) - (Year_centena * 100) - (Year_mil * 1000);

    Mostrar_Datos (Year_mil,0,0,false);
    Mostrar_Datos (Year_centena,1,0,false);
    Mostrar_Datos (Year_decena,2,0,false);
    Mostrar_Datos (Year_unidad,3,0,false);
}

void Mostrar_Vacio (int _Pantalla) {
    for (int N = 0;N <= 7;N++){
        lcd.setRow (_Pantalla,N,0);
    }
};

bool Handler_Boton (int _Boton, int _PosMem) {
    if (digitalRead (_Boton) == true && _Estado [_PosMem] == false){
        _Estado [_PosMem] = true;
        delay (10);
        return true;
    }
    if (digitalRead (_Boton) == false && _Estado [_PosMem] == true){
        _Estado [_PosMem] = false;
    }
    delay (10);
    return false;
};

byte Separar_decena (int _Numero) {
    volatile short int Numero_decena = _Numero /10;
    return Numero_decena;
}

byte Separar_unidad (int _Numero) {
    volatile short int Numero_decena = _Numero /10;
    volatile short int Numero_unidad = _Numero - Numero_decena * 10;
    return Numero_unidad;
}

byte Separar_decimal (double _Numero) {
    volatile short int Numero_Fix = _Numero * 10;
    volatile short int Numero_decena = _Numero /10;
    volatile short int Numero_unidad = _Numero - Numero_decena * 10;
    volatile short int Numero_decimal = Numero_Fix - (Numero_unidad * 10) - (Numero_decena * 100);
    return Numero_decimal;
}

int Standart_Menu (int _Modo, int _Vuelta,bool _MovIzq, bool _MovDer) {
      if (Handler_Boton (Boton1,1) && _MovIzq) {
        return _Modo - 1;
      }
      if (Handler_Boton (Boton2,2) && _MovDer) {
        return _Modo + 1;
      }
      if (Handler_Boton (Boton5,5)) {
        return _Vuelta;
      }
      return _Modo;
};

/*Receptores:
    0 = NONE
    1 = Lampara 1
    2 = Lampara 2
    3 = Persiana
*/
void Mandar_Telegrama_KNX (byte _Receptor,byte _Accion){
    Serial.print (Text_Encabezado_KNX);
    switch (_Receptor) {
    case (1): {     //Lampara 1
        Serial.print (Text_Direccion_L1);
        Serial.print (_Accion);
        Serial.print (" ");
        Serial.print (1+1+1+2+_Accion);
    }
    break;
    case (2): {     //Lampara 2
        Serial.print (Text_Direccion_L2);
        Serial.print (_Accion);
        Serial.print (" ");
        Serial.print (1+1+1+3+_Accion);
    }
    break;
    case (3): {     //Persiana
        Serial.print (Text_Direccion_Persiana);
        volatile int Altura_persiana_TEMP = map (_Accion,0,5,0,255);
        Serial.print (Altura_persiana_TEMP);
        Serial.print (" ");
        Altura_persiana_TEMP += 1+1+1+4;
        if (Altura_persiana_TEMP > 255) Altura_persiana_TEMP -= 256;
        Serial.print (Altura_persiana_TEMP);
    }
    break;
    
    default: {
        Serial.print ("ERROR Receptor inexistente ");
        Serial.print (_Receptor);
    }
    break;
    Serial.print ("\n");
    }
}

/*Anim:
_Num (0 - 5)
*/
void Anim_Persiana (byte _Num) {
    Mostrar_Vacio (0);
    Mostrar_Vacio (3);

    lcd.setRow (1,7,255);
    lcd.setRow (1,6,255);
    lcd.setRow (2,7,255);
    lcd.setRow (2,6,255);

    lcd.setRow (1,5,B11111110);
    lcd.setRow (2,5,B01111111);

    if (_Num >= 1) {
        lcd.setRow (1,4,B11111110);
        lcd.setRow (2,4,B01111111);
    }
    else {
        lcd.setRow (1,4,0);
        lcd.setRow (2,4,0);
    }

    if (_Num >= 2) {
        lcd.setRow (1,3,B11111110);
        lcd.setRow (2,3,B01111111);
    }
    else {
        lcd.setRow (1,3,0);
        lcd.setRow (2,3,0);
    }

    if (_Num >= 3) {
        lcd.setRow (1,2,B11111110);
        lcd.setRow (2,2,B01111111);
    }
    else {
        lcd.setRow (1,2,0);
        lcd.setRow (2,2,0);
    }
    
    if (_Num >= 4) {
        lcd.setRow (1,1,B11111110);
        lcd.setRow (2,1,B01111111);
    }
    else {
        lcd.setRow (1,1,0);
        lcd.setRow (2,1,0);
    }

    if (_Num >= 5) {
        lcd.setRow (1,0,B11111110);
        lcd.setRow (2,0,B01111111);
    }
    else {
        lcd.setRow (1,0,0);
        lcd.setRow (2,0,0);
    }
}

void Handler_Porgramacion_KNX (byte _Seleccion_Prog_KNX) {
    switch (_Seleccion_Prog_KNX) {
        case (1): Lamp_1 = true; break;                   //L1
        case (2): Lamp_2 = true; break;                   //L2
        case (3): {                                       //L1 + L2
            Lamp_1 = true;
            Lamp_2 = true;
        }
        break;
        case (4): Posicion_Perisana = 255; break;          //Persiana
        case (5): {                                       //Persiana + L1
            Posicion_Perisana = 255;
            Lamp_1 = true;
        } 
        break;
        case (6): {                                       //Persiana + L2
            Posicion_Perisana = 255;
            Lamp_2 = true;
        }
        break;
        case (7): {                                       //Todo
            Posicion_Perisana = 255;
            Lamp_1 = true;
            Lamp_2 = true;
            }
        break;
    }
}