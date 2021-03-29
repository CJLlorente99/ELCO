#include "model.h"

/*  En Arduino Uno podemos usar SW serial, en ESP32 no */
#if defined(ESP32)
    HardwareSerial dfPlayerSerial(2); // pines 16 y 17
#else
    SoftwareSerial dfPlayerSerial(2, 3); // RX, TX
#endif

static DFRobotDFPlayerMini myDFPlayer;

/*  Numero de los GPIO de los botones
    Considerar que los botones deben poder ponerse en modo pullup */
#define BOTONREPETIR    18
#define BOTONNUEVOJUEGO 14
#define BOTONMATRIZ1    23
#define BOTONMATRIZ2    22
#define BOTONMATRIZ3    21
#define BOTONMATRIZ4    19

/*  Declaracion de las ISR de los botones */
static void repetirElegidoISR();
static void nuevoJuegoISR();
static void botonMatriz1ISR();
static void botonMatriz2ISR();
static void botonMatriz3ISR();
static void botonMatriz4ISR();

/*  Declaracion de otras funciones que se utilizan */
void refrescarMatrices(fsm_data_t data);
void playNum(int num);
void playLetter(int num);
void rellenarMatrizPulsada(fsm_data_t* fsm_data);
void cambiarEstadoMatrices(fsm_data_t* fsm_data);

static fsm_t* fsm;
fsm_data_t* fsm_data;

void setup() {
    /*  Delay para "llegar" a ver el monitor serie desde el principio */
    delay(2000);

    /*  Iniciar comunicacion serial con el portatil y la que va a ser usada con el DFPlayer */
    Serial.begin(115200);
    dfPlayerSerial.begin(9600, SERIAL_8N1, 16, 17);

    // Serial.println();
    // Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

    // if (!myDFPlayer.begin(dfPlayerSerial)) {  //Use softwareSerial to communicate with mp3.
    // Serial.println(F("Unable to begin:"));
    // Serial.println(F("1.Please recheck the connection!"));
    // Serial.println(F("2.Please insert the SD card!"));
    // while(true);
    // }
    // Serial.println(F("DFPlayer Mini online."));

    // myDFPlayer.volume(20);  //Set volume value. From 0 to 30

    /*  Arrancar una semilla aleatoria */
    randomSeed(millis());

    /*  Inicializacion de los botones que van a ser usados
        Al estar en modo pullup el estado normal es nivel alto, bajando a nivel bajo cuando se presionen */
    pinMode(BOTONREPETIR, INPUT_PULLUP);
    pinMode(BOTONNUEVOJUEGO, INPUT_PULLUP);
    pinMode(BOTONMATRIZ1, INPUT_PULLUP);
    pinMode(BOTONMATRIZ2, INPUT_PULLUP);
    pinMode(BOTONMATRIZ3, INPUT_PULLUP);
    pinMode(BOTONMATRIZ4, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BOTONREPETIR), repetirElegidoISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONNUEVOJUEGO), nuevoJuegoISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ1), botonMatriz1ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ2), botonMatriz2ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ3), botonMatriz3ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ4), botonMatriz4ISR, FALLING);
}

void loop() {
    /*  Inicializacion de la estructura de datos que se usa en la maquina de estados
        Todo a cero excepto el numero de boton de cada matriz */
    fsm_data = (fsm_data_t*)malloc(sizeof(fsm_data_t));
    memset(fsm_data, 0, sizeof(fsm_data_t));

    fsm_data->matricesLED[0].numBoton = BOTONMATRIZ1;
    fsm_data->matricesLED[1].numBoton = BOTONMATRIZ2;
    fsm_data->matricesLED[2].numBoton = BOTONMATRIZ3;
    fsm_data->matricesLED[3].numBoton = BOTONMATRIZ4;

    fsm_trans_t tt[] = {
        {IDLE, siempre1, ELECCION, initEleccion},
        {ELECCION, juegoNumerosElegido, JUEGONUMEROS, initJuegoNumeros},
        {ELECCION, juegoLetrasElegido, JUEGOLETRAS, initJuegoLetras},
        {JUEGONUMEROS, repetirCaracter, JUEGONUMEROS, playCaracter},
        {JUEGONUMEROS, matrizPulsadaIncorrecta, JUEGONUMEROS, pintarMatrizIncorrecta},
        {JUEGONUMEROS, matrizPulsadaCorrecta, ESPERANUMEROS, pintarMatrizCorrecta},
        {ESPERANUMEROS, tiempoCumplido, JUEGONUMEROS, initJuegoNumeros},
        {JUEGOLETRAS, repetirCaracter, JUEGOLETRAS, playCaracter},
        {JUEGOLETRAS, matrizPulsadaIncorrecta, JUEGOLETRAS, pintarMatrizIncorrecta},
        {JUEGOLETRAS, matrizPulsadaCorrecta, ESPERALETRAS, pintarMatrizCorrecta},
        {ESPERALETRAS, tiempoCumplido, JUEGOLETRAS, initJuegoLetras},
        /* NUEVO JUEGO */
        {ELECCION, nuevoJuego, ELECCION, initEleccion},
        {JUEGONUMEROS, nuevoJuego, ELECCION, initEleccion},
        {ESPERANUMEROS, nuevoJuego, ELECCION, initEleccion},
        {JUEGOLETRAS, nuevoJuego, ELECCION, initEleccion},
        {ESPERALETRAS, nuevoJuego, ELECCION, initEleccion},
        {-1, NULL, -1, NULL}
    };

    fsm = fsm_new(IDLE, tt, fsm_data);

    Serial.println("Entering infinite loop");

    /*  Un "reloj"  para llamar a la maquina de estados y a la funcion de refresco */

    // TODO
    // Creo que en esp32 se puede hacer con timers 
    unsigned long lastMillisFSM = millis();
    unsigned long lastMillisLED = millis();
    unsigned long actMillis;

    while(1){
        actMillis = millis();
        if(actMillis - lastMillisFSM >= 200){
            lastMillisFSM = millis();
            fsm_fire(fsm);
        }
        if(actMillis - lastMillisLED >= 100){
            lastMillisLED = millis();
            refrescarMatrices(*fsm_data);
        }
    }
}

/*  ISR de los botones */
ISR_HEADER
repetirElegidoISR(){
    fsm_data->flags.repetirCaracter = 1;
}

ISR_HEADER
nuevoJuegoISR(){
    fsm_data->flags.nuevoJuego = 1;
}

ISR_HEADER
botonMatriz1ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ1;
}

ISR_HEADER
botonMatriz2ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ2;
}

ISR_HEADER
botonMatriz3ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ3;
}

ISR_HEADER
botonMatriz4ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ4;
}

/*  Funcion para refrescar las matrices en base a lo que hay dentro de fsm_data */
void
refrescarMatrices(fsm_data_t data){
    matrizLED_t* matrices = data.matricesLED;

    // TODO
}

/*********************************************************************************************/
/*********************************************************************************************/
/**************************IMPLEMENTACION DE LA MAQUINA DE ESTADOS****************************/
/*********************************************************************************************/
/*********************************************************************************************/

/*  Guards */
static int
siempre1(fsm_t* fsm){
    return 1;
}

static int
juegoNumerosElegido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[0].numBoton)
        res = 1;
    interrupts();

    return res;
}

static int
juegoLetrasElegido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[1].numBoton)
        res = 1;
    interrupts();

    return res;
}

static int
repetirCaracter(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->flags.repetirCaracter == 1)
        res = 1;
    interrupts();

    return res;
}

static int
matrizPulsadaCorrecta(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton)
        res = 1;
    interrupts();

    return res;
}

static int
matrizPulsadaIncorrecta(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->ultimoBotonPulsado != fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton && fsm_data->ultimoBotonPulsado != 0)
        res = 1;
    interrupts();

    return res;
}

static int
tiempoCumplido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    if(fsm_data->timeout >= millis()){
        res = 1;
    }

    return res;
}

static int
nuevoJuego(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->flags.nuevoJuego == 1)
        res = 1;
    interrupts();

    return res;
}

/* Transition functions */
static void
initEleccion(fsm_t* fsm){
    Serial.println("initEleccion");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    fsm_data->matricesLED[0].caracterARepresentar = random(48,57);
    fsm_data->matricesLED[1].caracterARepresentar = random(65,90);
    fsm_data->matricesLED[2].caracterARepresentar = 0;
    fsm_data->matricesLED[3].caracterARepresentar = 0;

    cambiarEstadoMatrices(fsm_data);

    noInterrupts();
    fsm_data->flags.nuevoJuego = 0;
    interrupts();
}

static void
initJuegoNumeros(fsm_t* fsm){
    Serial.println("initJuegoNumeros");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    fsm_data->caracterElegido = random(48,57); //+48 por codigo ascii
    Serial.print("Caracter elegido (ascii) ");
    Serial.print(fsm_data->caracterElegido);
    Serial.print(" que es el  ");
    Serial.println(fsm_data->caracterElegido - 48);

    playNum(fsm_data->caracterElegido);

    int matriz = random(3);
    fsm_data->matrizCorrecta = matriz;

    Serial.print("La matriz correcta es ");
    Serial.println(matriz);

    int num;
    int caracteresElegidos[4];
    memset(&caracteresElegidos, fsm_data->caracterElegido, 4*sizeof(int));
    for(int i = 0; i < 4; i++){
        if(i == fsm_data->matrizCorrecta){
            num = fsm_data->caracterElegido;
            fsm_data->matricesLED[i].caracterARepresentar = fsm_data->caracterElegido;
        } else{
            do
            {
                num = random(48,57);
            } while (std::find(std::begin(caracteresElegidos), std::end(caracteresElegidos), num) != std::end(caracteresElegidos));
            fsm_data->matricesLED[i].caracterARepresentar = num;
        }
        caracteresElegidos[i] = num;

        Serial.print("Matriz ");
        Serial.print(i);
        Serial.print(" caracter(ascii) ");
        Serial.println(num);
    }

    cambiarEstadoMatrices(fsm_data);

    noInterrupts();
    fsm_data->ultimoBotonPulsado = 0;
    interrupts();
}

static void
initJuegoLetras(fsm_t* fsm){
    Serial.println("initJuegoLetras");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    fsm_data->caracterElegido = random(65,90); // la ñ no se puede representar; + 65 por ascii

    String ascii;
    ascii = (char)fsm_data->caracterElegido;

    Serial.print("Caracter elegido (ascii) ");
    Serial.print(fsm_data->caracterElegido);
    Serial.print(" que es el  ");
    Serial.println(ascii);

    int matriz = random(3);
    fsm_data->matrizCorrecta = matriz;

    Serial.print("La matriz correcta es ");
    Serial.println(matriz);

    int num;
    int caracteresElegidos[4];
    memset(&caracteresElegidos, fsm_data->caracterElegido, 4*sizeof(int));
    for(int i = 0; i < 4; i++){
        if(i == fsm_data->matrizCorrecta){
            num = fsm_data->caracterElegido;
            fsm_data->matricesLED[i].caracterARepresentar = fsm_data->caracterElegido;
        } else{
            do
            {
                num = random(65,90);
            } while (std::find(std::begin(caracteresElegidos), std::end(caracteresElegidos), num) != std::end(caracteresElegidos));
            fsm_data->matricesLED[i].caracterARepresentar = num;
        }
        Serial.print("Matriz ");
        Serial.print(i);
        Serial.print(" caracter (ascii) ");
        Serial.println(num);
    }

    cambiarEstadoMatrices(fsm_data);

    noInterrupts();
    fsm_data->ultimoBotonPulsado = 0;
    interrupts();
}

static void
playCaracter(fsm_t* fsm){
    Serial.println("playCaracter");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    playNum(fsm_data->caracterElegido);

    noInterrupts();
    fsm_data->flags.repetirCaracter = 0;
    interrupts();
}

static void
pintarMatrizCorrecta(fsm_t* fsm){
    Serial.println("pintarMatrizCorrecta");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    rellenarMatrizPulsada(fsm_data);

    fsm_data->timeout = millis() + TIMEOUTCORRECTO;

    noInterrupts();
    fsm_data->ultimoBotonPulsado = 0;
    interrupts();
}

static void
pintarMatrizIncorrecta(fsm_t* fsm){
    Serial.println("pintarMatrizIncorrecta");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    rellenarMatrizPulsada(fsm_data);

    noInterrupts();
    fsm_data->ultimoBotonPulsado = 0;
    interrupts();
}

void
playNum(int num){
    Serial.print("Playing num ");
    Serial.println(num);
    // myDFPlayer.play(num);
}

/*********************************************************************************************/
/*********************************************************************************************/
/*************************************FUNCIONES AUXILIARES************************************/
/*********************************************************************************************/
/*********************************************************************************************/

/*  A partir del numero que debe mostrarse en cada momento, se actualizan los valores de las
    estructuras matrizLED_t */
void
cambiarEstadoMatrices(fsm_data_t* fsm_data){
    String csvName;
    int32_t* widthIndex;
    int32_t* heightIndex;
    int32_t* brightness;
    int32_t* R;
    int32_t* G;
    int32_t* B;
    for(int i = 0; i < 4; i++){
        matrizLED_t matriz = fsm_data->matricesLED[i];
        int caracterASCII = matriz.caracterARepresentar;
        String ascii;
        ascii = (char)caracterASCII;
        Serial.print("Cambiando matriz ");
        Serial.print(i);
        Serial.print(" ascii= ");
        Serial.println(ascii);
        if(caracterASCII == 0){
            csvName = "nada";
        } else if(caracterASCII >= 65){ // es una letra
            csvName = ascii + "mayus";
        } else{
            csvName = "numero" + ascii;
        }

        CSV_Parser cp(csvName.c_str(), /*format*/ "LLLLL");
        widthIndex = (int32_t*)cp["WIDTHINDEX"];
        heightIndex = (int32_t*)cp["HEIGHTINDEX"];
        brightness = (int32_t*)cp["BRIGHTNESS"];
        R = (int32_t*)cp["RED"];
        G = (int32_t*)cp["GREEN"];
        B = (int32_t*)cp["BLUE"];

        memcpy(&matriz.widthIndex, &widthIndex, 64*sizeof(int32_t));
        memcpy(&matriz.heightIndex, &heightIndex, 64*sizeof(int32_t));
        memcpy(&matriz.brightness, &brightness, 64*sizeof(int32_t));
        memcpy(&matriz.R,&R, 64*sizeof(int32_t));
        memcpy(&matriz.G,&G, 64*sizeof(int32_t));
        memcpy(&matriz.B,&B, 64*sizeof(int32_t));
    }
}

/*  Se rellena toda la matriz de color verde o rojo dependiendo si se ha pulsado la matriz correcta 
    o incorrecta */
void
rellenarMatrizPulsada(fsm_data_t* fsm_data){
    int ultBoton = fsm_data->ultimoBotonPulsado;

    if(ultBoton == fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton){
        Serial.print("Pintando de verde la matriz ");
        Serial.println(fsm_data->matrizCorrecta);
        memset(&fsm_data->matricesLED[fsm_data->matrizCorrecta].brightness, BRIGHTNESSVERDE, 64*sizeof(int));
        memset(&fsm_data->matricesLED[fsm_data->matrizCorrecta].R, RVERDE, 64*sizeof(int));
        memset(&fsm_data->matricesLED[fsm_data->matrizCorrecta].G, GVERDE, 64*sizeof(int));
        memset(&fsm_data->matricesLED[fsm_data->matrizCorrecta].B, BVERDE, 64*sizeof(int));
    } else {
        for(int i = 0; i < 4; i++){
            if(ultBoton == fsm_data->matricesLED[i].numBoton){
                Serial.print("Pintando de rojo la matriz ");
                Serial.println(i);
                memset(&fsm_data->matricesLED[i].brightness, BRIGHTNESSROJO, 64*sizeof(int));
                memset(&fsm_data->matricesLED[i].R, RROJO, 64*sizeof(int));
                memset(&fsm_data->matricesLED[i].G, GROJO, 64*sizeof(int));
                memset(&fsm_data->matricesLED[i].B, BROJO, 64*sizeof(int));
            }
        }
    }
}