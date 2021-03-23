#include "model.h"

#define BOTONNUMEROS    1
#define BOTONLETRAS     2
#define BOTONREPETIR    3
#define BOTONMATRIZ1    4
#define BOTONMATRIZ2    5
#define BOTONMATRIZ3    6
#define BOTONMATRIZ4    7

fsm_t* fsm;
fsm_data_t* fsm_data;

static void numerosElegidoISR();
static void letrasElegidoISR();
static void repetirElegidoISR();
static void botonMatriz1ISR();
static void botonMatriz2ISR();
static void botonMatriz3ISR();
static void botonMatriz4ISR();

void setup() {
    Serial.begin(115200);

    pinMode(BOTONNUMEROS, INPUT_PULLUP);
    pinMode(BOTONLETRAS, INPUT_PULLUP);
    pinMode(BOTONREPETIR, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BOTONNUMEROS), numerosElegidoISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONLETRAS), letrasElegidoISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONREPETIR), repetirElegidoISR, FALLING);

    // Inicializar los botones de cada matriz led
    pinMode(BOTONMATRIZ1, INPUT_PULLUP);
    pinMode(BOTONMATRIZ2, INPUT_PULLUP);
    pinMode(BOTONMATRIZ3, INPUT_PULLUP);
    pinMode(BOTONMATRIZ4, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ1), botonMatriz1ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ2), botonMatriz2ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ3), botonMatriz3ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ4), botonMatriz4ISR, FALLING);
}

void loop() {
    fsm_data = (fsm_data_t*)malloc(sizeof(fsm_data_t));
    memset(fsm_data, 0, sizeof(fsm_data_t));
    // fsm_data->matricesLED = (matrizLED_t*)malloc(4*sizeof(matrizLED_t));

    // rellenar la info necesaria en fsm_data->matricesLED, si procede
    fsm_data->matricesLED[0].numBoton = BOTONMATRIZ1;
    fsm_data->matricesLED[1].numBoton = BOTONMATRIZ2;
    fsm_data->matricesLED[2].numBoton = BOTONMATRIZ3;
    fsm_data->matricesLED[3].numBoton = BOTONMATRIZ4;

    fsm = new_ELCO_fsm(fsm_data);

    Serial.println("Entering infinite loop");

    unsigned long lastMillis = millis();
    unsigned long actMillis;

    while(1){
        actMillis = millis();
        if(actMillis - lastMillis >= 100){
            lastMillis = millis();
            fsm_fire(fsm);
        }
    }
}

static void
numerosElegidoISR(){
    fsm_data->flags.numerosElegido = 1;
}

static void
letrasElegidoISR(){
    fsm_data->flags.letrasElegido = 1;
}

static void
repetirElegidoISR(){
    fsm_data->flags.repetirCaracter = 1;
}

static void
botonMatriz1ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ1;
}

static void
botonMatriz2ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ2;
}

static void
botonMatriz3ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ3;
}

static void
botonMatriz4ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ4;
}