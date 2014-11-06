#include "grafico.h"
#include "../bgi/graphics.h"
#include<iostream>


#define TO_GROUND_LEVEL(y) ((SCREEN_H-10)- y)

using namespace std;

void print(vetor2d_type pos, graph_data_type *obj){
	float ref_y = pos.y + obj->h;
	putimage(pos.x, TO_GROUND_LEVEL(ref_y), obj->img, COPY_PUT);
	
}

void erase(){
	cleardevice();
	setbkcolor(BGCOLOR);
	
}




void graphInitObjects(graph_data_type *objeto, const char* caminho){//puxar arquivo com caminho e tamanhos das imagens 
	unsigned size;
	int left, top, right, bottom;
	setbkcolor(BGCOLOR);
	cleardevice();
	left = (SCREEN_W/2)-(objeto->w/2);
	top = (SCREEN_H/2)-(objeto->h/2);
	right = (SCREEN_W/2)+(objeto->w/2);
	bottom = (SCREEN_H/2)+(objeto->h/2);	
	readimagefile(caminho,left, top, right, bottom );
	
	size= imagesize(left, top,right,bottom);
	objeto->img = new int[size];
	getimage(left, top, right, bottom, objeto->img);
	
	cleardevice();
	
	
}

void updateScreen(){
	int page = getactivepage();
	setvisualpage(page);
	
	page = page ? 0:1;
	setactivepage(page);
	erase();
}
