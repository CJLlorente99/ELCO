#include "fsm.h"
#include "Arduino.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DFRobotDFPlayerMini.h"
#include "CSV_Parser.h"
#include <Adafruit_NeoPixel.h>
// #include <FastLED.h>

#if defined(ESP32) 
    #include "HardwareSerial.h"
    #include <algorithm>
#else
    #include "SoftwareSerial.h"
    #include "SD.h"
#endif

/*  Tiempo que se muestra la matriz coloreada en verde antes de pasar a un nuevo juego */
#define TIMEOUTCORRECTO     2000

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

/*  Lenguajes */
#define ESPANOL 1
#define INGLES  2
#define FRANCES 3

/*  Colores */
#define LENCOLORES  11

/*  Estados de la FSM */
enum states{
    IDLE,
    ELECCIONLENGUAJE,
    ELECCIONJUEGO,
    JUEGONUMEROS,
    ESPERANUMEROS,
    JUEGOLETRAS,
    ESPERALETRAS,
    JUEGOCOLORES,
    ESPERACOLORES
};

/*  Declaracion de tipos utiles */
typedef struct fsm_data_s fsm_data_t;
typedef struct flags_s flags_t;
typedef struct matrizLED_s matrizLED_t;
typedef struct caracter_s caracter_t;

/*  Definicion de tipos utiles */
struct flags_s{
    volatile int repetirCaracter;
    volatile int nuevoJuego;
};

struct matrizLED_s{
    int R[64];
    int G[64];
    int B[64];
    int brightness[64];
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
    int lenguaje;
};

struct caracter_s{
    int red[64];
    int green[64];
    int blue[64];
    int brightness[64];
};

/*  Instanciacion de todos las guardas y funciones de transición */
static int siempre1(fsm_t* fsm);
static int lenguajeElegido(fsm_t* fsm);
static int juegoNumerosElegido(fsm_t* fsm);
static int juegoLetrasElegido(fsm_t* fsm);
static int juegoColoresElegido(fsm_t* fsm);
static int repetirCaracter(fsm_t* fsm);
static int matrizPulsadaCorrecta(fsm_t* fsm);
static int matrizPulsadaIncorrecta(fsm_t* fsm);
static int tiempoCumplido(fsm_t* fsm);
static int nuevoJuego(fsm_t* fsm);
static void initEleccionJuego(fsm_t* fsm);
static void initEleccionLenguaje(fsm_t* fsm);
static void initJuegoNumeros(fsm_t* fsm);
static void initJuegoLetras(fsm_t* fsm);
static void initJuegoColores(fsm_t* fsm);
static void playCaracter(fsm_t* fsm);
static void pintarMatrizCorrecta(fsm_t* fsm);
static void pintarMatrizIncorrecta(fsm_t* fsm);
static void pintarMatrizCorrectaColores(fsm_t* fsm);
static void pintarMatrizIncorrectaColores(fsm_t* fsm);

/*  Instantiacion de las funciones para cambiar el estado de las matrices */
caracter_t getRepresentacion(int caracter);
