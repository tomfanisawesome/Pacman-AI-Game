#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>


#include "ai.h"
#include "utils.h"
#include "priority_queue.h"


struct heap h;

float get_reward( node_t* n );

/**
 * Function called by pacman.c
*/
void initialize_ai(){
	heap_init(&h);
}

/**
 * function to copy a src into a dst state
*/
void copy_state(state_t* dst, state_t* src){
	//Location of Ghosts and Pacman
	memcpy( dst->Loc, src->Loc, 5*2*sizeof(int) );

    //Direction of Ghosts and Pacman
	memcpy( dst->Dir, src->Dir, 5*2*sizeof(int) );

    //Default location in case Pacman/Ghosts die
	memcpy( dst->StartingPoints, src->StartingPoints, 5*2*sizeof(int) );

    //Check for invincibility
    dst->Invincible = src->Invincible;
    
    //Number of pellets left in level
    dst->Food = src->Food;
    
    //Main level array
	memcpy( dst->Level, src->Level, 29*28*sizeof(int) );

    //What level number are we on?
    dst->LevelNumber = src->LevelNumber;
    
    //Keep track of how many points to give for eating ghosts
    dst->GhostsInARow = src->GhostsInARow;

    //How long left for invincibility
    dst->tleft = src->tleft;

    //Initial points
    dst->Points = src->Points;

    //Remiaining Lives
    dst->Lives = src->Lives;   

}

node_t* create_init_node( state_t* init_state ){
	node_t * new_n = (node_t *) malloc(sizeof(node_t));
	new_n->parent = NULL;	
	new_n->priority = 0;
	new_n->depth = 0;
	new_n->num_childs = 0;
	copy_state(&(new_n->state), init_state);
	new_n->acc_reward =  get_reward( new_n );
	return new_n;
	
}


float heuristic( node_t* n ){
	float h = 0;
	float i = 0, l = 0, g = 0;
	
	//FILL IN MISSING CODE
	// Pac-Man has eaten a fruit and becomes invincible in that state
	if( n->state.Invincible == 1 ) i = 10;
	
	// a life has been lost in that state
	if( n->parent && n->parent->state.Lives > n->state.Lives ) l = 10;
	
	// g = 100 if the game is over
	if( n->state.Lives == 0 ) g = 100;
	
	h = i-l-g;
	return h;
}

float get_reward ( node_t* n ){
	float reward = 0;
	
	//FILL IN MISSING CODE
	// the heuristic value
	reward = heuristic( n );
	
	// the change in score from the current node and the parent node
	if( n->parent != NULL )
		reward += n->state.Points - n->parent->state.Points;
	
	// a discount factor of γ = 0.99^d, where d is the depth of the node,
	float discount = pow(0.99, n->depth);
	reward *= discount;
	
	// updates the accumulated reward from the initial node up to the current node
	if( n->parent !=NULL )
		reward += n->parent->acc_reward;
   	
	return reward;
}

/**
 * Apply an action to node n and return a new node resulting from executing the action
*/
bool applyAction(node_t* n, node_t** new_node, move_t action ){

	bool changed_dir = false;

	// points to the parent
	(*new_node)->parent = n;
	(*new_node)->depth = n->depth + 1;
	
	// updates the priority of the node to be the negative node’s depth d
	(*new_node)->priority = - (*new_node)->depth;
	
	copy_state(&((*new_node)->state), &(n->state));
	(*new_node)->move = action;

    changed_dir = execute_move_t( &((*new_node)->state), action );	
	
	// updates the reward
	(*new_node)->acc_reward = get_reward( *new_node );
	
	return changed_dir;

}

/**
 * Check whether the move is applicable under current state
*/
bool check_applicable(state_t* state, move_t move){
	int x = state->Loc[4][0];
	int y = state->Loc[4][1];
    switch (move) {
	case left:         //Move left
		y = y-1;
		if( y<0 ) y = 28-1;
        break;
    case right:          //Move right
		y = y+1;
		if(y>=28) y = 0;
        break;		
    case up:          //Move  up
		x = x -1;
		if( x<0 ) x = 29-1;
        break;

    case down:         //Move  down
		x = x+ 1;
		if( x>=29 ) x = 0;
        break;
	}
	if( state->Level[x][y]!=1 && state->Level[x][y]!=4 ) return true;
	return false;
}

/**
 * takes the score of the newly generated node, and propagates back the score to the first action of the path.
 */
void propagateBackScoreToFirstAction(node_t* node, propagation_t propagation, float* best_action_score){
	move_t action;
	float score = 0;
	if( propagation == avg ) score = 0;
	else if( propagation == max ) score = INT_MIN;
	
	// go throught the path from node to the first one
	node_t* current = node;
	while( NULL != current->parent ){ 
		action = current->move;
		if( propagation == max ){
			if( current->acc_reward > score ) score = current->acc_reward;
		}else if( propagation == avg ){
			score += current->acc_reward / node->depth;
		}
		current = current->parent;
	}
	
	// update action's score to best_action_score
	if( score > best_action_score[action] ) best_action_score[action] = score;
}


/**
 * Find best action by building all possible paths up to budget
 * and back propagate using either max or avg
 */

move_t get_next_move( state_t init_state, int budget, propagation_t propagation, char* stats, int* info ){
	move_t best_action = rand() % 4;

	float best_action_score[4];
	for(unsigned i = 0; i < 4; i++)
	    best_action_score[i] = INT_MIN;

	unsigned generated_nodes = 0;
	unsigned expanded_nodes = 0;
	unsigned max_depth = 0;
	


	//Add the initial node
	node_t* n = create_init_node( &init_state );
	
	//Use the max heap API provided in priority_queue.h
	heap_push(&h,n);
	
	//FILL IN THE GRAPH ALGORITHM
	// array list that store all the node
	int explored_node_num = 0;
	node_t** explored_node_list = (node_t **) malloc(sizeof(node_t)*(64+budget));
	// failed to malloc
	if( explored_node_list==NULL )
		exit(-1);

	while( 0 < h.count ){
		node_t* node = heap_delete(&h);
		explored_node_list[explored_node_num] = node;
		explored_node_num += 1;
		
		if( budget > explored_node_num ){
			move_t action_list[4] = {left, right, up, down};
			for(unsigned i=0; i<4; i++){
				if( !check_applicable( &(node->state), action_list[i] ) ) continue;
				
				node_t* next_node = (node_t *) malloc(sizeof(node_t));
				// failed to malloc
				if( next_node==NULL ) exit(-1);

				generated_nodes += 1;					
				applyAction(node, &next_node, action_list[i]);
					
				// function propagateBackScoreToFirstAction
				propagateBackScoreToFirstAction(next_node, propagation, best_action_score);
					
				// lose life ?
				if( node->state.Lives > next_node->state.Lives ){
					free(next_node);
				}else{
					heap_push(&h, next_node);
					if( next_node->depth > max_depth ) max_depth = next_node->depth;
				}
			}
			expanded_nodes += 1;
		}
	}
	// freeMemory(explored)
	for(int i=0; i<explored_node_num; i++) free(explored_node_list[i]);
	free(explored_node_list);

	// bestAction
	for(int i = 0; i < 4; i++)
		if( best_action_score[i] > best_action_score[best_action] )
	    	best_action = i;
	
	// summary all the generated and expanded node info during the whole game
	if( max_depth > info[0] ) info[0] = max_depth; // max_depth
	info[1] += generated_nodes; // total_generated_node
	info[2] += expanded_nodes; // total_expaned_node
	
	sprintf(stats, "Max Depth: %d Expanded nodes: %d  Generated nodes: %d\n",max_depth,expanded_nodes,generated_nodes);
	
	if(best_action == left)
		sprintf(stats, "%sSelected action: Left\n",stats);
	if(best_action == right)
		sprintf(stats, "%sSelected action: Right\n",stats);
	if(best_action == up)
		sprintf(stats, "%sSelected action: Up\n",stats);
	if(best_action == down)
		sprintf(stats, "%sSelected action: Down\n",stats);

	sprintf(stats, "%sScore Left %f Right %f Up %f Down %f",stats,best_action_score[left],best_action_score[right],best_action_score[up],best_action_score[down]);
	return best_action;
}