#include "safeStopSign.h"/*{{{*/
#include "stopSign.h"
#include "intersection.h"
#include "mutexAccessValidator.h"/*}}}*/

void initSafeStopSign(SafeStopSign* sign, int count) {/*{{{*/
	int i;
	initStopSign(&sign->base, count);

	for(i = 0; i < DIRECTION_COUNT; i++) {
		/* init LaneQueue */
		sign->lane_queue[i].count = 0;
		/* don't need to malloc mem for pointers in LaneQueue struct since will
		 * malloc when creating LaneNode */
		sign->lane_queue[i].orig_front = NULL;

		sign->lane_mutex[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		sign->lane_turn[i] = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	}
	sign->quadrants_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	sign->quadrants_turn = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	for (i = 0; i < 4; i++) {
		sign->busy_quadrants[i] = 0;
	}
}/*}}}*/

void destroyLaneNodes(SafeStopSign* sign) {/*{{{*/
	/* only need to free LaneNode's inside LaneQueue since all other var's will
	 * be freed in testing when this sign pointer is freed */
	LaneNode* cur_front;
	LaneNode* next_front;
	int cars_freed = 0;
	for(int i = 0; i < DIRECTION_COUNT; i++) {
		cur_front = sign->lane_queue[i].orig_front;
		while (cur_front != NULL) {
			next_front = cur_front->next;
			/* printf("next_front is: %p\n", next_front); */
			cars_freed++;
			free(cur_front);
			cur_front = next_front;
		}
	}
	printf("%d cars freed.\n", cars_freed);
}/*}}}*/

void destroySafeStopSign(SafeStopSign* sign) {/*{{{*/
	destroyStopSign(&sign->base);
	destroyLaneNodes(sign);
}/*}}}*/

void addCarToLaneQueue(Car* car, SafeStopSign* sign)/*{{{*/
{
	int lane_index = getLaneIndex(car);
	/* create LaneNode for current Car */
	LaneNode* cur_car_node = (LaneNode*)malloc(sizeof(LaneNode));
	/* don't need to malloc for car since already malloc'd by testing */
	cur_car_node->car = car;
	cur_car_node->next = NULL;

	LaneQueue* cur_lane_queue = &sign->lane_queue[lane_index];
	/* add cur_car_node to cur_lane_queue after acquiring lock for cur lane*/
	pthread_mutex_lock(&sign->lane_mutex[lane_index]);
	if (cur_lane_queue->count == 0) {
		/* check if first car for this lane to set orig_front */
		if (cur_lane_queue->orig_front == NULL) {
			cur_lane_queue->orig_front = cur_car_node;
			/* printf("Assigning orig_front for Lane %d with Car %d.\n", lane_index, car->index); */
		/* link prev. back to this car if not orig_front to keep list
		 * linked */
		} else {
			/* printf("LaneQueue is empty. orig_front is %p.", cur_lane_queue->orig_front); */
			cur_lane_queue->back->next = cur_car_node;
		}
		cur_lane_queue->cur_front = cur_car_node;
		cur_lane_queue->back = cur_car_node;
	} else {
		cur_lane_queue->back->next = cur_car_node;
		cur_lane_queue->back = cur_car_node;
	}
	cur_lane_queue->count++;
	enterLane(car, getLane(car, &sign->base));
	/* printf("Added Car %d (addr. %p) to LaneQueue %d at pos %d. "
			"orig_front is Car %d.\n", car->index,
			car, lane_index, cur_lane_queue->count - 1,
			cur_lane_queue->orig_front->car->index); */
	/* printf("Added Car %d (addr. %p) to LaneQueue %d at pos %d.\n",
			car->index, car, lane_index, cur_lane_queue->count - 1); */
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);
}/*}}}*/

void dequeueFront(SafeStopSign* sign, int lane_index)/*{{{*/
{
	LaneQueue* cur_lane_queue = &sign->lane_queue[lane_index];

	pthread_mutex_lock(&sign->lane_mutex[lane_index]);
	if (cur_lane_queue->count == 0) { 
		return;	
	}
	/* LaneNode* cur_front = cur_lane_queue->cur_front; */
	/* printf("Dequeuing Car %d from Lane %d.\n",
			cur_front->car->index, lane_index); */
	/* int* cur_front_token = &getLane(cur_front->car, &sign->base)->enterTokens[cur_front->car->index].valid; */
	/* int* cur_front_token = &sign->base.tokens[cur_front->car->index].valid; */
	/* printf("Car %d has token value %d before dequeuing car.\n",
			cur_front->car->index, *cur_front_token); */
	if (cur_lane_queue->count == 1) {
		cur_lane_queue->cur_front = NULL;
	} else {
		cur_lane_queue->cur_front = cur_lane_queue->cur_front->next;
	}
	cur_lane_queue->count--;
	/* printf("Dequeued Car %d from Lane %d. New count is %d.\n",
			cur_front->car->index, lane_index,
			cur_lane_queue->count); */
	/* printf("Car %d has token value %d after dequeuing car.\n",
			cur_front->car->index, *cur_front_token); */
	/* wake up cars waiting to enter intersection due to not being in front */
	pthread_cond_broadcast(&sign->lane_turn[lane_index]);
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);
	/* printf("Cur front token value is %d after freeing.\n", *cur_front_token); */
}/*}}}*/

void waitForFrontOfQueue(Car* car, SafeStopSign* sign)/*{{{*/
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
		/* pthread_cond_wait(&sign->quadrants_turn,
				&sign->lane_mutex[lane_index]); */
	}
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);
}/*}}}*/

int checkIfQuadrantsSafe(Car* car, SafeStopSign* sign) {/*{{{*/
	/* assume if called this function that have quadrants_mutex */
	int car_quadrants[QUADRANT_COUNT];
	int quadrant_count = getStopSignRequiredQuadrants(car, car_quadrants);
	for (int i = 0; i < quadrant_count; i++) {
		if (sign->busy_quadrants[car_quadrants[i]] == 1) { 
			/* printf("Not safe for Car %d in Lane %d to make Action %d.\n",
					car->index, getLaneIndex(car), car->action); */
			return 0;
		}
	}
	return 1;
}/*}}}*/

void setCarQuadrants(Car* car, SafeStopSign* sign, int value) {/*{{{*/
	/* assume if called this function that have quadrants_mutex */
	int i;
	int car_quadrants[QUADRANT_COUNT];
	int quadrant_count = getStopSignRequiredQuadrants(car, car_quadrants);
	/* printf("Setting Car %d quadrants to %d.\n", car->index, value); */
	for (i = 0; i < quadrant_count; i++) {
		sign->busy_quadrants[car_quadrants[i]] = value;
	}

	/* check if simul. cars in intersection  {{{ */
	
	/* if (value == 1) {
		int j;
		for (i = 0; i < QUADRANT_COUNT; i++) {
			printf("%d", sign->busy_quadrants[i]);
			for (j = 0; j < quadrant_count; j++) {
				if (car_quadrants[j] == i) {
					printf("C");
				}
			}
			printf(" | ");
		}
		printf("\n");
	} */
	
	/* }}} check if simul. cars in intersection  */
}/*}}}*/

void goThroughStopSignValid(Car* car, SafeStopSign* sign) {/*{{{*/
	/* int lane_index = getLaneIndex(car); */
	waitForFrontOfQueue(car, sign);

	/* TODO: forcing cars to go through stop sign one at a time right now with
	 * the mutex? */
	pthread_mutex_lock(&sign->quadrants_mutex);
	while (checkIfQuadrantsSafe(car, sign) == 0) {
		pthread_cond_wait(&sign->quadrants_turn, &sign->quadrants_mutex);
	}
	/* assume at this point quadrants are free. */
	setCarQuadrants(car, sign, 1);
	pthread_mutex_unlock(&sign->quadrants_mutex);

	/* go through stop sign and broadcast to others to go after */
	/* printf("Car %d entering Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid); */
	goThroughStopSign(car, &sign->base);
}/*}}}*/

void exitIntersectionValid(Car* car, SafeStopSign* sign)/*{{{*/
{
	int lane_index = getLaneIndex(car);

	pthread_mutex_lock(&sign->quadrants_mutex);
	exitIntersection(car, getLane(car, &sign->base));
	/* free car quadrants */
	setCarQuadrants(car, sign, 0);
	/* wake up cars waiting to enter intersection due to quadrants being busy */
	pthread_cond_broadcast(&sign->quadrants_turn);
	pthread_mutex_unlock(&sign->quadrants_mutex);
	/* printf("Car %d exited from Lane %d.\n", car->index, getLaneIndex(car)); */
	/* printf("Car %d went through Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid); */

	dequeueFront(sign, lane_index);
	/*printf("Car %d dequeued from Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid);*/
	/* printf("Car %d dequeued from Lane %d.\n", car->index, getLaneIndex(car)); */
}/*}}}*/

void runStopSignCar(Car* car, SafeStopSign* sign) {/*{{{*/
	addCarToLaneQueue(car, sign);
	/* printf("Car %d entered Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid); */

	goThroughStopSignValid(car, sign);

	exitIntersectionValid(car, sign);
}/*}}}*/
