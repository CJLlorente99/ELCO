#include "fsm.h"
#include "Arduino.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "DFRobotDFPlayerMini.h"
#include "CSV_Parser.h"

#if defined(ESP32) 
    #include "HardwareSerial.h"
#else
    #include "SoftwareSerial.h"
    #include "SD.h"
#endif

/*  Tiempo que se muestra la matriz coloreada en verde antes de pasar a un nuevo juego */
#define TIMEOUTCORRECTO     3000

/*  En ESP32 se recomienda guardar los ISR en la memoria RAM en vez de en la flash */
#if defined(ESP32) 
    #define ISR_HEADER static void IRAM_ATTR
#else
    #define ISR_HEADER static void
#endif

/*  Colores para respuesta correcta o incorrecta */
#define BRIGHTNESSVERDE     0
#define RVERDE  0
#define GVERDE  187
#define BVERDE  45

#define BRIGHTNESSROJO     0
#define RROJO   255
#define GROJO   0
#define BROJO   0

/*  Estados de la FSM */
enum states{
    IDLE,
    ELECCION,
    JUEGONUMEROS,
    ESPERANUMEROS,
    JUEGOLETRAS,
    ESPERALETRAS
};

/*  Declaracion de tipos utiles */
typedef struct fsm_data_s fsm_data_t;
typedef struct flags_s flags_t;
typedef struct matrizLED_s matrizLED_t;

/*  Definicion de tipos utiles */
struct flags_s{
    volatile int repetirCaracter;
    volatile int nuevoJuego;
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
    long caracterElegido;
    long matrizCorrecta;
    volatile int ultimoBotonPulsado;
    uint32_t timeout;
    matrizLED_t matricesLED[4];
};

/*  Instanciacion de todos las guardas y funciones de transición */
static int siempre1(fsm_t* fsm);
static int juegoNumerosElegido(fsm_t* fsm);
static int juegoLetrasElegido(fsm_t* fsm);
static int repetirCaracter(fsm_t* fsm);
static int matrizPulsadaCorrecta(fsm_t* fsm);
static int matrizPulsadaIncorrecta(fsm_t* fsm);
static int tiempoCumplido(fsm_t* fsm);
static int nuevoJuego(fsm_t* fsm);
static void initEleccion(fsm_t* fsm);
static void initJuegoNumeros(fsm_t* fsm);
static void initJuegoLetras(fsm_t* fsm);
static void playCaracter(fsm_t* fsm);
static void pintarMatrizCorrecta(fsm_t* fsm);
static void pintarMatrizIncorrecta(fsm_t* fsm);