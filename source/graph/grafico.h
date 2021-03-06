#ifndef _GRAFICOS_H__
#define _GRAFICOS_H__

#include "../h/types.h"
#include "../bgi/graphics.h"
//defini��es das  propriedades de tela
#define SCREEN_W 1280
#define SCREEN_H 600
#define SCREEN_CENTER SCREEN_W/2
#define PLAYER_SPOT SCREEN_W/3
#define BGCOLOR COLOR(131,169,196)
#define BAR_MAX_HEIGHT 200
#define BAR_WIDTH 20
#define BAR_UNIT 1

//defini��es de IDs pros objetos gr�ficos utilizados para definir o que deve ser desenhado no initobj
//pode ser �til tamb�m na hora de c�lcular as posi��es dos �tens
	
// Prot�tipo das fun��es ------------------------------
void updateScreen();
void print(vetor2d_type pos, graph_data_type *obj);
void erase();
void graphInitObjects(graph_data_type *objeto, const char* caminho, const char* caminhomsk="");
void printDirection(vetor2d_type pos,float angulo, float forca);
void printTxt(char *texto, vetor2d_type pos);
void fontSize(int char_size=1);
void drawProgressBar(float value, vetor2d_type pos);

#endif
