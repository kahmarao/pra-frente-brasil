/**
*  @file game.cpp
*  @brief Arquivo cont�ndo toda a l�gica do game e interface com a BGI
*  
*  @author
*  @version 0.1
*  @date 10/10/14
*/ 

#include "game.h"
#include "graph/grafico.h"
#include "physics/physics.h"
#include "socket/socket.h"
#include <stdlib.h>
#include <conio.h>
#include <iostream>
#include <string.h>
#include <fstream>

// Defines =====================================================
#define MAX_OBSTACLES 150 /**< N�mero m�ximo de obst�culos que ser�o gerados por partida.*/
#define PLAYER_FIX_POS PLAYER_SPOT /**< Posi��o m�xima que o player avan�ar� em rela��o a tela*/
#define PLAYER_INIT_X 10 /**< Posi��o incial do player em x*/
#define PLAYER_INIT_Y 0  /**< Posi��o incial do player em y*/
#define RESOURCES_ROOT "../resources/data/img_data.cvs"
#define SERVER_CONF_PATH "../resources/data/server.inf"

#define ATRITO 50
#define BOUNCE 0.5
#define SPEED_LIM_X 2000
#define SPEED_LIM_Y 2000
#define MAX_BONUS 0.2

#define POWER_UP_UNITS 1

// typedefs privados ===========================================
typedef struct{
	char *file_name;
	int heigth;
	int width;
}init_file_type;


// Prot�tipos Privados ==========================================
int initGame (float dt);
int initMenu (float dt);
int showMenu (float dt);
int showLoja (float dt);
int initServer (float dt);
int serverSendObstacles (float dt);
int initClient (float dt);
int clientGetObstacles (float dt);
int loadSingleGame (float dt);
int setLaunchVector (game_object_type &ref);
int preLancamento (float dt);
int preLancamentoMult (float dt);
int singleStep (float dt);
int multiStep (float dt);
void floorCheck(game_object_type *player);
void groundStep(game_object_type *objeto, game_object_type *ground, float dt);
int multiEnd(float dt);
int singleEnd(float dt);
int showCredits (float dt);
float variaForca(float valor);
int initLoja (float dt);
void resetGame();
void resetLoja();
void exibirSeta();
void mudarVelocidade(vetor2d_type *speed);
void printP2(game_object_type &ref, game_object_type &p2);

//Vari�veis privadas ============================================
game_object_type player1,player2, ground;
game_object_type green_aura, red_aura;
game_object_type loja_options[NUM_LOJA_MENU];
game_object_type menu_options[NUM_OPTIONS_MENU];
game_object_type world_obstacles[MAX_OBSTACLES];
graph_data_type graphs_profiles[NUM_OBJECTS_DEFINE];
unsigned int left_obstacles_index = 0,right_obstacles_index = 0;
float  ground_offset;
float obstacles_weight[NUM_BLOCKS],max_obstacles_per_type[NUM_BLOCKS]; 
int total_obstacles = 0;
int total_score = 0; 
int total_rounds = 3;
float profile_collision_bonus[NUM_BLOCKS];

////////////////////////// Declara��o dos estados da m�quina do game ////////////////

//Primeiro estado do jogo. Carrga as inf do resources e chama a inicializa��o do menu
game_state_type load_state ={
	initGame,
	(game_state_type*[]){
		&load_state,		 // return 0
		&load_menu_state	 // return 1
	}
};
// Reseta o menu para o estado incial
game_state_type load_menu_state ={
	initMenu,
	(game_state_type*[]){
		&load_menu_state, // return 0
		&menu_state       // return 1
	}
};

// Menu principal
game_state_type menu_state ={
	showMenu,
	(game_state_type*[]){
		&menu_state,		 // return 0
		&load_single, 	 // return 1
		&client_conect,		 // return 2	
		&server_conect,		 // return 3
		&termina_programa // return 4
	}
};

// Reseta as vari�veis para o in�cio do jogo
game_state_type load_single ={
	loadSingleGame,
	(game_state_type*[]){
		&load_single,	 // return 0
		&pre_lancamento  // return 1
	}
};

// Loop de escolha do angulo e for�a do lan�amento
game_state_type pre_lancamento ={
	preLancamento,
	(game_state_type*[]){
		&pre_lancamento,	// return 0
		&step_single 		// return 1
	},
};

// Loop de "loja" com as propostas que ser�o escolidas pelo jogador
game_state_type loja_state ={
	showLoja,
	(game_state_type*[]){
		&loja_state,	 // return 0
		&load_single	 // return 1
	}
};

// Inicializa��o da loja
game_state_type load_loja ={
	initLoja,
	(game_state_type*[]){
		&load_loja,	 // return 0
		&loja_state	 // return 1
	}
};

// Finaliza��o das rodadas do singleplayer
game_state_type end_single ={
	singleEnd,
	(game_state_type*[]){
		&end_single,	 // return 0
		&load_loja,	 // return 1
		&load_menu_state // return 2
	}
};


// Loop da rodada (inicio do lan�amento at� speed = 0) para single player
game_state_type step_single ={
	singleStep,
	(game_state_type*[]){
		&step_single,	 // return 0
		&end_single     // return 1
	}
};

// Tela final do jogo quando ecolhido a op��o de menu de finaliza��o
game_state_type termina_programa ={
	showCredits,
	(game_state_type*[]){
		&termina_programa,
		&load_menu_state
	}
};

// Loop de espera de conex�o do servidor
game_state_type server_conect ={
	initServer,
	(game_state_type*[]){
		&server_conect,
		&server_obstacles
	}
};

// Loop de requisi��o de conex�o do client
game_state_type client_conect ={
	initClient,
	(game_state_type*[]){
		&client_conect,
		&client_obstacles
	}
};

// Loop de inicializa��o dos obst�culos do servidor para o client
game_state_type server_obstacles ={
	serverSendObstacles,
	(game_state_type*[]){
		&server_obstacles,
		&server_prelancamento
	}
};

// Loop de requisi��o dos obst�culos do client ao server
game_state_type client_obstacles ={
	clientGetObstacles,
	(game_state_type*[]){
		&client_obstacles,
		&client_prelancamento
	}
};

// Pr� lan�amento para multiplayer do server
game_state_type server_prelancamento ={
	preLancamentoMult,
	(game_state_type*[]){
		&server_prelancamento,
		&step_server
	}
};

// Pr� lan�amento para multiplayer do client
game_state_type client_prelancamento ={
	preLancamentoMult,
	(game_state_type*[]){
		&client_prelancamento,
		&step_client
	}
};
// Loop da rodada do server
game_state_type step_server ={
	multiStep,
	(game_state_type*[]){
		&step_server,	 // return 0
		&end_server     // return 1
	}
};

// Loop da rodada do client
game_state_type step_client ={
	multiStep,
	(game_state_type*[]){
		&step_client,	 // return 0
		&end_client     // return 1
	}
};

// Fim de rodada do server
game_state_type end_server ={
	multiEnd,
	(game_state_type*[]){
		&end_server,	 // return 0
		&load_loja_server,	 // return 1
		&load_menu_state // return 2
	}
};

// Fim de rodada do client
game_state_type end_client ={
	multiEnd,
	(game_state_type*[]){
		&end_client,	 // return 0
		&load_loja_client,	 // return 1
		&load_menu_state // return 2
	}
};

// Inicializa��o da loja do server
game_state_type load_loja_server ={
	initLoja,
	(game_state_type*[]){
		&load_loja_server,	 // return 0
		&loja_server	 // return 1
	}
};

// Inicializa��o da loja do client
game_state_type load_loja_client ={
	initLoja,
	(game_state_type*[]){
		&load_loja_client,	 // return 0
		&loja_client	 // return 1
	}
};

// Loop da loja do server
game_state_type loja_server ={
	showLoja,
	(game_state_type*[]){
		&loja_server,	 // return 0
		&server_obstacles	 // return 1
	}
};

// Loop da loja do client
game_state_type loja_client ={
	showLoja,
	(game_state_type*[]){
		&loja_client,	 // return 0
		&client_obstacles	 // return 1
	}
};

/**
 *  Ponteiro de trabalho da m�quina de estado. Ser� ele que apontar� para cada estado
 */
game_state_type *game_states =  &load_state;

static void debugTrace (char *msg){
#ifdef ON_DEBUG
	std::cout << "debugTrace: " << msg << std::endl;
#endif	
};

/**
 *  @brief Inicia os recursos que ser�o utilzados no game
 *  
 *  @param dt
 */ 
int initGame (float dt){
	
	// FAZER LOAD DO ARQUIVO COM OS NOMES DAS IMAGENS ==========
	using namespace std;	
	int f_index =0;
	char data[8000];
	
	{// Truque para deixar o tempo de vida do file m�nima
		ifstream file(RESOURCES_ROOT);
		while(file.good()){
			data[f_index] = file.get();
			f_index++;
		}
		file.close();
	}
	data[f_index] = '\0';
	
	char * pch;
		
	pch = strtok(data,",");//carrega o primeiro booleano
	
	for(int i = 0; i <NUM_OBJECTS_DEFINE; ++i){
		static char temp[300],tempmsk[300], t_width[4], t_height[5];
		

		
		//carrega valor da largura
		strcpy(t_width,pch);
		graphs_profiles[i].w = atoi(t_width);
	

		//carrega valor da altura
		pch = strtok(NULL,",");
		strcpy(t_height,pch);
		graphs_profiles[i].h = atoi(t_height);
		
		//carrega caminho da mascara

		pch = strtok(NULL,",");
		strcpy(tempmsk,pch);	
	

		pch = strtok(NULL,"\n\0");
		strcpy(temp,pch);
		
		pch = strtok(NULL,",");
		
		// Carrega e captura a imagem na mem�ria
		graphInitObjects(&graphs_profiles[i], temp,tempmsk);
	}
	// Inicializa o gr�fico do player1
	player1.graph = graphs_profiles[PLAYER1];
	
	
	//inicializa o fator inicial de bonus
	for(int i = 0; i < NUM_BLOCKS;++i)	profile_collision_bonus[i] = 0.0;
	

	
	// Inicializa o gr�fico do player2
	player2.graph = graphs_profiles[PLAYER2];
	
	// Parametros do ground
	ground.graph = graphs_profiles[GROUND];
	ground.body.pos.x = 0;
	ground.body.pos.y = -10 ;
	ground.body.speed.x = 0;
	ground.body.speed.y = 0;
	
	// Itens do menu
	for(int i = 0; i < NUM_OPTIONS_MENU; ++i){
		menu_options[i].graph = graphs_profiles[MENU_OPTION_1 + i];
		
		vetor2d_type menu_pos{(SCREEN_W - menu_options[i].graph.w)/2,(SCREEN_H/(NUM_OPTIONS_MENU +3)) * ( NUM_OPTIONS_MENU -1 - i)};
		menu_options[i].body.pos =	menu_pos;
	}
	
	resetGame();


	
	// Feedback visuais de colis�o
	green_aura.graph = graphs_profiles[GREEN_AURA];
	red_aura.graph = graphs_profiles[RED_AURA];
	
	//carrega lista de pesos dos objetos
	for(int i = 0; i < NUM_BLOCKS; i++) obstacles_weight[i] = 1;
	
	f_index =0;
	{// Truque para deixar o tempo de vida do file m�nima
		ifstream file(SERVER_CONF_PATH);
		while(file.good()){
			data[f_index] = file.get();
			f_index++;
		}
		file.close();
	}
	data[f_index] = '\0';
	
	char *ip = strtok(data,":");
	pch = strtok(NULL,":");
	short port = strtoul(pch,0,10);
	
	setServerConfig(ip,port);
	
	
	return 1;
}

/**
 *  @brief Exibe cr�ditos do jogo
 *  
 *  @param [in] dt n�o faz nada aqui
 *  @return -1 para interromper o loop da m�quina de estados
 */
int showCredits (float dt){
	setbkcolor(COLOR(222,219,190));
	print(vetor2d_type{0,-10},&graphs_profiles[CREDITOS]);
	setcolor(COLOR(85,63,18));
	fontSize(6);
	printTxt("Obrigado por jogar!", vetor2d_type{100, SCREEN_H/2-(textheight("Obrigado por jogar!")+40)});
	fontSize(4);
	printTxt("Anderson Ara�jo", vetor2d_type{150, SCREEN_H/2-(textheight("Anderson Ara�jo")-30)});
	printTxt("Carol Fernandes", vetor2d_type{150, SCREEN_H/2-(textheight("Carol Fernandes")-70)});
	printTxt("Diego Hortiz", vetor2d_type{150, SCREEN_H/2-(textheight("Diego Hortiz")-110)});
	printTxt("Lucas Pina", vetor2d_type{150, SCREEN_H/2-(textheight("Lucas Pina")-150)});
	printTxt("Marcelo Pietragala", vetor2d_type{150, SCREEN_H/2-(textheight("Marcelo Pietragala")-190)});
	
	if(kbhit())
		return -1;
		
	return 0;
}

/**
 *  @brief Limpa a mem�ria heap
 */
void endGame (void){
	
	for(int i = 0; i <NUM_OBJECTS_DEFINE; ++i){
		delete graphs_profiles[i].img;
	}
}

/**
 *  @brief Faz  inciliza��o do menu
 *  
 *  @param [in] dt N/A
 *  @return Sempre retorna 1
 */
int initMenu (float dt){
	
	if(kbhit()){
		getch();
		fflush(stdin);
	}
	// Pura gra�a -------------------------------
	player1.body.pos.y = (SCREEN_H*3)/4;
	player1.body.pos.x = (SCREEN_W/6);
	player2.body.pos.setVector((SCREEN_W*5)/6,0);
	player2.body.speed.setVector(750,90);
	// ------------------------------------------
	return 1;
}

/**
 *  @brief Brief
 *  
 *  @param [in] dt delta t para a paresenta��o das anima��es
 *  @return 0-> Continua o loop, de 1 a 4 a op��o escolhida no menu
 */
int showMenu (float dt){
	
	// Pura gra�a -------------------------------
	lancamento(&player1,dt);
	lancamento(&player2,dt);
	// Verifica se atingiu o ch�o
	if(player1.body.pos.y<=0){
		player1.body.pos.y=0;
		player1.body.speed.y *= -0.99;
	}
	// Verifica se atingiu o ch�o
	if(player2.body.pos.y<=0){
		player2.body.pos.y=0;
		player2.body.speed.y *= -0.99;
	}
	print(vetor2d_type{0,-10},&graphs_profiles[LOGOTIPO]);
	print(player1.body.pos,&player1.graph);
	print(player2.body.pos,&player2.graph);
	// -------------------------------------------
	
	
	for(int i = 0; i < NUM_OPTIONS_MENU; ++i)
		print(menu_options[i].body.pos,&menu_options[i].graph);

	for(int i = 0; i < NUM_OPTIONS_MENU; ++i){
		if(GetKeyState(VK_LBUTTON)&0x80){ //clique do mouse
			POINT mouse_pos;
			if (GetCursorPos(&mouse_pos)){ //retorna uma estrutura que cont�m a posi��o atual do cursor
				char debug[50];
								
				HWND hwnd = GetForegroundWindow(); 
				if (ScreenToClient(hwnd, &mouse_pos)){
					
					// Passa o y para as coordenadas de nossos objetos
					mouse_pos.y = SCREEN_H - mouse_pos.y;
					// Verifica os limites de x
					if((mouse_pos.x < (int)menu_options[i].bottomLeft().x) || (mouse_pos.x > (int)(menu_options[i].topRight().x))) continue;
					// Verifica os limites de y
					if((mouse_pos.y < (int)menu_options[i].bottomLeft().y) || (mouse_pos.y > (int)menu_options[i].topRight().y)) continue;
					// Retorna a op��o do menu			
					return i + 1;
				}
			}
		}
	}	
	return 0;
}

/**
 *  @brief Iniciliza as vari�veis da loja
 *  
 *  @param [in] dt N/A
 *  @return Sempre 1
 */
int initLoja (float dt){
	
	
	
	for(int i = 0; i < NUM_BLOCKS;++i)	profile_collision_bonus[i] = 0.0;// reseta o bonus
	
				

	
	if(kbhit()){
		getch();
		fflush(stdin);
	}

	return 1;
}

/**
*  *brief Exibe e gerencia a escolha do player das propostas de governo
*  
*  *param [in] dt delta tempo usado para as anima��es
*  *return 0->loop, 1-> pr�ximo estado
*/
int showLoja (float dt){
	
	static int selected = 0;
	int i, a=0;
	char texto[50];
	static game_object_type *moving_object = 0;
	static game_object_type *moving_pairobject = 0;
	const int pairpolitics[] = {1,0,3,2,5,4,7,6,9,8,11,10,13,12,-1,-1,-1,-1,-1,-1,-1,-1};
	const float bonus[NUM_LOJA_MENU-1][NUM_BLOCKS] = {	{0.0,	1.0,	0.0,	0.5,	0.0,	-1.0,	0.0,	0.5,	0.0,	0.0,	0.0,	-1.0,	0.0,	0.0,	0.0,	0.0},
														{0.0,	-1.0,	0.0,	0.0,	0.0,	0.5,	0.0,	0.0,	0.0,	0.0,	0.0,	1.0,	0.0,	0.0,	0.0,	0.0},
														{0.0,	0.0,	0.0,	0.5,	0.5,	0.0,	0.0,	-1.0,	0.0,	-1.0,	0.0,	0.0,	0.0,	0.5,	0.0,	1.0},
														{0.0,	0.0,	0.0,	-0.5,	2.0,	0.0,	0.0,	1.0,	0.0,	1.0,	0.0,	0.0,	0.0,	-1.0,	0.0,	-1.0},
														{-0.5,	-0.5,	-0.5,	0.0,	0.0,	0.0,	-0.5,	1.0,	1.0,	0.0,	1.0,	0.0,	0.0,	-0.5,	0.0,	-0.5},
														{0.5,	0.5,	0.5,	0.0,	0.0,	0.0,	0.5,	-1.0,	-1.0,	0.0,	-1.0,	0.0,	0.0,	0.5,	0.0,	0.5},
														{0.5,	0.5,	0.0,	0.0,	0.0,	0.0,	0.5,	-0.5,	0.0,	0.0,	0.0,	0.0,	-0.5,	0.5,	1.0,	0.5},
														{-0.5,	-0.5,	0.0,	0.0,	0.0,	0.0,	-0.5,	0.5,	0.0,	0.0,	0.0,	0.0,	0.5,	-0.5,	-1.0,	-0.5},
														{0.5,	-0.5,	0.0,	0.0,	0.5,	0.0,	0.5,	0.0,	0.0,	0.0,	0.0,	0.0,	-0.5,	0.5,	1.0,	0.0},
														{0.0,	-1.0,	0.0,	0.0,	0.0,	0.5,	0.0,	0.0,	0.0,	0.0,	0.0,	1.0,	0.0,	0.0,	-1.0,	0.0},
														{1.0,	1.0,	-1.0,	0.5,	0.0,	0.0,	1.0,	-0.5,	0.5,	0.0,	0.0,	0.0,	0.0,	1.0,	0.5,	1.0},
														{-1.0,	-1.0,	-1.0,	-0.5,	0.0,	0.0,	-1.0,	0.5,	-0.5,	0.0,	0.0,	0.0,	0.0,	-1.0,	-0.5,	-1.0},
														{0.0,	0.5,	0.5,	0.0,	1.5,	0.0,	-0.5,	-1.0,	1.0,	1.0,	-1.0,	0.0,	0.0,	-0.5,	0.0,	0.0},
														{0.0,	-0.5,	0.5,	0.0,	0.0,	0.0,	0.5,	1.0,	-1.0,	-1.0,	1.0,	0.0,	0.0,	0.0,	0.0,	0.0},
														{0.5,	0.5,	-1.0,	0.0,	0.0,	0.0,	0.5,	-0.5,	0.0,	0.0,	0.0,	0.0,	-0.5,	0.5,	1.0,	0.5},
														{-0.5,	-0.5,	0.0,	0.0,	0.0,	0.0,	-0.5,	1.0,	-1.0,	0.5,	1.0,	0.0,	0.0,	-0.5,	0.0,	-0.5},
														{0.0,	0.0,	0.0,	1.0,	0.5,	0.0,	0.0,	0.0,	0.0,	-1.0,	0.0,	0.0,	0.0,	1.0,	0.0,	0.0},
														{0.5,	0.5,	-0.5,	0.0,	0.0,	0.0,	0.5,	0.0,	0.0,	0.0,	0.0,	0.0,	-0.5,	0.5,	1.0,	0.5},
														{1.0,	0.0,	0.0,	1.0,	0.0,	-1.0,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0},
														{1.0,	0.5,	-1.0,	0.0,	0.0,	-0.5,	1.0,	0.0,	0.0,	0.0,	-1.0,	0.0,	-1.0,	0.0,	0.5,	0.5},
														{0.5,	0.0,	0.0,	0.0,	0.0,	-1.0,	1.0,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0},
														{1.0,	1.0,	-1.0,	0.0,	0.0,	0.0,	1.0,	-1.0,	0.0,	0.0,	-1.0,	0.0,	-0.5,	1.0,	1.0,	1.0}
														};
												
												
	for(i=0;i < NUM_LOJA_MENU;i++)
        print(loja_options[i].body.pos,&loja_options[i].graph);

	if(moving_pairobject){
		moving_pairobject->body.pos.x -= 25;
		
		if( moving_pairobject->body.pos.x< (0 - moving_pairobject->graph.w)) moving_pairobject=0;

	}
	if(moving_object){
		moving_object->body.pos.x -= 25;
		
		if( moving_object->body.pos.x< (0 - moving_object->graph.w)) moving_object=0;
		
		return 0;
	}

	for(int i = 0; i <= NUM_LOJA_MENU; ++i){
		if(GetKeyState(VK_LBUTTON)&0x80){ //clique do mouse
			POINT mouse_pos;
			
			if (GetCursorPos(&mouse_pos)){ //retorna uma estrutura que cont�m a posi��o atual do cursor
				char debug[50];
				HWND hwnd = GetForegroundWindow(); 
				if (ScreenToClient(hwnd, &mouse_pos)){
					
					// Passa o y para as coordenadas de nossos objetos
					//mouse_pos.y = SCREEN_H - mouse_pos.y;
					mouse_pos.y = SCREEN_H - mouse_pos.y;
					// Verifica os limites de x
					if((mouse_pos.x < (int)loja_options[i].bottomLeft().x) || (mouse_pos.x > (int)loja_options[i].topRight().x)) continue;
					// Verifica os limites de y
					if((mouse_pos.y < (int)loja_options[i].bottomLeft().y) || (mouse_pos.y > (int)loja_options[i].topRight().y)) continue;
					// Retorna a op��o do menu			
					if(i == LOJA_OK - LOJA_OPTION_1) {
						selected = 0;
						return 1;
					}
					if(i == LOJA_RESET - LOJA_OPTION_1) {
						resetLoja();
						selected = 0;
						return 0;
					}
					if(selected <2){
						++selected;					
						moving_object = &loja_options[i];
						if(pairpolitics[i]>=0) moving_pairobject = &loja_options[pairpolitics[i]];
						//carrega em profile_collision_bonus os valores selecionados
						for(int u = 0; u <NUM_BLOCKS ; ++u){
							profile_collision_bonus[u] = profile_collision_bonus[u]+bonus[i][u]*MAX_BONUS;
							if(u == NUVEM_POLUICAO) profile_collision_bonus[u] = -1* MAX_BONUS;
							
							obstacles_weight[u] = obstacles_weight[u]-(bonus[i][u]/4);
							
							
						}	
					}
				
					
					
				}
			}
		}
	}

	return 0;
} 

/**
 *  @brief Cria de forma rand�mica os obst�culos na tela respeitando os limites de cada um
 */
void initObstacles (void){
	//carrega lista de n�meros m�ximos de cada objeto
	for(int i = 0; i < NUM_BLOCKS; i++){
		
		if(i == CONGRESSO)
			max_obstacles_per_type[i] =1;
		else
	 		max_obstacles_per_type[i] = (MAX_OBSTACLES/NUM_BLOCKS) *obstacles_weight[i];
	 		
		total_obstacles+=max_obstacles_per_type[i];
	}
	
	//garante que o n�mero de obst�culos seja sempre MAX_OBSTACLES
	int z = 0;
	while(total_obstacles<MAX_OBSTACLES){
		if(z!=CONGRESSO ){
			max_obstacles_per_type[z]++;
			total_obstacles++;
			
		}
		z++;
		if(z>=NUM_BLOCKS)	 z=0;
	}
	

	//	
	int obstacles_defined = 0;
	while(obstacles_defined < MAX_OBSTACLES){

		// Randomiza qual o perfil (tipo) de obst�culo ele ser�, Igreja, nuvem....
		unsigned int obj_profile = (rand() % NUM_BLOCKS);
		
		//checa se o profile selecionado j� alcan�ou o n�mero limite previsto 
		//se sim, sorteia novamente
		//se n�o, segue com o procedimento de carregar ele na lista
		if(max_obstacles_per_type[obj_profile]>0){
		
				world_obstacles[obstacles_defined].profile = obj_profile;
				world_obstacles[obstacles_defined].graph = graphs_profiles[obj_profile]; // Aponta para qual o bitmap que pertence aquele perfil
				
				//partindo da posi��o minima para deixar fora da tela, a posi��o de cada obstaculo varia 100px,
				//a partir do primeiro objeto colocado a posicao minima vira a posi��o do objeto anterior
				//e a ele � somado a largura desse mesmo objeto objeto anterior, de forma a evitar a sobreposi��o
				world_obstacles[obstacles_defined].body.pos.x = ((rand() % 600) + 50) + ((obstacles_defined != 0) ? world_obstacles[obstacles_defined-1].graph.w + world_obstacles[obstacles_defined-1].body.pos.x: SCREEN_W - 50);
				
				if(obj_profile == NUVEM_POLUICAO || obj_profile== EUA)
					world_obstacles[obstacles_defined].body.pos.y = ((rand() % 300) + 50);
				else
					world_obstacles[obstacles_defined].body.pos.y = 0;
				
				max_obstacles_per_type[obj_profile]--;
				++obstacles_defined;
		}
	}
	

}

/**
 *  @brief Inicializa��o do servidor
 *  
 *  @param [in] dt N/A
 * 
 *  @return 1 conex�o client server estabelecida,0 aguardando client (sem conex�o)
 */
int initServer (float dt){
	initSocket();
	
	char texto[] = "Waiting for client";
	setcolor(COLOR(255,255,255));
	fontSize(2);
	printTxt(texto, vetor2d_type{(SCREEN_W/2)-(textwidth(texto)/2), SCREEN_H/2});
	
	
	if(!waitClient()) return 0;
	
	return 1;
}

/**
 *  @brief envia informa��es de obst�culos ao client quando requisitado
 *  
 *  @param [in] dt N/A
 *
 *  @return 1 0->Loop, 1->Pr�ximo estado
 */
int serverSendObstacles (float dt){

	static bool new_round = true;
	static int count = 0;
	if(new_round){
		initObstacles();
		new_round = false;
	}
	
	char texto[] = "Loading Map";
	setcolor(COLOR(255,255,255));
	fontSize(2);
	printTxt(texto, vetor2d_type{(SCREEN_W/2)-(textwidth(texto)/2), SCREEN_H/2});
	
	setcolor(COLOR(255,255,255));
	rectangle((SCREEN_W/2)-150,(SCREEN_H/2)+30,(SCREEN_W/2-150)+300,(SCREEN_H/2)+40);
	bar((SCREEN_W/2)-150,(SCREEN_H/2)+30,(SCREEN_W/2-150)+count,(SCREEN_H/2)+40);
	
	packet_type resp;
	if(getPacket(resp) > 0){
		if(resp.ctrl. operation == OBSTACLE_UPDATE){
			count++;
			union{
				short i_16;
				char i_8[2];
			};
			
			// Converte o 8 bits em 16. (altamente inseguro devido os diferentes endiands entre m?quinas
			i_8[0] = resp.buff[0];
			i_8[1] = resp.buff[1];
		
			// Preenche as informa??es sobre o obstaculo
			gam_obj_pack_type obst{
					world_obstacles[i_16].profile,
					world_obstacles[i_16].body.pos.x,
					world_obstacles[i_16].body.pos.y,
					0
			};
			// Prepara o packed de resposta
			packet_type resp{
				{
					0,
					(sizeof(gam_obj_pack_type)+2),
					OBSTACLE_UPDATE
				},
				{}
			};
			
			memcpy(&resp.buff[2],&obst,sizeof(gam_obj_pack_type));
			resp.buff[0] = i_8[0];
			resp.buff[1] = i_8[1];
			sendPacket(resp);	
		}
		else if(resp.ctrl. operation == WAINTING_GAME){
			player1.body.pos.x = PLAYER_INIT_X;
			player1.body.pos.y = PLAYER_INIT_Y;
			
			player2.body.pos.x = PLAYER_INIT_X;
			player2.body.pos.y = PLAYER_INIT_Y;
			
			count = 0;
	
			return 1;
		}
	}
	return 0;
}

/**
 *  @brief Loop de requisi��o de conex�o com o client
 *  
 *  @param [in] dt delta t para c�lculo do tempo de retry das requisi��es
 *
 *  @return 0-> Loop, 1->Pr�ximo estado
 */
int initClient (float dt){
	static float retry_time =0.0;
	initSocket();
		
	retry_time -= dt;
	if(retry_time <= 0){
		if(connectToServer()){
			debugTrace("Conected");
			return 1;
		}
		retry_time = 0.5;
	}
	
/*	char texto[] = "Searching Server";
	printTxt(texto, vetor2d_type{(SCREEN_W/2)-(textwidth(texto)/2), SCREEN_H/2});
	*/
	
	return 0;
}

/**
 *  @brief Requesita ao server os obst�culos do mapa
 *  
 *  @param [in] dt delta t para c�lculo do tempo de retry das requisi��es
 *
 *  @return 0-> Loop, 1->Pr�ximo estado
 */
int clientGetObstacles (float dt){
	static bool new_round = true;
	static int count = 0;
	
	if(new_round){
		debugTrace("clientGetObstacles: new_round");
		
		memset(world_obstacles,0,sizeof(game_object_type) * MAX_OBSTACLES);
		new_round = false;
	}
	
	char texto[] = "Loading Map";
	setcolor(COLOR(255,255,255));
	fontSize(2);
	printTxt(texto, vetor2d_type{(SCREEN_W/2)-(textwidth(texto)/2), SCREEN_H/2});
	
	setcolor(COLOR(255,255,255));
	rectangle((SCREEN_W/2)-150,(SCREEN_H/2)+30,(SCREEN_W/2-150)+300,(SCREEN_H/2)+40);
	bar((SCREEN_W/2)-150,(SCREEN_H/2)+30,(SCREEN_W/2-150)+count,(SCREEN_H/2)+40);
	
	packet_type resp;
	if(getPacket(resp) > 0){
		if(resp.ctrl. operation == OBSTACLE_UPDATE){
		
			union{
				short i_16;
				char i_8[2];
			};
			
			// Converte o 8 bits em 16. (altamente inseguro devido os diferentes endiands entre m�quinas
			i_8[0] = resp.buff[0];
			i_8[1] = resp.buff[1];
			
			world_obstacles[i_16].profile = ((gam_obj_pack_type *)(&resp.buff[2]))->profile;  // profile
			world_obstacles[i_16].body.pos.x = ((gam_obj_pack_type *)(&resp.buff[2]))->pos_x; // posi��o x
			world_obstacles[i_16].body.pos.y = ((gam_obj_pack_type *)(&resp.buff[2]))->pos_y; // posi��o y
			world_obstacles[i_16].graph = graphs_profiles[world_obstacles[i_16].profile];
			++count;
		}	
	}
	
	
	static float retry_time =0.0;
	retry_time -= dt;
	if(retry_time <= 0){
		for(short i =0; i <  MAX_OBSTACLES;++i){
			if(!world_obstacles[i].graph.img){
				
				// Converte o 8 bits em 16. (altamente inseguro devido os diferentes endiands entre m�quinas
				
				union{
					short i_16;
					char i_8[2];
				};
				i_16 =i;
				packet_type req = {
					{
						0, // a fun��o de send se preocupa com o pack count
						2, // S� transmitir� 2 bytes de buffer
						OBSTACLE_UPDATE //
					},
					{i_8[0],i_8[1]} // O indice do objeto faltante
				};
				
				sendPacket(req);
				
				#ifdef ON_DEBUG
					char buff [30];
					sprintf(buff,"Request obstacle %d",i);
					debugTrace(buff);
				#endif
				retry_time = (0.06);
				return 0;
			}
		}
		
	}
	else return 0;
	packet_type req{
		{
			0,
			0,
			WAINTING_GAME
		},
		{}
	
	};
	sendPacket(req);	
	
	player1.body.pos.x = PLAYER_INIT_X;
	player1.body.pos.y = PLAYER_INIT_Y;
	
	player2.body.pos.x = PLAYER_INIT_X;
	player2.body.pos.y = PLAYER_INIT_Y;
	
	new_round = true;
	count = 0;
	debugTrace("clientGetObstacles: loading finished");
	return 1;
	
}

/**
 *  @brief Acha quais s�o os objetos de fronteira em rela��o ao objeto refer�ncia
 *  
 *  @param [in] ref   Objeto de refer�ncia
 *  @param [out] left  Objeto mais a esquerda
 *  @param [out] right Objeto mais a direita
 *  @return False-> Fim dos obst�culos em rela��o � refer�ncia
 *  
 */
bool setObstaclesRange (const game_object_type &ref,int &left, int &right){
	
	// Enquanto o objeto.pos.x mais a esquerda for menor que o canto mais a esquerda da tela
	while((world_obstacles[left].body.pos.x + world_obstacles[left].graph.w) < ref.body.pos.x - PLAYER_FIX_POS){
		if(++left == MAX_OBSTACLES){
			left = MAX_OBSTACLES-1;
			return false;// impede que estoure o buffer
		}
	}
	// Enquanto o objeto mais a direita (+1) for menor que o canto direito da tela
	while(world_obstacles[right+1].body.pos.x < ref.body.pos.x +(SCREEN_W - PLAYER_FIX_POS)){
		if(++right == MAX_OBSTACLES){
			right = MAX_OBSTACLES-1;
			return false; // Impede que estoure o buffer
		}
	}
}

/**
 *  @brief Imprime os objetos presentes na tela em rela��o a ref
 *  
 *  @param [in] ref Objeto refer�ncia
 *  
 */
void atualizaObjetos (game_object_type &ref,const int &left_index,const int &right_index){
	

	// Imprime todos aqueles que est�o dentro do range da tela

	for (int i = left_index; i <= right_index; ++i){
		

		float obj_x = world_obstacles[i].body.pos.x -  ref.body.pos.x + PLAYER_FIX_POS;
		print(vetor2d_type{obj_x,world_obstacles[i].body.pos.y}, &world_obstacles[i].graph);
	
	}
	

	float ref_x = (ref.body.pos.x < PLAYER_FIX_POS) ? ref.body.pos.x: PLAYER_FIX_POS;
	print(vetor2d_type{ref_x, ref.body.pos.y},&ref.graph);
}

/**
*	@brief Inicializa as condi��es para um novo jogo posicionando
*  o player no local certo e gerando um novo mapa.
*	@param dt Nessa fun��o n�o h� utilidade a n�o ser compatibilidade com os demais states
*
*   @return 1 para passar para o pr�ximo estado.
*/
int loadSingleGame (float dt){

	player1.body.pos.x = PLAYER_INIT_X;
	player1.body.pos.y = PLAYER_INIT_Y;
	
	player1.body.speed.setVector(0,0);
	total_obstacles = 0;

	initObstacles();
	
	

	
	ground_offset = 0;
	left_obstacles_index = 0;
	right_obstacles_index = 0;
	
	// Limpa qualquer tecla que esteja em buffer do teclado.
	if(kbhit())
		getch();
	fflush(stdin);

	return 1; // next state = preLancamento
}

/**
*	@brief Estado no qual o jogo aguarda o jogador escolher o �ngulo e 
*	o momento em que ele deseja lan�ar o player1.
*
*	@param dt delta tempo para a atualiza��o do demonstrativo do �ngulo escolhido
*	@return 0 caso o jogador ainda n�o tenha feito a escolha, 1 para quando deve-se iniciar o jogo.
*/
int preLancamento (float dt){
	int key;
	static int forca = 2000;
	static int angulo =0;
	static float fatorF = 0.1; 
	
	fatorF = variaForca(fatorF);
	
	if(kbhit()){
		key = (int)getch();
		
		switch(key){
			
			case SPACE:
				player1.body.speed.setVector(forca*fatorF,angulo);
				
				angulo =0;
				forca = 2000;
				return 1;
			case UP:
				angulo = angulo+5;
				if(angulo >90)
					angulo = 90;
				break;
			case DOWN:
				angulo = angulo-5;
				if(angulo <0)
					angulo = 0;
				break;
				
				
		}
		key = 0;
		
	}
	
	drawProgressBar(fatorF*BAR_MAX_HEIGHT, vetor2d_type {(player1.body.pos.x +player1.graph.w/2)-(BAR_WIDTH/2) ,-80}); // desenha barra de for�a
	groundStep( &player1, &ground,dt);	
	atualizaObjetos(player1,0,0);
	printDirection(vetor2d_type{player1.body.pos.x+(player1.graph.w/2) ,player1.body.pos.y+(player1.graph.h/2)}, angulo,300);
	return 0; // preLancamento
}

/**
 *  @brief O mesmo que o @ref preLancamento por�m com a atualiza��o da posi��o do segundo player
 *  
 *  @param [in] dt delta t para as anima��es
 *  @return Os mesmos de @ref preLancamento
 *  
 */
int preLancamentoMult (float dt){
	
	packet_type player_pos;
	if(getPacket(player_pos) > 0){
		if(player_pos.ctrl. operation == PLAYER_STATUS){
			player2.body.pos.x = ((gam_obj_pack_type *)(&player_pos.buff[0]))->pos_x; // posi��o x
			player2.body.pos.y = ((gam_obj_pack_type *)(&player_pos.buff[0]))->pos_y; // posi��o y
		}
	}
	printP2(player1, player2);
	return preLancamento(dt);
}

/**
*	@brief Estado no qual o jogo est� rodando. 
*
*	@param dt delta tempo para o c�lculo do espa�o percorrido e anima��es
*	@return 0 para o jogo em andamento, 1 para quando a velocidade seja igual a 0.
*/
int singleStep (float dt){
	
	static int left_index = 0;
	static int right_index = 0;
	static int last_colide = -1;
	static int red_aura_frames = 0;
	static int green_aura_frames = 0;
	static int show_power = 0;
	static char power_msg[100] = "";
	static int boost = POWER_UP_UNITS;
	static int angle = POWER_UP_UNITS;

	lancamento(&player1,dt);

	floorCheck(&player1);

	groundStep( &player1, &ground, dt);
	

	setObstaclesRange(player1,left_index,right_index);

	atualizaObjetos(player1,left_index,right_index);


	if(green_aura_frames){
		--green_aura_frames;
		print(vetor2d_type{PLAYER_FIX_POS - ((green_aura.graph.w - player1.graph.w)/2),player1.body.pos.y - ((green_aura.graph.h - player1.graph.h)/2)},&green_aura.graph);
	}
	

	if(red_aura_frames){
		--red_aura_frames;
		print(vetor2d_type{PLAYER_FIX_POS - ((red_aura.graph.w - player1.graph.w)/2),player1.body.pos.y  - ((red_aura.graph.h - player1.graph.h)/2)},&red_aura.graph);
	}
	

		// verifica se o player saiu da tela:
	if (player1.body.pos.y > SCREEN_H){
		exibirSeta();
	}
	

	if(kbhit()){
		char key = getch();
		
		if ((key == 'a' || key == 'A') && (boost>0)){
			mudarVelocidade(&player1.body.speed);
			sprintf(power_msg,"Apoio Popular!");
			show_power = 30;
			boost--;
		}
		
		if ((key == 'd' || key == 'D') && (angle>0)){
			player1.body.speed.setVector(player1.body.speed.modulo(), 45);
			sprintf(power_msg,"Distribui��o de Renda!");
			show_power = 30;
			angle--;
		}
	}

	for(int i = left_index; i <= right_index;++i){
		if(last_colide < i){ // Maior pois o player nunca anda para tr�s
			if(colide(player1,world_obstacles[i])){
				last_colide = i;
				if(profile_collision_bonus[world_obstacles[i].profile]>0){
					player1.body.speed.x *= (1 + profile_collision_bonus[world_obstacles[i].profile]);
					player1.body.speed.y*= (1 + profile_collision_bonus[world_obstacles[i].profile]);
					
					green_aura_frames = 30;
					red_aura_frames = 0;
				}
				else if(profile_collision_bonus[world_obstacles[i].profile]<0){
					player1.body.speed.x *= (1 + profile_collision_bonus[world_obstacles[i].profile]);
					player1.body.speed.y*= (1 + profile_collision_bonus[world_obstacles[i].profile]);
					
					red_aura_frames = 30;
					green_aura_frames = 0;
				}
				else{
					red_aura_frames = 0;
					green_aura_frames = 0;
				}
					
				break; // Afinal s� colide um por vez
			}
		}
	}


	//limitador de velocidade
	if(player1.body.speed.y> SPEED_LIM_Y)
		player1.body.speed.y =SPEED_LIM_Y;	
	if(player1.body.speed.y< -SPEED_LIM_Y)
		player1.body.speed.y = -SPEED_LIM_Y;
	if(player1.body.speed.x> SPEED_LIM_X)
		player1.body.speed.x =SPEED_LIM_X;	
	if(player1.body.speed.x< -SPEED_LIM_X)
		player1.body.speed.x = -SPEED_LIM_X;


	char  texto [100];
	sprintf(texto,"Distancia:\n %d metros",(int)(player1.body.pos.x - PLAYER_INIT_X)/50);
	setcolor(COLOR(255,255,255));
	fontSize(3);
	printTxt(texto, vetor2d_type{SCREEN_W-(textwidth(texto)+0), 20});
	
	
	
	
	
	sprintf(texto,"Power ups:");
	fontSize(1);
	printTxt(texto, vetor2d_type{SCREEN_W-(textwidth(texto)+20), 60});

	if(boost>0){

		sprintf(texto,"(A)");
		setcolor(COLOR(255,246,0));
		fontSize(2);
		printTxt(texto, vetor2d_type{SCREEN_W-(textwidth(texto)+30),80});
		
	}
	if(angle>0){

		sprintf(texto,"(D)");
		setcolor(COLOR(0,110,25));
		fontSize(2);
		printTxt(texto, vetor2d_type{SCREEN_W-(textwidth(texto)+100), 80});
	}
	
	
	
	
	
	if(show_power){
		setcolor(COLOR(255,0,0));
		fontSize(3);
		printTxt(power_msg, vetor2d_type{(SCREEN_W/2)-(textwidth(power_msg)/2), SCREEN_H/2-(textheight(power_msg)+20)});
		show_power--;
	}

	if(player1.body.speed.modulo())
		return 0;     //singleStep

	if(kbhit()){
		getch();
		fflush(stdin);
	}
	
	left_index = 0; 
	right_index = 0;
	last_colide = -1;
	red_aura_frames = 0;
	green_aura_frames = 0;
	total_score+= (int)(player1.body.pos.x - PLAYER_INIT_X)/50;
	total_rounds--;
	boost = POWER_UP_UNITS;
	angle = POWER_UP_UNITS;
	return 1;
}


/**
*  *brief Estado no qual o jogo est� rodando em multiplayer
*  
*  *param [in] dt delta tempo para o c�lculo do espa�o percorrido e anima��es
*  *return Mesmos que @ref singleStep
*/
int multiStep (float dt){
		
	// Envia a posi��o do player1 =====================================
	static float retry_time =0.0;
	retry_time -= dt;
	if(retry_time <= 0){
	
		packet_type report = {
			{
				0, // a fun��o de send se preocupa com o pack count
				sizeof(gam_obj_pack_type),
				PLAYER_STATUS //
			},
			{}
		};
		
		gam_obj_pack_type player_pos{
				player1.profile,
				player1.body.pos.x,
				player1.body.pos.y,
				player1.body.speed.x,
				player1.body.speed.y
		};
				
		memcpy(&report.buff[0],&player_pos,sizeof(gam_obj_pack_type));
		sendPacket(report);
		retry_time = 0;
	}
	// =================================================================
	packet_type player2_pos;
	if(getPacket(player2_pos) > 0){
		if(player2_pos.ctrl. operation == PLAYER_STATUS){
			player2.body.pos.x = ((gam_obj_pack_type *)(&player2_pos.buff[0]))->pos_x; // posi��o x
			player2.body.pos.y = ((gam_obj_pack_type *)(&player2_pos.buff[0]))->pos_y; // posi��o y
			player2.body.speed.x = ((gam_obj_pack_type *)(&player2_pos.buff[0]))->speed_x; // velocidade x
			player2.body.speed.y = ((gam_obj_pack_type *)(&player2_pos.buff[0]))->speed_y; // velocidade y
		}
	}
	
	printP2(player1, player2);
	
	return singleStep(dt);
}

/**
 *  @brief Exibe a somat�ria dos scores das rodadas
 *  
 *  @param [in] dt N/A
 *  @return 0-> Loop, 1-> Pr�ximo estado, 2-> fim de jogo
 *  
 */
int singleEnd(float dt){
	char *texto ="TENTE NOVAMENTE", score[50];
	int ret = 1;
	int left_index = 0;
	static int right_index = 0;
	
	groundStep( &player1, &ground, dt);
	
	setObstaclesRange(player1,left_index,right_index);
	atualizaObjetos(player1,left_index,right_index);
	
	sprintf(score,"Voc� percorreu\n %d metros em %d rodadas.",(int)total_score,3-total_rounds);
	
	setcolor(COLOR(255,255,255));
	fontSize(2);
	printTxt(score, vetor2d_type{(SCREEN_W/2)-(textwidth(score)/2), SCREEN_H/2-(textheight(score)+20)});
	if(total_rounds<=0){
	
		setcolor(COLOR(255,0,0));
		texto ="GAME OVER";
		ret=2;
	}
	fontSize(5);
	printTxt(texto, vetor2d_type{(SCREEN_W/2)-(textwidth(texto)/2), SCREEN_H/2});

	if(kbhit()) {
		
		if(ret == 2){
			total_score = 0;
			total_rounds =3;
		}
		
		left_index = 0;
		right_index = 0;
		

		return ret;
		 
	}
	
	return 0;
}

/**
 *  \brief Caso seja o primeiro player a parar, exibe o jogo do outro player, depois, exibe a somat�ria dos scores das rodadas
 *  
 *  \param [in] dt delta tempo para as anima��es
 *  \return @return 0-> Loop, 1-> Pr�ximo estado
 */
int multiEnd(float dt){
	char *texto ="TENTE NOVAMENTE", score[50];
	int ret = 1;
	int left_index = 0;
	static int right_index = 0;
	static bool show_score = false;
	static int other_score = 0;
	
	
	static float retry_time =0.0;
	retry_time -= dt;
	if(retry_time <= 0){

		packet_type report = {
			{
				0, // a fun��o de send se preocupa com o pack count
				sizeof(int),
				SCORE_UPDATE //
			},
			{}
		};
					
		memcpy(report.buff,&total_score,sizeof(int));
		sendPacket(report);
		retry_time = 0.5;
	}
	
	
	if(!show_score){
		
		packet_type resp;
		if(getPacket(resp) > 0){
			if(resp.ctrl.operation == SCORE_UPDATE){
				
				memcpy(&other_score,resp.buff,sizeof(int));
				
				show_score = true;
				left_index = 0;
				right_index = 0;
				return 0;
			}
			else if(resp.ctrl.operation == PLAYER_STATUS){
				player2.body.pos.x = ((gam_obj_pack_type *)(&resp.buff[0]))->pos_x; // posi��o x
				player2.body.pos.y = ((gam_obj_pack_type *)(&resp.buff[0]))->pos_y; // posi��o y
				player2.body.speed.x = ((gam_obj_pack_type *)(&resp.buff[0]))->speed_x; // velocidade x
				player2.body.speed.y = ((gam_obj_pack_type *)(&resp.buff[0]))->speed_y; // velocidade y
			}
		}
		
			// =====================================
		static float retry_time =0.0;
		retry_time -= dt;
		if(retry_time <= 0){
		
			packet_type report = {
				{
					0, // a fun��o de send se preocupa com o pack count
					sizeof(int),
					SCORE_UPDATE //
				},
				{}
			};
						
			memcpy(report.buff,&total_score,sizeof(int));
			sendPacket(report);
			retry_time = 0.5;
		}

		
		setObstaclesRange(player2,left_index,right_index);
		atualizaObjetos(player2,left_index,right_index);
		groundStep(&player2,&ground,dt);	
		
	
	static float blink_time = 0.5;
	static char blue = 255, green = 255;
	blink_time -=dt;
	if(blink_time <=0){
		green = blue = (blue) ? 0:255;
		blink_time = 0.5;
	}
		
	char texto[] = "Spectator mode";
	setcolor(COLOR(255,green,blue));
	fontSize(2);
	printTxt(texto, vetor2d_type{(SCREEN_W/2)-(textwidth(texto)/2), SCREEN_H/2});
		
	}
	else{
	
		groundStep( &player1, &ground, dt);
	
		setObstaclesRange(player1,left_index,right_index);
		atualizaObjetos(player1,left_index,right_index);
		
		sprintf(score,"Voc� percorreu\n %d metros em %d rodadas.",(int)total_score,3-total_rounds);
		
		setcolor(COLOR(255,255,255));
		fontSize(2);
		printTxt(score, vetor2d_type{(SCREEN_W/2)-(textwidth(score)/2), SCREEN_H/2-(textheight(score)+20)});
		
		sprintf(score,"O player 2 percorreu\n %d metros em %d rodadas.",(int)other_score,3-total_rounds);
		
		setcolor(COLOR(255,255,255));
		fontSize(2);
		printTxt(score, vetor2d_type{(SCREEN_W/2)-(textwidth(score)/2), SCREEN_H/2-(textheight(score)+40)});
		
		if(total_rounds<=0){
		
			setcolor(COLOR(255,0,0));
			texto ="GAME OVER";
			ret=2;
		}
		fontSize(5);
		printTxt(texto, vetor2d_type{(SCREEN_W/2)-(textwidth(texto)/2), SCREEN_H/2});

		if(kbhit()) {
			
			if(ret == 2){
				total_score = 0;
				total_rounds =3;
			}
			
			left_index = 0;
			right_index = 0;
			
			show_score = false;;
			
			return ret;
			 
		}
		
		return 0;
	
	}
	return 0;
}


/**
*	@brief Testa o contato do player com o ch�o 
*
*	@param player Objeto a ser testado
*/
void floorCheck(game_object_type *player){
	
	if(player->body.pos.y<=0){
			player->body.pos.y=0;
						
			player->body.speed.y = -BOUNCE*player->body.speed.y;
			atrito(player, ATRITO, TARGET_FRAME_RATE);
			
			if(player->body.speed.modulo() < 17)
				player->body.speed.setVector(0,0);
		}
}

/**
*	@brief Fun��o que calcula o deslocamento do ch�o em fun��o da posi��o do player1 
*
*	@param objeto 	player
*	@param ground   ground
* 	@param dt delta tempo para o c�lculo do espa�o percorrido.
*/
void groundStep(game_object_type *objeto, game_object_type *ground, float dt){
	

	if(objeto->body.pos.x>= PLAYER_FIX_POS)
		ground_offset = ground_offset - objeto->body.speed.x*dt;
		
	if(ground_offset <=(float)-ground->graph.w)
		ground_offset = ground_offset + ground->graph.w;
	
	for(int i =0; i<=(SCREEN_W/ground->graph.w)+1;i++){
		int posground=i*ground->graph.w+ ground_offset;
		print(vetor2d_type{posground, ground->body.pos.y}, &ground->graph);
	}	
}

/**
*  @brief Varia valor de for�a de 10% � 100%
*  
*  @param [in] valor Valor inicial
*  @return valor final
*/
float variaForca(float valor){
	static float incremento = 0.02;

		valor  = valor + incremento;
		if(valor>1){
			valor = 1;
			incremento = -incremento;
		}
		if(valor<0.1){
			valor = 0.1;
			incremento = -incremento;
		}	

	return valor;	
	
}
/**
 *  @brief Faz o processo de reinicializa��o de todos os par�metros necess�rio para o reset do jogo
 */
void resetGame(){
	

	total_obstacles = 0;
	total_score = 0; 
	total_rounds = 3;	
	left_obstacles_index = 0;
	right_obstacles_index = 0;
	player1.body.pos.x = PLAYER_INIT_X;
	player1.body.pos.y = PLAYER_INIT_Y;	
	player1.body.speed.setVector(0,0);
	ground_offset = 0;
	resetLoja();	

	//inicializa o fator inicial de bonus
	for(int i = 0; i < NUM_BLOCKS;++i)	profile_collision_bonus[i] = 0.0;

}
/**
 *  @brief Reinicia os objetos da loja
 */
void resetLoja(){
	//Itens da loja
	int coluna, linha, j=0;
	for(int i = 0; i < NUM_LOJA_MENU; ++i){
		loja_options[i].graph = graphs_profiles[LOJA_OPTION_1 + i];
		
		if (i == NUM_LOJA_MENU-2){
			coluna = SCREEN_W -graphs_profiles[LOJA_OPTION_1 + i].w -20;
			linha = (500)-(graphs_profiles[LOJA_OPTION_1 + i].h +10);}
		else if (i == NUM_LOJA_MENU-1){
			coluna = SCREEN_W -graphs_profiles[LOJA_OPTION_1 + i].w -20;
			linha = (500)-(graphs_profiles[LOJA_OPTION_1 + i].h*2 +10);}
		else if (i%2 == 0){
			coluna = 20;
			linha = (500)-(graphs_profiles[LOJA_OPTION_1 + i].h +10)*j;
			j++;
		} else
			coluna = graphs_profiles[LOJA_OPTION_1 + i].w +25;
			
		
		vetor2d_type loja_pos{coluna,linha};
		loja_options[i].body.pos =	loja_pos;
	}
}

/**
 *  @brief Exibe uma seta no canto superior da tela caso o usu�rio ultrapasse as bordas, representando sua posi��o atual 
 */
void exibirSeta (){
	print(vetor2d_type { (player1.body.pos.x< PLAYER_FIX_POS)?player1.body.pos.x: PLAYER_FIX_POS, SCREEN_H - graphs_profiles[SETA_CIMA_P1].h-10}, &graphs_profiles[SETA_CIMA_P1]);
}

/**
 *  @brief aumenta a velocidade do player em 20%
 *  
 *  @param [out] *speed Parameter_Description
 */
 
/**
*  @brief Power up que aumenta a velocidade do objeto em 20%
*  
*  @param [out] speed Velocidade do objeto
*/
void mudarVelocidade(vetor2d_type *speed){
	// aumenta a velocidade em 20%
	speed->x = speed->x * 1.2;
	
	speed->y = speed->y * 1.2;
	
}

/**
*  @brief Fun��o de impress�o de feedback da pocis�o do segundo player em rela��o ao primeiro
*  
*  @param [in] ref refer�ncia do player
*  @param [in] p2  player a ser impresso conforme a refer�ncia
*/
void printP2(game_object_type &ref, game_object_type &p2){
	
	float dist_to_ref = p2.body.pos.x - ref.body.pos.x;
	
	
	if((dist_to_ref> -PLAYER_FIX_POS)&&(dist_to_ref< SCREEN_W)){
		
		print(vetor2d_type{dist_to_ref, p2.body.pos.y},&p2.graph);	
		
		if(p2.body.pos.y > SCREEN_H) 
			print(vetor2d_type { p2.body.pos.x, SCREEN_H - graphs_profiles[SETA_CIMA_P2].h-10}, &graphs_profiles[SETA_CIMA_P2]);
	}
	//checa se p2 ficou pra tr�s e exibe seta esquerda
	if(dist_to_ref< (-PLAYER_FIX_POS - graphs_profiles[SETA_ESQUERDA_P2].h) ){
		print(vetor2d_type { 0,(p2.body.pos.y>SCREEN_H -graphs_profiles[SETA_ESQUERDA_P2].h )?SCREEN_H -graphs_profiles[SETA_ESQUERDA_P2].h : p2.body.pos.y}, &graphs_profiles[SETA_ESQUERDA_P2]);
	}
	
	//checa se p2 est� a frente e exibe seta direita
	if(dist_to_ref > SCREEN_W){
		
		print(vetor2d_type { SCREEN_W -graphs_profiles[SETA_DIREITA_P2].w ,(p2.body.pos.y>SCREEN_H -graphs_profiles[SETA_DIREITA_P2].h )?SCREEN_H -graphs_profiles[SETA_DIREITA_P2].h : p2.body.pos.y}, &graphs_profiles[SETA_DIREITA_P2]);
	}
	
	
	
}

