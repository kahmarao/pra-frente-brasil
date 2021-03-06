#ifndef __GAME_H__
#define __GAME_H__

#include "h/types.h"


// Typedefs p�blicos ====================================
typedef struct game_state_type{

	int (*func)(float dt);
	game_state_type **states;
}game_state_type;


// defines p�blicos =====================================
#define UP 		72
#define DOWN 	80
#define LEFT   	75
#define RIGHT  	77
#define SPACE 	(int)' '
#define PWUP_1	'A'
#define PWUP_2	'S'
#define ESC 27
#define TARGET_FRAME_RATE (1.0/120.0)
// Prot�tipos P�blicos ==================================
void endGame (void);

// Vari�veis p�blicas ===================================
extern game_state_type *game_states;
extern game_state_type load_menu_state;
extern game_state_type menu_state;
extern game_state_type load_loja;
extern game_state_type pre_lancamento;
extern game_state_type step_single;
extern game_state_type load_single;
extern game_state_type termina_programa;
extern game_state_type server_conect;
extern game_state_type client_conect;
extern game_state_type server_obstacles;
extern game_state_type client_obstacles;
extern game_state_type server_prelancamento;
extern game_state_type client_prelancamento;
extern game_state_type step_server;
extern game_state_type step_client;
extern game_state_type end_server;
extern game_state_type end_client;
extern game_state_type load_loja_server;
extern game_state_type load_loja_client;
extern game_state_type loja_server;
extern game_state_type loja_client;
#endif
