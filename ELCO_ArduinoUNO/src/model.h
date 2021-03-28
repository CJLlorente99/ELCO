#include "fsm.h"
#include "Arduino.h"
#include <stdio.h>
#include <stdlib.h>
#include "DFRobotDFPlayerMini.h"
#include "CSV_Parser.h"

#if defined(ESP32) 
#include "HardwareSerial.h"
#else
#include "SoftwareSerial.h"
#include "SD.h"
#endif

#define TIMEOUTCORRECTO     1000

// Colores para respuesta correcta o incorrecta
#define BRIGHTNESSVERDE     0
#define RVERDE  0
#define GVERDE  187
#define BVERDE  45

#define BRIGHTNESSROJO     0
#define RROJO   255
#define GROJO   0
#define BROJO   0

enum states{
    IDLE,
    ELECCION,
    JUEGONUMEROS,
    ESPERANUMEROS,
    JUEGOLETRAS,
    ESPERALETRAS
};

typedef struct fsm_data_s fsm_data_t;
typedef struct flags_s flags_t;
typedef struct matrizLED_s matrizLED_t;

struct flags_s{
    volatile int repetirCaracter : 1;
    volatile int nuevoJuego : 1;
};

struct matrizLED_s{
    int32_t heightIndex[64];
    int32_t widthIndex[64];
    int32_t R[64];
    int32_t G[64];
    int32_t B[64];
    int32_t brightness[64];
    int numBoton;
    int caracterARepresentar;
};

struct fsm_data_s{
    flags_t flags;
    int caracterElegido;
    int matrizCorrecta;
    int ultimoBotonPulsado;
    uint32_t timeout;
    matrizLED_t matricesLED[4];
};

fsm_t* new_ELCO_fsm(fsm_data_t* fsm_data);