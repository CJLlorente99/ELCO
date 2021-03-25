#include "model.h"

SoftwareSerial mySoftwareSerial(2, 3); // RX, TX
static DFRobotDFPlayerMini myDFPlayer;

void playNum(int num);
void playLetter(int num);
void rellenarMatrizPulsada(fsm_data_t* fsm_data);
void cambiarEstadoMatrices(fsm_data_t* fsm_data);

/* Guards */
static int
juegoNumerosElegido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->flags.numerosElegido)
        res = 1;
    interrupts();

    return res;
}

static int
juegoLetrasElegido(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->flags.letrasElegido)
        res = 1;
    interrupts();

    return res;
}

static int
repetirCaracter(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->flags.repetirCaracter)
        res = 1;
    interrupts();

    return res;
}

static int
matrizPulsada(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;
    int res = 0;

    noInterrupts();
    if(fsm_data->ultimoBotonPulsado != 0)
        res = 1;
    interrupts();

    return res;
}

/* Transition functions */
static void
initJuegoNumeros(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    fsm_data->caracterElegido = rand()%10 + 48; //+48 por codigo ascii

    playNum(fsm_data->caracterElegido);

    // Asignar aleatoriamente un numero a cada matriz, siendo uno el correcto
    int matriz = rand()%4;
    fsm_data->matrizCorrecta = matriz;

    // TODO
    // El numero no puede ser identico al que aparece en cualquier otra matriz

    int num;
    for(int i = 0; i < 4; i++){
        if(i == fsm_data->matrizCorrecta){
            fsm_data->matricesLED[i].caracterARepresentar = fsm_data->caracterElegido;
        } else{
            do
            {
                num = rand()%10 + 48;
            } while (num == fsm_data->caracterElegido);
             
            fsm_data->matricesLED[i].caracterARepresentar = num;
        }
    }

    cambiarEstadoMatrices(fsm_data);

    noInterrupts();
    fsm_data->flags.numerosElegido = 0;
    interrupts();
}

static void
initJuegoLetras(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    fsm_data->caracterElegido = rand()%26 + 65; // la ñ no se puede representar; + 65 por ascii

    // Asignar aleatoriamente un numero a cada matriz, siendo uno el correcto
    int matriz = rand()%4;
    fsm_data->matrizCorrecta = matriz;

    int num;
    for(int i = 0; i < 4; i++){
        if(i == fsm_data->matrizCorrecta){
            fsm_data->matricesLED[i].caracterARepresentar = fsm_data->caracterElegido;
        } else{
            do
            {
                num = rand()%26 + 65;
            } while (num == fsm_data->caracterElegido);
            
            
            fsm_data->matricesLED[i].caracterARepresentar = num;
        }
    }

    cambiarEstadoMatrices(fsm_data);

    noInterrupts();
    fsm_data->flags.letrasElegido = 0;
    interrupts();
}

static void
playCaracter(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    playNum(fsm_data->caracterElegido);

    noInterrupts();
    fsm_data->flags.repetirCaracter = 0;
    interrupts();
}

//TODO
// poner a verde o rojo segun corresponda
static void
pintarMatriz(fsm_t* fsm){
    fsm_data_t* fsm_data = (fsm_data_t*)fsm->user_data;

    rellenarMatrizPulsada(fsm_data);

    noInterrupts();
    fsm_data->ultimoBotonPulsado = 0;
    interrupts();
}

/* FSM init function */
fsm_t* new_ELCO_fsm(fsm_data_t* fsm_data){
    mySoftwareSerial.begin(9600);

    Serial.println();
    Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

    if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
    }
    Serial.println(F("DFPlayer Mini online."));

    myDFPlayer.volume(20);  //Set volume value. From 0 to 30

    // TODO
    // transición de juego a juego si es incorrecto
    // en el futuro, si aciertas, volver a estado juego para hacer otra ronda
    // transición de un juego a otro si se pulsa el otro boton

    fsm_trans_t tt[] = {
        {IDLE, juegoNumerosElegido, JUEGONUMEROS, initJuegoNumeros},
        {IDLE, juegoLetrasElegido, JUEGOLETRAS, initJuegoLetras},
        {JUEGONUMEROS, repetirCaracter, JUEGONUMEROS, playCaracter},
        {JUEGONUMEROS, matrizPulsada, IDLE, pintarMatriz},
        {JUEGOLETRAS, repetirCaracter, JUEGOLETRAS, playCaracter},
        {JUEGOLETRAS, matrizPulsada, IDLE, pintarMatriz},
        {-1, NULL, -1, NULL}
    };  
    
    fsm_t* fsm = fsm_new(IDLE, tt, fsm_data);
    return fsm;
}

void
playNum(int num){
    myDFPlayer.play(num);
}

void
cambiarEstadoMatrices(fsm_data_t* fsm_data){
    char* csvName;
    int32_t* WidthIndex;
    int32_t* HeightIndex;
    int32_t* R;
    int32_t* G;
    int32_t* B;
    for(int i = 0; i < 4; i++){
        matrizLED_t matriz = fsm_data->matricesLED[i];
        int caracterASCII = matriz.caracterARepresentar;
        if(caracterASCII >= 65){ // es una letra
            csvName = strcat((char*)&caracterASCII,"mayus");
        } else{
            csvName = strcat("numero",(char*)&caracterASCII);
        }

        CSV_Parser cp(csvName, /*format*/ "LLLLL");
        WidthIndex = (int32_t*)cp["WIDTHINDEX"];
        HeightIndex = (int32_t*)cp["HEIGHTINDEX"];
        R = (int32_t*)cp["RED"];
        G = (int32_t*)cp["GREEN"];
        B = (int32_t*)cp["BLUE"];

        memcpy(&matriz.WidthIndex, &WidthIndex, 64*sizeof(int32_t));
        memcpy(&matriz.HeightIndex, &HeightIndex, 64*sizeof(int32_t));
        memcpy(&matriz.R,&R, 64*sizeof(int32_t));
        memcpy(&matriz.G,&G, 64*sizeof(int32_t));
        memcpy(&matriz.B,&B, 64*sizeof(int32_t));
    }
}

void
rellenarMatrizPulsada(fsm_data_t* fsm_data){
    int ultBoton = fsm_data->ultimoBotonPulsado;

    if(ultBoton == fsm_data->matricesLED[fsm_data->matrizCorrecta].numBoton){
        // TODO
        // rellenar en verde lo que estaba en blanco o poner directamente todo a verde
    } else {
        // TODO
        // ver a que matriz pertenece el boton pulsado
        // rellenar en rojo lo que estaba en blanco o poner directamente todo a rojo
    }
}