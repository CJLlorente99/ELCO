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
#define BOTONREPETIR    19
#define BOTONNUEVOJUEGO 18
#define BOTONMATRIZ11   12
#define BOTONMATRIZ12   14
#define BOTONMATRIZ21   27
#define BOTONMATRIZ22   26
#define BOTONMATRIZ31   25
#define BOTONMATRIZ32   33
#define BOTONMATRIZ41   32
#define BOTONMATRIZ42   35

/*  Colores */
const String colores[LENCOLORES] = {"ROJO",
                                    "LIMA",
                                    "AZUL",
                                    "AMARILLO",
                                    "CYAN",
                                    "MAGENTA",
                                    "GRIS",
                                    "MARRON",
                                    "OLIVA",
                                    "VERDE",
                                    "VIOLETA",
};

/* Numero de pixeles por matriz y pin adjudicado a cada una */
#define PIN1        15 // Este pin es el que puse para probar la matriz en arduino UNO, pero en el ESP32 variara
#define PIN2        0
#define PIN3        2
#define PIN4        4
#define NUMPIXELS 64
#define BRIGHTNESS 50

Adafruit_NeoPixel pixels1(NUMPIXELS, PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2(NUMPIXELS, PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels3(NUMPIXELS, PIN3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels4(NUMPIXELS, PIN4, NEO_GRB + NEO_KHZ800);

/*  Declaracion de las ISR de los botones */
static void repetirElegidoISR();
static void nuevoJuegoISR();
static void botonMatriz11ISR();
static void botonMatriz12ISR();
static void botonMatriz21ISR();
static void botonMatriz22ISR();
static void botonMatriz31ISR();
static void botonMatriz32ISR();
static void botonMatriz41ISR();
static void botonMatriz42ISR();

/*  Declaracion de otras funciones que se utilizan */
void refrescarMatrices(fsm_data_t data);
void playNum(int num, int language);
void rellenarMatrizPulsada(fsm_data_t* fsm_data);
void rellenarMatrizPulsadaColores(fsm_data_t* fsm_data);
void cambiarEstadoMatrices(fsm_data_t* fsm_data);
int numeroNoRepetido(int elegidos[4], int num);

static fsm_t* fsm;
fsm_data_t* fsm_data;

void setup() {
    /*  Delay para "llegar" a ver el monitor serie desde el principio */
    delay(2000);

    /*  Iniciar comunicacion serial con el portatil y la que va a ser usada con el DFPlayer */
    Serial.begin(115200);
    #if defined(ESP32)
        dfPlayerSerial.begin(9600, SERIAL_8N1, 16, 17);
    #endif

    Serial.println();
    Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

    while(!myDFPlayer.begin(dfPlayerSerial)) {  //Use softwareSerial to communicate with mp3.
        Serial.println(F("Unable to begin:"));
        Serial.println(F("1.Please recheck the connection!"));
        Serial.println(F("2.Please insert the SD card!"));
    }
    Serial.println(F("DFPlayer Mini online."));

    myDFPlayer.volume(30);  //Set volume value. From 0 to 30

    /*  Arrancar una semilla aleatoria */
    randomSeed(millis());

    /*  Inicializacion de los botones que van a ser usados
        Al estar en modo pullup el estado normal es nivel alto, bajando a nivel bajo cuando se presionen */
    pinMode(BOTONREPETIR, INPUT_PULLUP);
    pinMode(BOTONNUEVOJUEGO, INPUT_PULLUP);
    pinMode(BOTONMATRIZ11, INPUT_PULLUP);
    pinMode(BOTONMATRIZ12, INPUT_PULLUP);
    pinMode(BOTONMATRIZ21, INPUT_PULLUP);
    pinMode(BOTONMATRIZ22, INPUT_PULLUP);
    pinMode(BOTONMATRIZ31, INPUT_PULLUP);
    pinMode(BOTONMATRIZ32, INPUT_PULLUP);
    pinMode(BOTONMATRIZ41, INPUT_PULLUP);
    pinMode(BOTONMATRIZ42, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BOTONREPETIR), repetirElegidoISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONNUEVOJUEGO), nuevoJuegoISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ11), botonMatriz11ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ12), botonMatriz12ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ21), botonMatriz21ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ22), botonMatriz22ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ31), botonMatriz31ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ32), botonMatriz32ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ41), botonMatriz41ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BOTONMATRIZ42), botonMatriz42ISR, FALLING);

    /* Added to implement LED matrix functionalities */
    pixels1.begin();           // INITIALIZE NeoPixel pixel object (REQUIRED)
    pixels1.show();            // Turn OFF all pixels ASAP
    pixels1.setBrightness(BRIGHTNESS);

    pixels2.begin();           
    pixels2.show();            
    pixels2.setBrightness(BRIGHTNESS);

    pixels3.begin();           
    pixels3.show();            
    pixels3.setBrightness(BRIGHTNESS);

    pixels4.begin();           
    pixels4.show();            
    pixels4.setBrightness(BRIGHTNESS);
}

void loop() {
    /*  Inicializacion de la estructura de datos que se usa en la maquina de estados
        Todo a cero excepto el numero de boton de cada matriz */
    fsm_data = (fsm_data_t*)malloc(sizeof(fsm_data_t));
    memset(fsm_data, 0, sizeof(fsm_data_t));

    fsm_data->matricesLED[0].numBoton1 = BOTONMATRIZ11;
    fsm_data->matricesLED[0].numBoton2 = BOTONMATRIZ12;
    fsm_data->matricesLED[1].numBoton1 = BOTONMATRIZ21;
    fsm_data->matricesLED[1].numBoton2 = BOTONMATRIZ22;
    fsm_data->matricesLED[2].numBoton1 = BOTONMATRIZ31;
    fsm_data->matricesLED[2].numBoton2 = BOTONMATRIZ32;
    fsm_data->matricesLED[3].numBoton1 = BOTONMATRIZ41;
    fsm_data->matricesLED[3].numBoton2 = BOTONMATRIZ42;

    fsm_trans_t tt[] = {
        {IDLE, siempre1, ELECCIONLENGUAJE, initEleccionLenguaje},

        {ELECCIONLENGUAJE, lenguajeElegido, ELECCIONJUEGO, initEleccionJuego},
        {ELECCIONLENGUAJE, repetirCaracter, ELECCIONLENGUAJE, initEleccionLenguaje},

        {ELECCIONJUEGO, juegoNumerosElegido, JUEGONUMEROS, initJuegoNumeros},
        {ELECCIONJUEGO, juegoLetrasElegido, JUEGOLETRAS, initJuegoLetras},
        {ELECCIONJUEGO, juegoColoresElegido, JUEGOCOLORES, initJuegoColores},

        {JUEGONUMEROS, repetirCaracter, JUEGONUMEROS, playCaracter},
        {JUEGONUMEROS, matrizPulsadaIncorrecta, JUEGONUMEROS, pintarMatrizIncorrecta},
        {JUEGONUMEROS, matrizPulsadaCorrecta, ESPERANUMEROS, pintarMatrizCorrecta},
        {ESPERANUMEROS, tiempoCumplido, JUEGONUMEROS, initJuegoNumeros},

        {JUEGOLETRAS, repetirCaracter, JUEGOLETRAS, playCaracter},
        {JUEGOLETRAS, matrizPulsadaIncorrecta, JUEGOLETRAS, pintarMatrizIncorrecta},
        {JUEGOLETRAS, matrizPulsadaCorrecta, ESPERALETRAS, pintarMatrizCorrecta},
        {ESPERALETRAS, tiempoCumplido, JUEGOLETRAS, initJuegoLetras},

        {JUEGOCOLORES, repetirCaracter, JUEGOCOLORES, playCaracter},
        {JUEGOCOLORES, matrizPulsadaIncorrecta, JUEGOCOLORES, pintarMatrizIncorrectaColores},
        {JUEGOCOLORES, matrizPulsadaCorrecta, ESPERACOLORES, pintarMatrizCorrectaColores},
        {ESPERACOLORES, tiempoCumplido, JUEGOCOLORES, initJuegoColores},        
        /* NUEVO JUEGO */
        {ELECCIONLENGUAJE, nuevoJuego, ELECCIONLENGUAJE, initEleccionLenguaje},
        {ELECCIONJUEGO, nuevoJuego, ELECCIONLENGUAJE, initEleccionLenguaje},
        {JUEGONUMEROS, nuevoJuego, ELECCIONLENGUAJE, initEleccionLenguaje},
        {ESPERANUMEROS, nuevoJuego, ELECCIONLENGUAJE, initEleccionLenguaje},
        {JUEGOLETRAS, nuevoJuego, ELECCIONLENGUAJE, initEleccionLenguaje},
        {ESPERALETRAS, nuevoJuego, ELECCIONLENGUAJE, initEleccionLenguaje},
        {JUEGOCOLORES, nuevoJuego, ELECCIONLENGUAJE, initEleccionLenguaje},
        {ESPERACOLORES, nuevoJuego, ELECCIONLENGUAJE, initEleccionLenguaje},
        {-1, NULL, -1, NULL}
    };

    fsm = fsm_new(IDLE, tt, fsm_data);

    Serial.println("Entering infinite loop");

    /*  Un "reloj"  para llamar a la maquina de estados y a la funcion de refresco */

    // TODO
    // Creo que en esp32 se puede hacer con timers 
    uint32_t lastMillisFSM = millis();
    uint32_t lastMillisLED = millis();
    uint32_t actMillis;

    while(1){
        actMillis = millis();
        if(actMillis - lastMillisFSM >= 525){
            Serial.print("FSM ");
            Serial.println(actMillis-lastMillisFSM);
            lastMillisFSM = millis();
            fsm_fire(fsm);
        }
        if(actMillis - lastMillisLED >= 525){
            Serial.print("LED ");
            Serial.println(actMillis-lastMillisLED);
            lastMillisLED = millis();
            refrescarMatrices(*fsm_data);
        }

        // Para valores menores de 525 no tiene sentido el light sleep (ni intentar hacer alguna accion para frecuencia mayores)
        // if((actMillis-lastMillisFSM <= 1000)&&(actMillis-lastMillisLED <= 1000)){
        //     if(actMillis-lastMillisLED < actMillis-lastMillisFSM){
        //         esp_sleep_enable_timer_wakeup((actMillis-lastMillisLED)*1000);
        //         Serial.print("Sleep time ");
        //         Serial.println(actMillis-lastMillisLED);
        //     } else{
        //         Serial.print("Sleep time ");
        //         Serial.println(actMillis-lastMillisFSM);
        //         esp_sleep_enable_timer_wakeup((actMillis-lastMillisFSM)*1000);
        //     }

        //     esp_light_sleep_start();
        //     Serial.println("Wakeup");
        // }
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
botonMatriz11ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ11;
}

ISR_HEADER
botonMatriz12ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ12;
}

ISR_HEADER
botonMatriz21ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ21;
}

ISR_HEADER
botonMatriz22ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ22;
}


ISR_HEADER
botonMatriz31ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ31;
}

ISR_HEADER
botonMatriz32ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ32;
}

ISR_HEADER
botonMatriz41ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ41;
}

ISR_HEADER
botonMatriz42ISR(){
    fsm_data->ultimoBotonPulsado = BOTONMATRIZ42;
}

/*  Funcion para refrescar las matrices en base a lo que hay dentro de fsm_data */
void
refrescarMatrices(fsm_data_t data){
    matrizLED_t* matrices = data.matricesLED;

    for (int i = 0; i < NUMPIXELS; i++){
        if ((i / 8) % 2){
            pixels1.setPixelColor((((i / 8) + 1)*8 - i%8 - 1), pixels1.Color(matrices[0].R[i], matrices[0].G[i], matrices[0].B[i]));
            pixels2.setPixelColor((((i / 8) + 1)*8 - i%8 - 1), pixels2.Color(matrices[1].R[i], matrices[1].G[i], matrices[1].B[i]));
            pixels3.setPixelColor((((i / 8) + 1)*8 - i%8 - 1), pixels3.Color(matrices[2].R[i], matrices[2].G[i], matrices[2].B[i]));
            pixels4.setPixelColor((((i / 8) + 1)*8 - i%8 - 1), pixels4.Color(matrices[3].R[i], matrices[3].G[i], matrices[3].B[i]));
        } else {
            pixels1.setPixelColor(i, pixels1.Color(matrices[0].R[i], matrices[0].G[i], matrices[0].B[i]));
            pixels2.setPixelColor(i, pixels2.Color(matrices[1].R[i], matrices[1].G[i], matrices[1].B[i]));
            pixels3.setPixelColor(i, pixels3.Color(matrices[2].R[i], matrices[2].G[i], matrices[2].B[i]));
            pixels4.setPixelColor(i, pixels4.Color(matrices[3].R[i], matrices[3].G[i], matrices[3].B[i]));
        }
        pixels1.show();
        pixels2.show();
        pixels3.show();
        pixels4.show();
    }
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
lenguajeElegido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if((fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[0].numBoton1)||(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[0].numBoton2)){
        fsm_data->lenguaje = ESPANOL;
        res = 1;
    } else if((fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[1].numBoton1)||(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[1].numBoton2)){
        fsm_data->lenguaje = INGLES;
        res = 1;
    } else if((fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[2].numBoton1)||(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[2].numBoton2)){
        fsm_data->lenguaje = FRANCES;
        res = 1;
    }
    interrupts();

    return res;
}

static int
juegoNumerosElegido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if((fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[0].numBoton1)||(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[0].numBoton2))
        res = 1;
    interrupts();

    return res;
}

static int
juegoLetrasElegido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if((fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[1].numBoton1)||(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[1].numBoton2))
        res = 1;
    interrupts();

    return res;
}

static int
juegoColoresElegido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if((fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[2].numBoton1)||(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[2].numBoton2))
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
    if((fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton1)||(fsm_data->ultimoBotonPulsado == fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton2))
        res = 1;
    interrupts();

    return res;
}

static int
matrizPulsadaIncorrecta(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(((fsm_data->ultimoBotonPulsado != fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton1)&&(fsm_data->ultimoBotonPulsado != fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton2)) && fsm_data->ultimoBotonPulsado != 0)
        res = 1;
    interrupts();

    return res;
}

static int
tiempoCumplido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    if(fsm_data->timeout <= millis()){
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
initEleccionLenguaje(fsm_t* fsm){
    Serial.println("initEleccionLenguaje");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    playNum(2,1);

    fsm_data->matricesLED[0].caracterARepresentar = 1; // ESPAÑOL
    fsm_data->matricesLED[1].caracterARepresentar = 2; // INGLES
    fsm_data->matricesLED[2].caracterARepresentar = 3; // FRANCES
    fsm_data->matricesLED[3].caracterARepresentar = 0;

    cambiarEstadoMatrices(fsm_data);

    noInterrupts();
    fsm_data->flags.nuevoJuego = 0;
    fsm_data->flags.repetirCaracter = 0;
    interrupts();
}

static void
initEleccionJuego(fsm_t* fsm){
    Serial.println("initEleccionJuego");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    playNum(1,fsm_data->lenguaje);

    fsm_data->matricesLED[0].caracterARepresentar = random(48,57);
    fsm_data->matricesLED[1].caracterARepresentar = random(65,90);
    fsm_data->matricesLED[2].caracterARepresentar = random(10, 10+LENCOLORES-1);
    fsm_data->matricesLED[3].caracterARepresentar = 0;

    cambiarEstadoMatrices(fsm_data);

    noInterrupts();
    fsm_data->ultimoBotonPulsado = 0;
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

    playNum(fsm_data->caracterElegido, fsm_data->lenguaje);

    int matriz = random(3);
    fsm_data->matrizCorrecta = matriz;

    Serial.print("La matriz correcta es ");
    Serial.println(matriz);

    int num;
    int caracteresElegidos[4] = {fsm_data->caracterElegido,fsm_data->caracterElegido,fsm_data->caracterElegido,fsm_data->caracterElegido};
    for(int i = 0; i < 4; i++){
        if(i == fsm_data->matrizCorrecta){
            num = fsm_data->caracterElegido;
            fsm_data->matricesLED[i].caracterARepresentar = fsm_data->caracterElegido;
        } else{
            #if defined(ESP32)
                do
                {
                    num = random(48,57);
                } while (std::find(std::begin(caracteresElegidos), std::end(caracteresElegidos), num) != std::end(caracteresElegidos));
            #else
                do{
                    num = random(65,90);
                } while(numeroNoRepetido(caracteresElegidos, num));
            #endif
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

    playNum(fsm_data->caracterElegido, fsm_data->lenguaje);

    int matriz = random(3);
    fsm_data->matrizCorrecta = matriz;

    Serial.print("La matriz correcta es ");
    Serial.println(matriz);

    int num;
    int caracteresElegidos[4] = {fsm_data->caracterElegido,fsm_data->caracterElegido,fsm_data->caracterElegido,fsm_data->caracterElegido};
    for(int i = 0; i < 4; i++){
        if(i == fsm_data->matrizCorrecta){
            num = fsm_data->caracterElegido;
            fsm_data->matricesLED[i].caracterARepresentar = fsm_data->caracterElegido;
        } else{
            #if defined(ESP32)
                do
                {
                    num = random(65,90);
                } while (std::find(std::begin(caracteresElegidos), std::end(caracteresElegidos), num) != std::end(caracteresElegidos));
            #else
                do{
                    num = random(65,90);
                } while(numeroNoRepetido(caracteresElegidos, num));
            #endif
            fsm_data->matricesLED[i].caracterARepresentar = num;
        }
        caracteresElegidos[i] = num;

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
initJuegoColores(fsm_t* fsm){
    Serial.println("initJuegoLetras");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    fsm_data->caracterElegido = random(10,10 + LENCOLORES-1);

    Serial.print("Numero color elegido ");
    Serial.print(fsm_data->caracterElegido);
    Serial.print(" Color elegido ");
    Serial.println(colores[fsm_data->caracterElegido-10]);

    playNum(fsm_data->caracterElegido, fsm_data->lenguaje);

    int matriz = random(3);
    fsm_data->matrizCorrecta = matriz;

    Serial.print("La matriz correcta es ");
    Serial.println(matriz);

    int num;
    int caracteresElegidos[4] = {fsm_data->caracterElegido,fsm_data->caracterElegido,fsm_data->caracterElegido,fsm_data->caracterElegido};
    for(int i = 0; i < 4; i++){
        if(i == fsm_data->matrizCorrecta){
            num = fsm_data->caracterElegido;
            fsm_data->matricesLED[i].caracterARepresentar = fsm_data->caracterElegido;
        } else{
            #if defined(ESP32)
                do
                {
                    num = random(10,10 + LENCOLORES-1);
                } while (std::find(std::begin(caracteresElegidos), std::end(caracteresElegidos), num) != std::end(caracteresElegidos));
            #else
                do{
                    num = random(10,10 + LENCOLORES);
                } while(numeroNoRepetido(caracteresElegidos, num));
            #endif
            fsm_data->matricesLED[i].caracterARepresentar = num;
        }
        caracteresElegidos[i] = num;

        Serial.print("Matriz ");
        Serial.print(i);
        Serial.print(" color ");
        Serial.println(colores[num-10]);
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

    playNum(fsm_data->caracterElegido, fsm_data->lenguaje);

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
pintarMatrizCorrectaColores(fsm_t* fsm){
    Serial.println("pintarMatrizCorrectaColores");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    rellenarMatrizPulsadaColores(fsm_data);

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

static void
pintarMatrizIncorrectaColores(fsm_t* fsm){
    Serial.println("pintarMatrizIncorrecta");
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    rellenarMatrizPulsadaColores(fsm_data);

    noInterrupts();
    fsm_data->ultimoBotonPulsado = 0;
    interrupts();
}

void
playNum(int num, int language){
    Serial.print("Playing num ");
    Serial.println(num);

    myDFPlayer.playFolder(language, num);
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

    for(int i = 0; i < 4; i++){
        int caracterASCII = fsm_data->matricesLED[i].caracterARepresentar;
        String ascii;
        ascii = (char)caracterASCII;
        Serial.print("Cambiando matriz ");
        Serial.print(i);
        Serial.print(" ascii= ");
        Serial.println(ascii);


        caracter_t caracter;
        caracter = getRepresentacion(caracterASCII);

        memcpy(fsm_data->matricesLED[i].brightness, caracter.brightness, 64*sizeof(int));
        memcpy(fsm_data->matricesLED[i].R, caracter.red, 64*sizeof(int));
        memcpy(fsm_data->matricesLED[i].G, caracter.green, 64*sizeof(int));
        memcpy(fsm_data->matricesLED[i].B, caracter.blue, 64*sizeof(int));
    }
}

/*  Se rellena toda la matriz de color verde o rojo dependiendo si se ha pulsado la matriz correcta 
    o incorrecta */
void
rellenarMatrizPulsada(fsm_data_t* fsm_data){
    int ultBoton = fsm_data->ultimoBotonPulsado;

    if((ultBoton == fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton1)||(ultBoton == fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton2)){
        Serial.print("Pintando de verde la matriz ");
        Serial.println(fsm_data->matrizCorrecta);
        memset(&fsm_data->matricesLED[fsm_data->matrizCorrecta].brightness, BRIGHTNESSVERDE, 64*sizeof(int));
        memset(&fsm_data->matricesLED[fsm_data->matrizCorrecta].R, RVERDE, 64*sizeof(int));
        memset(&fsm_data->matricesLED[fsm_data->matrizCorrecta].G, GVERDE, 64*sizeof(int));
        memset(&fsm_data->matricesLED[fsm_data->matrizCorrecta].B, BVERDE, 64*sizeof(int));
    } else {
        for(int i = 0; i < 4; i++){
            if((ultBoton == fsm_data->matricesLED[i].numBoton1)||(ultBoton == fsm_data->matricesLED[i].numBoton2)){
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

/*  Como el juego funciona con colores, no se puede rellenar. Si es incorrecta se apaga, si es correcta se apagan
    todas las demaś */
void
rellenarMatrizPulsadaColores(fsm_data_t* fsm_data){
    int ultBoton = fsm_data->ultimoBotonPulsado;

    if((ultBoton != fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton1)&&(ultBoton != fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton2)){
        Serial.print("Apagando la matriz incorrecta ");
        Serial.println(fsm_data->matrizCorrecta);
        for(int i = 0; i < 4; i++){
            if((ultBoton == fsm_data->matricesLED[i].numBoton1)||(ultBoton == fsm_data->matricesLED[i].numBoton2)){
                memset(&fsm_data->matricesLED[i].R, 0, 64*sizeof(int));
                memset(&fsm_data->matricesLED[i].G, 0, 64*sizeof(int));
                memset(&fsm_data->matricesLED[i].B, 0, 64*sizeof(int));
                memset(&fsm_data->matricesLED[i].brightness, 0, 64*sizeof(int));
            }
        }
    } else if((ultBoton == fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton1)||(ultBoton == fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton2)){
        for(int i = 0; i < 4; i++){
            if((ultBoton != fsm_data->matricesLED[i].numBoton1)&&(ultBoton != fsm_data->matricesLED[i].numBoton2)){
                Serial.print("Apagando todas las matrices menos la correcta ");
                memset(&fsm_data->matricesLED[i].R, 0, 64*sizeof(int));
                memset(&fsm_data->matricesLED[i].G, 0, 64*sizeof(int));
                memset(&fsm_data->matricesLED[i].B, 0, 64*sizeof(int));
                memset(&fsm_data->matricesLED[i].brightness, 0, 64*sizeof(int));
            }
        }
    }
}

/*  Se comprueba que el numero no es igual al correcto ni se ha puesto en una matriz ya */
int
numeroNoRepetido(int elegidos[4], int num){
    for(int i = 0; i < 4; i++){
        if(num == elegidos[i])
            return 1;
    }
    return 0;
}
