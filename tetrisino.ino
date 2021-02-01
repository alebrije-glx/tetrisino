
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
// PINES A USAR PARA LA INTERACCION CON EL JUEGO.
/*
#define PIN_BOTON_IZQ    2   // Definir bien que puertos
#define PIN_BOTON_DER   3   // son los que usare para
#define PIN_BOTON_ABAJO 4   // controlar el juego del
#define PIN_BOTON_ROTAR 5   // tetris.
*/
#define PIN_BOTON_FIN   7   // Para finalizar el juego.

//#define PIN_TIRA_LED  6   // Definir el pin para la salida de datos
//#define NUM_LEDS      256 // de la tira y su cantidad de leds.

// ***** VARIABLES GLOBALES PARA EL JUEGO ******************************

#define TABLERO_H 16    // Altura del tablero (filas)
#define TABLERO_W 16    // Ancho del tablero (columnas)

Adafruit_NeoPixel tiraled = Adafruit_NeoPixel(256, 6, NEO_GRB + NEO_KHZ800);

const uint32_t colores[6] = { tiraled.Color(255, 0, 255),    // magenta
                              tiraled.Color(255, 0, 255),    // rojo
                              tiraled.Color(0, 255, 0),      // verde
                              tiraled.Color(0, 0, 255),      // azul
                              tiraled.Color(255, 255, 0),    // amarillo
                              tiraled.Color(0, 255, 255) };  // rojo

//          0          1           2            3           4          5        6
enum {F_CUADRADO, F_LNORMAL, F_LINVERTIDA, F_ZNORMAL, F_ZINVERTIDA, F_PODIO, F_LARGA};

enum {M_IZQ, M_DER, M_ABAJO};

const int8_t formas_2x2[2][2] = { {1,0},
                                  {1,1} };
                             
const int8_t formas_3x3[5][3][3] = {{ {1,0,0},
                                      {1,0,0},
                                      {1,1,0} },
                                    { {0,0,1},
                                      {0,0,1},
                                      {0,1,1} },
                                    { {0,0,1},
                                      {0,1,1},
                                      {0,1,0} },
                                    { {1,0,0},
                                      {1,1,0},
                                      {0,1,0} },
                                    { {0,1,0},
                                      {1,1,1},
                                      {0,0,0} }};
                                 
const int8_t formas_4x4[4][4] = { {0,0,0,0},
                                  {0,0,0,0},
                                  {1,1,1,1},
                                  {0,0,0,0} };

int8_t tablero[TABLERO_H][TABLERO_W];

int8_t led_color[TABLERO_H][TABLERO_W]; // para almacenar los colores de cada led.

int8_t figura_4x4[4][4];   // En estos arreglos (matrices) se almacenan
int8_t figura_3x3[3][3];   // las piezas para realizar las operaciones de 
int8_t figura_2x2[2][2];   // movimiento y rotacion sobre estas.

int tipo_forma, tamano, pza_color;
int pieza_colocada, game_over;
int pos_x, pos_y, top_x;    // top_x es la posicion de la pieza que esta colocada mas arriba.
                            // que sirve como limite para hacer el recorrido de las filas
int opcion, mov_abajo;      // Para leer el teclado debe ser un entero, no char (en ncurses)

unsigned long tiempo_anterior, tiempo_limite = 1000, tiempo_espera; // en milisegundos

void setup()
{
    pinMode(2, INPUT);
    pinMode(3, INPUT);
    pinMode(4, INPUT);
    pinMode(5, INPUT);
    pinMode(PIN_BOTON_FIN,   INPUT);

    #if defined (__AVR_ATtiny85__)
        if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
    #endif
    
    tiraled.begin();    // Inicializa tira led.
    tiraled.setBrightness(5); // Brillo de la tira led.
    tiraled.show(); // Inicializa todos los led apagandolos (off)
    
    randomSeed(millis());
        
    tiempo_anterior = millis();
    tiempo_espera = millis();
    
    pieza_colocada = 1, game_over = 0;
    opcion = 0;
    mov_abajo = 0;
    
    top_x = (int)TABLERO_H;
    
    inicializa_tablero();
}


void loop()
{
    if(opcion != 5 && !game_over)
    {   
        if(pieza_colocada)
        {
            pos_x = 0; pos_y = (TABLERO_W / 2) - 2;

            selecciona_figura();
            asigna_figura();
            
            if(detecta_colision(pos_x, pos_y))
                game_over = 1;
            else
                pieza_colocada = 0;
            
            imprime_tablero();  // ¿Este no es necesario?
            imprime_figura();   // Aquí se llama a la funcion show()
        }
        
        if(!game_over)
        {
            // Se mueve hacia abajo automaticamente cada determinado tiempo.
            if(mov_abajo) opcion = 3; // <<<---------
            //else opcion = 0;
            else if( (millis() - tiempo_espera) > 100)
            {
                if(digitalRead(2))     opcion = 1;
                else if(digitalRead(3))     opcion = 2;
                else if(digitalRead(4))   opcion = 3;
                else if(digitalRead(5))   opcion = 4;
                else if(digitalRead(PIN_BOTON_FIN))     opcion = 5;
                
                tiempo_espera = millis();
            }

            switch(opcion)  // opcion sera el boton presionado
            {   
                 case 4: // Rotación
                    if(!rotar_figura()) {
                        imprime_tablero(); // ¿Este no es necesario? <-----
                        imprime_figura();
                    }
                    break;
                
                case 1: // Izquierda
                    if(pos_y - 1 >= 0)
                        verifica_movimiento(M_IZQ);
                    else
                        movimiento_en_limites(M_IZQ);
                    break;

                case 2: // Derecha
                    if(pos_y + 1 <= TABLERO_W - tamano)
                        verifica_movimiento(M_DER);
                    else
                        movimiento_en_limites(M_DER);
                    break;
                
                case 3: // Abajo
                    if(pos_x + 1 <= TABLERO_H - tamano)
                        verifica_movimiento(M_ABAJO);
                    else
                        movimiento_en_limites(M_ABAJO);

                    if(pieza_colocada)
                    {
                        if (pos_x < top_x) top_x = pos_x;
                        
                        coloca_figura_en_tablero();
                        verifica_linea_completa();
                    }
                    if(mov_abajo)
                        mov_abajo = 0;
                    break;
                
                case 5:                    
                    game_over = 1;
                    delay(2000);
                    tiraled.clear();
                    tiraled.show();
                    break;
                    
                default:
                // No se hizo movimiento en el tiempo establecido, se desplaza hacia abajo.
                    if((millis() - tiempo_anterior) > tiempo_limite)
                    {
                        mov_abajo = 1;
                        tiempo_anterior = millis();
                    }
                    break;
            } // switch
            opcion = 0;
        } // el if de game over
    } // primer if( opcion != 5 && !game_over )
} // loop principal.


void movimiento_en_limites(int direccion)
{
    if(verifica_espacio(direccion))
    {
        if(!movimiento_interno(direccion))
        {          
            imprime_tablero(); // Esta no creo que sea necesaria.
            imprime_figura();
        }
        else if(direccion == M_ABAJO) pieza_colocada = 1;
    }       
    else if(direccion == M_ABAJO) pieza_colocada = 1;
}

int movimiento_interno(int direccion)
{
    int i, j, py, px, colision = 0;
    int matriz[tamano][tamano];
    
    switch(direccion)
    {
        case M_IZQ:
            for(i = 0; i < tamano; i++)
                for(j = 0; j < tamano ; j++)
                {
                    if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO)
                        matriz[i][j] = (j < tamano -1) ? figura_3x3[i][j+1] : 0;
                    else
                        matriz[i][j] = (j < tamano -1) ? figura_4x4[i][j+1] : 0;
                }
            break;
            
        case M_DER:
            for(i = 0; i < tamano; i++)
                for(j = tamano - 1; j >= 0; j--)
                {
                    if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO)
                        matriz[i][j] = (j > 0) ? figura_3x3[i][j-1] : 0;
                    else
                        matriz[i][j] = (j > 0)? figura_4x4[i][j-1] : 0;
                }
            break;
        
        case M_ABAJO:
            for(i = tamano - 1; i >= 0; i--)
                for(j = 0; j < tamano; j++)
                {
                    if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO)
                        matriz[i][j] = (i > 0 ? figura_3x3[i-1][j] : 0);
                    else
                        matriz[i][j] = (i > 0 ? figura_4x4[i-1][j] : 0);
                }
            break;
    }
    // Detectar la colisión de la figura (matriz) del movimiento.
    i = 0; px = pos_x;
    while(!colision && i < tamano) {
        py = pos_y; j = 0;
        while(!colision && j < tamano)
        {
            if(matriz[i][j] && tablero[px][py]) colision = 1; 
            j++; py++;
        }
        i++; px++;
    }
    // Si no hay colision, se ajusta la pieza.
    if(!colision) {
        for(i = 0; i < tamano; i++)
            for(j = 0; j < tamano; j++)
            {
                if(tipo_forma == F_CUADRADO) figura_2x2[i][j] = matriz[i][j]; // Esta reasignación no es necesaria.
                else if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) figura_3x3[i][j] = matriz[i][j];
                else figura_4x4[i][j] = matriz[i][j];
            }
    }  
    return colision;
}

int verifica_espacio(int direccion)
{   
    int k = 0, espacio = 1, aux;
    
    if(tipo_forma == F_CUADRADO) espacio = 0;
    else
    {
        if(direccion == M_IZQ || direccion == M_DER) aux = (direccion == M_DER ? tamano - 1 : 0);
        
        while(espacio && k < tamano)
        {
            if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) {
                if(figura_3x3[direccion == M_ABAJO ? tamano - 1 : k][direccion == M_ABAJO ? k : aux]) espacio = 0; }
            else if(figura_4x4[direccion == M_ABAJO ? tamano - 1 : k][direccion == M_ABAJO ? k : aux]) espacio = 0;
            k++;
        }
    }               
    return espacio;
}

void filtra_cima()
{
    int col, fila_vacia = 1;
    
    while(fila_vacia && top_x < TABLERO_H) {
        col = 0;
        while(fila_vacia && col < TABLERO_W)
        {
            if(tablero[top_x][col])
                fila_vacia = 0;
            col++;
        }
        if(fila_vacia)
            top_x = top_x + 1;
    }
}

void verifica_linea_completa()
{
    int i, j, px = pos_x, linea_completa;
    
    filtra_cima();
    
    for(i = 0; i < tamano; i++, px++)
    {
        linea_completa = 1;
        j = 0;
        while(linea_completa && j < TABLERO_W)
        {
            if(!tablero[px][j]) linea_completa = 0;
            j++;
        }
        if(linea_completa)
        {           
            recorre_filas_abajo(px);
            top_x = top_x + 1;
        }
    }
}

void recorre_filas_abajo(int fila)
{
    int col, aux = 0;
    
    for(; fila >= top_x; fila--, aux++)
        for(col = 0; col < TABLERO_W; col++)
        {
            tablero[fila][col] = (fila > 0 || fila > top_x ? tablero[fila-1][col] : 0);
            led_color[fila][col] = (fila > 0 || fila > top_x ? led_color[fila-1][col] : 0);
        }
}

void verifica_movimiento(int direccion)
{
    int px = pos_x, py = pos_y;
    // detecta_colision(px, py)
    if(!detecta_colision(direccion == M_ABAJO ? px + 1 : px, direccion == M_IZQ ? py - 1: (direccion == M_DER ? py + 1 : py)))
    {
        if(direccion == M_IZQ || direccion == M_DER)
            pos_y = (direccion == M_IZQ ? py - 1 : py + 1);
        
        if(direccion == M_ABAJO)
            pos_x = px + 1; // o pox_x++;

        imprime_tablero(); // ¿?
        imprime_figura();
    }
    else if(direccion == M_ABAJO) pieza_colocada = 1;
}

int detecta_colision(int px, int py)
{
    int i = 0, j, y;
    int colision = 0;
    
    while(!colision && i < tamano)
    {
        y = py; j = 0;
        while(!colision && j < tamano)
        {
            if(tipo_forma == F_CUADRADO) colision = (figura_2x2[i][j] && tablero[px][y]);
            else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) colision = (figura_3x3[i][j] && tablero[px][y]);
            else colision = (figura_4x4[i][j] && tablero[px][y]);
            j++; y++;
        }
        i++; px++;
    }
    return colision;
}

void coloca_figura_en_tablero()
{
    int i, j, py, px;
    
    for(i = 0, px = pos_x; i < tamano; i++, px++)
        for(j = 0, py = pos_y; j < tamano; j++, py++)
        {
            if(tipo_forma == F_CUADRADO) {
                if(figura_2x2[i][j]) {
                    tablero[px][py] = figura_2x2[i][j];
                    led_color[px][py] = pza_color;
                }
            }
            else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) {
                if(figura_3x3[i][j]) {
                    tablero[px][py] = figura_3x3[i][j];
                    led_color[px][py] = pza_color;
                }
            }
            else if(figura_4x4[i][j]) {
                tablero[px][py] = figura_4x4[i][j];
                led_color[px][py] = pza_color;
            }
        }
}

int rotar_figura()
{
    int i, j, k, colision = 0, px = pos_x, py;
    int aux[tamano][tamano]; // Matriz auxiliar para guardar la rotacion.

    for(i = 0, k = tamano - 1; i < tamano, k >= 0; i++, k--)
        for(j = 0; j < tamano; j++)
        {
            if(tipo_forma == F_CUADRADO) aux[j][k] = figura_2x2[i][j]; // Esta no es necesaria a menos que se agregen mas figuras de 2x2.
            else if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) aux[j][k] = figura_3x3[i][j];
            else aux[j][k] = figura_4x4[i][j];
        }
    // Se verfica que al momento de querer girar la pieza no haya colision.
    i = 0;
    while(!colision && i < tamano) {
        py = pos_y; j = 0;
        while(!colision && j < tamano)
        {
            if(aux[i][j] && tablero[px][py]) colision = 1; 
            j++; py++;
        }
        i++; px++;
    }
    // Si no hay colision gira la pieza y se debe imprimir en la pantalla (en el ciclo main)
    if(!colision)
        for(i = 0; i < tamano; i++)
            for(j = 0; j < tamano; j++)
            {
                if(tipo_forma == F_CUADRADO) figura_2x2[i][j] = aux[i][j]; // Esta reasignación no es necesaria.
                else if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) figura_3x3[i][j] = aux[i][j];
                else figura_4x4[i][j] = aux[i][j];
            }   
    return colision;
}

void imprime_tablero()
{
    int fila, col;
    int8_t color_index; // Esta variable no es tan necesaria.

    tiraled.clear();
    for(fila = 0; fila < TABLERO_H; fila++)
        for(col = 0; col < TABLERO_W; col++)
            if(tablero[fila][col])
            {
                color_index = led_color[fila][col];
                tiraled.setPixelColor(fila*TABLERO_H+col, colores[color_index]);
            }
    tiraled.show();
    //delay(20);
}

void imprime_figura()
{
    int i, j, py, px = pos_x;

    for(i = 0; i < tamano; i++, px++)
    {       
        py = pos_y;
        for (j = 0; j < tamano; j++, py++)
        {
            if(tipo_forma == F_CUADRADO){ // Esto así puede quedar en un solo if con ?.
                if(figura_2x2[i][j]) tiraled.setPixelColor(px*TABLERO_H+py, colores[pza_color]);
            }
            else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) {
                if(figura_3x3[i][j]) tiraled.setPixelColor(px*TABLERO_H+py, colores[pza_color]);
            }
            else if(figura_4x4[i][j]) tiraled.setPixelColor(px*TABLERO_H+py, colores[pza_color]);
        }
    }
    tiraled.show(); // Se actualiza todo.
    delay(20);
}


void inicializa_tablero()
{
    int fila, col;
    
    for(fila = 0; fila < TABLERO_H; fila++)
        for(col = 0; col < TABLERO_W; col++)
        {
            tablero[fila][col] = 0;
            led_color[fila][col] = 0;
        }
}

void asigna_figura()
{
    int i, j;
    
    for(i = 0; i < tamano; i++)
        for (j = 0; j < tamano; j++)
        {   
            if(tipo_forma == F_CUADRADO) figura_2x2[i][j] = formas_2x2[i][j];
            else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) figura_3x3[i][j] = formas_3x3[tipo_forma-1][i][j];
            else figura_4x4[i][j] = formas_4x4[i][j];
        }
}

void selecciona_figura()
{
    tipo_forma = random(0, 7);
    pza_color = random(0, 6);
    
    if(tipo_forma == F_CUADRADO) tamano = 2;
    else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) tamano = 3;
    else tamano = 4;
}
