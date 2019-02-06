/**
* CSC369 Assignment 2
*
* This is the source/implementation file for your safe stop sign
* submission code.
*/
/* include {{{ */

#include "safeStopSign.h"
#include "stopSign.h"
#include "intersection.h"
#include "mutexAccessValidator.h"

/* }}} include */

void initSafeStopSign(SafeStopSign* sign, int count) {
	int i;
	initStopSign(&sign->base, count);

	for(i = 0; i < DIRECTION_COUNT; i++) {
		(*sign).lane_queue[i].count = 0;
		(*sign).lane_queue[i].orig_front = malloc(sizeof(LaneNode));
		(*sign).lane_queue[i].cur_front = malloc(sizeof(LaneNode));
		(*sign).lane_queue[i].back = malloc(sizeof(LaneNode));
		(*sign).lane_mutex[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		(*sign).lane_turn[i] = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	}
	(*sign).quadrants_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	(*sign).quadrants_turn = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}

/* destroySafeStopSign {{{ */

void destroySafeStopSign(SafeStopSign* sign) {
	destroyStopSign(&sign->base);

	// TODO: Add any logic you need to clean up data structures.
	/* for(i = 0; i < DIRECTION_COUNT; i++) {
		LaneNode* cur_front = (*sign).lane_queue[i].orig_front;
		LaneNode* next_front;
		while (cur_front != (*sign).lane_queue[i].back) { 
			next_front = cur_front->next;
			free(cur_front->car);
			printf("Freeing next.\n");
			free(cur_front->next);
			printf("Freeing front.\n");
			free(cur_front);
		}
	} */
}

/* }}}  destroySafeStopSign */

void addCarToLaneQueue(Car* car, SafeStopSign* sign)
{
	int lane_index = getLaneIndex(car);
	/* TODO: add car to LaneQueue */
	/* create LaneNode for current Car */
	LaneNode* cur_car_node = malloc(sizeof(LaneNode*));
	cur_car_node->car = malloc(sizeof(Car*));
	cur_car_node->next = malloc(sizeof(LaneNode*));
	cur_car_node->car = car;

	LaneQueue* cur_lane_queue = &sign->lane_queue[lane_index];
	/* add cur_car_node to cur_lane_queue after acquiring lock for cur lane*/
	pthread_mutex_lock(&sign->lane_mutex[lane_index]);
	if (cur_lane_queue->count == 0) {
		cur_lane_queue->orig_front = cur_car_node;
		cur_lane_queue->cur_front = cur_car_node;
		cur_lane_queue->back = cur_car_node;
	} else {
		cur_lane_queue->back->next = cur_car_node;
		cur_lane_queue->back = cur_car_node;
	}
	cur_lane_queue->count++;
	printf("Added car %d to LaneQueue %d at pos %d.\n", car->index, lane_index,
			cur_lane_queue->count - 1);
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);
}

void dequeueFront(SafeStopSign* sign, int lane_index)
{
	LaneQueue* cur_lane_queue = &sign->lane_queue[lane_index];

	pthread_mutex_lock(&sign->lane_mutex[lane_index]);
	if (cur_lane_queue->count == 0) { 
		return;	
	}
	LaneNode* cur_front = cur_lane_queue->cur_front;
	/* printf("Dequeuing Car %d from Lane %d.\n",
			cur_front->car->index, lane_index); */
	/* int* cur_front_token = &getLane(cur_front->car, &sign->base)->enterTokens[cur_front->car->index].valid; */
	int* cur_front_token = &sign->base.tokens[cur_front->car->index].valid;
	/* printf("Car %d has token value %d before dequeuing car.\n",
			cur_front->car->index, *cur_front_token); */
	if (cur_lane_queue->count == 1) {
		cur_lane_queue->cur_front = NULL;
		cur_lane_queue->back = NULL;
	} else {
		cur_lane_queue->cur_front = cur_lane_queue->cur_front->next;
	}
	/* printf("Dequeuing and freeing Car %d.\n", cur_front->car->index); */
	/* printf("Car %d has token value is %d before freeing car.\n",
			, *cur_front_token); */
	/* free(cur_front->car);
	printf("Freeing next.\n");
	free(cur_front->next);
	printf("Freeing front.\n");
	free(cur_front); */
	cur_lane_queue->count--;
	/* printf("Dequeued Car %d from Lane %d. New front is Car %d.\n",
			cur_front->car->index, lane_index, 
			cur_lane_queue->cur_front->car->index); */
	/* printf("Dequeued Car %d from Lane %d.\n", cur_front->car->index, lane_index); */
	/* printf("Car %d has token value %d after dequeuing car.\n",
			cur_front->car->index, *cur_front_token); */
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);
	/* printf("Cur front token value is %d after freeing.\n", *cur_front_token); */
}

/* TODO: setup queue for entering lane */

/* checkIfQuadrantsSafe {{{ */

int checkIfQuadrantsSafe(Car* car, StopSign* intersection) {
	int quadrants[QUADRANT_COUNT];
	int quadrantCount = getStopSignRequiredQuadrants(car, quadrants);
	MutexAccessValidator curValidator;
	for (int i = 0; i < quadrantCount; i++) {
		curValidator = intersection->quadrants[quadrants[i]].validator;
		pthread_mutex_lock(&curValidator.lock);
		if (curValidator.current != NULL) {
			printf("Not safe for Car %d in Lane %d to make Action %d.\n",
					car->index, getLaneIndex(car), car->action);
			return 0;
		}
		pthread_mutex_unlock(&curValidator.lock);
	}
	return 1;
}

/* }}} checkIfQuadrantsSafe */

void waitForFrontOfQueue(Car* car, SafeStopSign* sign)
{
	int lane_index = getLaneIndex(car);
	/* check if car is in front of queue before going through stop sign */
	pthread_mutex_lock(&sign->lane_mutex[lane_index]);
	while (sign->lane_queue[lane_index].cur_front->car != car) {
		/* printf("Waiting for Car %d in Lane %d to be in front. ",
				car->index, getLaneIndex(car));
		printf("Currently %d cars with Car %d in front.\n",
				sign->lane_queue[lane_index].count,
				sign->lane_queue[lane_index].cur_front->car->index); */
		pthread_cond_wait(&sign->lane_turn[lane_index],
				&sign->lane_mutex[lane_index]);
	}
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);
}

/* goThroughStopSignValid {{{ */

void goThroughStopSignValid(Car* car, SafeStopSign* sign) {
	int lane_index = getLaneIndex(car);
	waitForFrontOfQueue(car, sign);
	/* set entering car lane to empty */
	/* pthread_mutex_lock(&sign->lane_mutex);
	sign->car_entering[getLaneIndex(car)] = 0;
	pthread_cond_broadcast(&sign->lane_turn);
	pthread_mutex_unlock(&sign->lane_mutex); */

	/* TODO: forcing cars to go through stop sign one at a time right now with
	 * the mutex */
	pthread_mutex_lock(&sign->quadrants_mutex);
	while (checkIfQuadrantsSafe(car, &sign->base) == 0) {
		pthread_cond_wait(&sign->quadrants_turn, &sign->quadrants_mutex);
	}
	/* assume at this point quadrants are free. */

	/* go through stop sign and broadcast to others to go after */
	/* printf("Car %d entering Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid); */
	goThroughStopSign(car, &sign->base);
	/* printf("Car %d went through Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid); */
	pthread_cond_broadcast(&sign->quadrants_turn);
	dequeueFront(sign, lane_index);
	/* printf("Car %d dequeued from Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid); */
	printf("Car %d dequeued from Lane %d.\n", car->index, getLaneIndex(car));
	pthread_cond_broadcast(&sign->lane_turn[lane_index]);
	pthread_mutex_unlock(&sign->quadrants_mutex);
}

/* }}} goThroughStopSignValid */

/* runStopSignCar {{{ */

void runStopSignCar(Car* car, SafeStopSign* sign) {
	/* enterLaneValid(car, sign); */
	addCarToLaneQueue(car, sign);
	enterLane(car, getLane(car, &sign->base));
	/* printf("Car %d entered Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid); */

	/* TODO: check if their turn to enter intersection (i.e. front of
	 * LaneQueue) */
	goThroughStopSignValid(car, sign);

	exitIntersection(car, getLane(car, &sign->base));
}

/* }}} runStopSignCar */

/* OLD/DEPRECATED - enter lane when possible */
/* enterLaneValid {{{ */

/* void enterLaneValid(Car* car, SafeStopSign* sign) {
	int car_lane_index; 

	pthread_mutex_lock(&sign->lane_mutex);
	car_lane_index = getLaneIndex(car);
	[>wait while other cars entering or someone already entering<]
	while(sign->car_entering[car_lane_index] == 1) {
		pthread_cond_wait(&sign->lane_turn, &sign->lane_mutex);
	}
	[>assume at this point lane is free and no other cars entering. then,
	 * enter lane<]
	enterLane(car, getLane(car, &sign->base));
	sign->car_entering[car_lane_index] = 1;
	[>allow other cars to try and enter lanes<]
	pthread_cond_broadcast(&sign->lane_turn);
	pthread_mutex_unlock(&sign->lane_mutex);
} */

/* }}} enterLaneValid */

