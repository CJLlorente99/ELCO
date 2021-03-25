#include "fsm.h"
#include "allRGB.h"
#include "Arduino.h"
#include <stdio.h>
#include <stdlib.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "CSV_Parser.h"
#include "SD.h"

enum states{
    IDLE,
    JUEGONUMEROS,
    JUEGOLETRAS
};

typedef struct fsm_data_s fsm_data_t;
typedef struct flags_s flags_t;
typedef struct matrizLED_s matrizLED_t;

struct flags_s{
    volatile int numerosElegido : 1;
    volatile int letrasElegido : 1;
    volatile int repetirCaracter : 1;
};

struct matrizLED_s{
    int32_t HeightIndex[64];
    int32_t WidthIndex[64];
    int32_t R[64];
    int32_t G[64];
    int32_t B[64];
    int32_t Brightness[64];
    int numBoton;
    int caracterARepresentar;
};

struct fsm_data_s{
    flags_t flags;
    int caracterElegido;
    int matrizCorrecta;
    int ultimoBotonPulsado;
    matrizLED_t matricesLED[4];
};

fsm_t* new_ELCO_fsm(fsm_data_t* fsm_data);