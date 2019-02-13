#include "safeStopSign.h"/*{{{*/
#include "stopSign.h"
#include "intersection.h"
#include "mutexAccessValidator.h"/*}}}*/

void initSafeStopSign(SafeStopSign* sign, int count) {/*{{{*/
	int i;
	initStopSign(&sign->base, count);

	/* sign->lane_queue = (LaneQueue*)malloc(sizeof(LaneQueue) * DIRECTION_COUNT); */
	
	for(i = 0; i < DIRECTION_COUNT; i++) {
		/* init LaneQueue */
		sign->lane_queue[i].count = 0;
		sign->lane_queue[i].total_count = 0;
		/* don't need to malloc mem for pointers in LaneQueue struct since will
		 * malloc when creating LaneNode */
		/* sign->lane_queue[i].orig_front = (LaneNode*)malloc(sizeof(LaneNode));
		sign->lane_queue[i].cur_front = (LaneNode*)malloc(sizeof(LaneNode));
		sign->lane_queue[i].back = (LaneNode*)malloc(sizeof(LaneNode)); */
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

void destroyMyMallocs(SafeStopSign* sign) {/*{{{*/
	/* TODO: freeing malloc's */
	LaneNode* cur_front;
	LaneNode* next_front;
	int cars_freed = 0;
	for(int i = 0; i < DIRECTION_COUNT; i++) {
		cur_front = sign->lane_queue[i].orig_front;
		/* while (cur_front != (*sign).lane_queue[i].back) { */
		while (cur_front != NULL) {
			next_front = cur_front->next;
			/* printf("next_front is: %p\n", next_front); */
			/* don't need to free cars? */
			/* if (cur_front->car != NULL) {
				printf("Freeing car.\n");
				free(cur_front->car);
				cars_freed++;
			} */
			/* don't free next since will free in next loop */
			/* if (cur_front->next != NULL) {
				[>printf("Freeing next.\n");<]
				free(cur_front->next);
			} */
			/* printf("Freeing cur_front.\n"); */

			if (cur_front->car != NULL) {
				cars_freed++;
				/* printf("Freeing front %p with Car %d at %p.\n", cur_front,
						cur_front->car->index, cur_front->car); */
			} else {
				printf("Freeing front %p with no car at %p.\n", cur_front,
						cur_front->car);
			}
			free(cur_front);
			cur_front = next_front;
			/* printf("cur_front is %p at end of loop.\n", cur_front); */
			/* if (cur_front == NULL) {
				printf("cur_front is now null.\n");
			} */
		}
	}
	printf("%d cars freed.\n", cars_freed);
}/*}}}*/

void destroyMyMallocs2(SafeStopSign* sign) {/*{{{*/
	LaneNode* cur_node;
	LaneNode* next_node;
	int cars_to_free;
	int cars_freed = 0;
	for(int i = 0; i < DIRECTION_COUNT; i++) {
		cars_to_free = sign->lane_queue[i].total_count;
		printf("Lane %d - %d to free.\n", i, cars_to_free);
		cur_node = sign->lane_queue[i].orig_front;
		/* printf("orig_front: Car %d.\n", sign->lane_queue[i].orig_front->car->index); */
		for (int j = 0; j < cars_to_free; j++) {
			/* printf("Free #%d - Car %d.\n", j, cur_node->car->index); */
			next_node = cur_node->next;
			free(cur_node);
			cur_node = next_node;
			cars_freed++;
		}
	}
	printf("%d cars freed.\n", cars_freed);
}/*}}}*/

void destroySafeStopSign(SafeStopSign* sign) {/*{{{*/
	destroyMyMallocs(sign);
	/* destroyMyMallocs2(sign); */
	destroyStopSign(&sign->base);
}/*}}}*/

void addCarToLaneQueue(Car* car, SafeStopSign* sign)/*{{{*/
{
	int lane_index = getLaneIndex(car);
	/* create LaneNode for current Car */
	LaneNode* cur_car_node = (LaneNode*)malloc(sizeof(LaneNode));
	/* do I need to malloc for their car? */
	/* cur_car_node->car = (Car*)malloc(sizeof(Car)); */
	/* cur_car_node->next = (LaneNode*)malloc(sizeof(LaneNode)); */
	cur_car_node->car = car;
	cur_car_node->next = NULL;

	LaneQueue* cur_lane_queue = &sign->lane_queue[lane_index];
	/* add cur_car_node to cur_lane_queue after acquiring lock for cur lane*/
	pthread_mutex_lock(&sign->lane_mutex[lane_index]);
	/* printf("Adding Car %d (addr. %p) to LaneQueue %d at pos %d. "
			"total_count is %d.\n", car->index,
			car, lane_index, cur_lane_queue->count,
			cur_lane_queue->total_count); */
	if (cur_lane_queue->count == 0) {
		/* check if first car for this lane to set orig_front */
		if (cur_lane_queue->orig_front == NULL) {
		/* if (cur_lane_queue->total_count == 0) { */
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
	cur_lane_queue->total_count++;
	enterLane(car, getLane(car, &sign->base));
	/* printf("Added Car %d (addr. %p) to LaneQueue %d at pos %d. "
			"orig_front is Car %d.\n", car->index,
			car, lane_index, cur_lane_queue->count - 1,
			cur_lane_queue->orig_front->car->index); */
	/* printf("Added Car %d (addr. %p) to LaneQueue %d at pos %d. "
			"total_count is %d.\n", car->index,
			car, lane_index, cur_lane_queue->count,
			cur_lane_queue->total_count); */
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
		/* cur_lane_queue->back = NULL; */
	} else {
		cur_lane_queue->cur_front = cur_lane_queue->cur_front->next;
	}
	cur_lane_queue->count--;
	/* printf("Dequeued Car %d from Lane %d. New count is %d.\n",
			cur_front->car->index, lane_index,
			cur_lane_queue->count); */
	/* printf("Car %d has token value %d after dequeuing car.\n",
			cur_front->car->index, *cur_front_token); */
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
	int carQuadrants[QUADRANT_COUNT];
	int quadrantCount = getStopSignRequiredQuadrants(car, carQuadrants);
	for (int i = 0; i < quadrantCount; i++) {
		if (sign->busy_quadrants[carQuadrants[i]] == 1) { 
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
	int carQuadrants[QUADRANT_COUNT];
	int quadrantCount = getStopSignRequiredQuadrants(car, carQuadrants);
	/* printf("Setting Car %d quadrants to %d.\n", car->index, value); */
	for (i = 0; i < quadrantCount; i++) {
		sign->busy_quadrants[carQuadrants[i]] = value;
	}

	/* check if simul. cars in intersection  {{{ */
	
	/* if (value == 1) {
		int j;
		for (i = 0; i < QUADRANT_COUNT; i++) { */
			/* old {{{ */
			
			/* match = 0;
			if (sign->busy_quadrants[carQuadrants[i]] == 1) {
				for (j = 0; j < quadrantCount; j++) {
					if (i == carQuadrants[j]) {
						match = 1;
						break;
					}
				}
				if (match == 0) {
					printf("Simul. cars in intersection: Car %d taking Action %d but Quadrant %d \n");
					break;
				}
			} */
			
			/* }}} old */
			/* printf("%d", sign->busy_quadrants[i]);
			for (j = 0; j < quadrantCount; j++) {
				if (carQuadrants[j] == i) {
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

	exitIntersection(car, getLane(car, &sign->base));
	/* printf("Car %d exited from Lane %d.\n", car->index, getLaneIndex(car)); */
	/* printf("Car %d went through Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid); */

	dequeueFront(sign, lane_index);
	/*printf("Car %d dequeued from Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid);*/
	/* printf("Car %d dequeued from Lane %d.\n", car->index, getLaneIndex(car)); */

	/* free car quadrants */
	pthread_mutex_lock(&sign->quadrants_mutex);
	setCarQuadrants(car, sign, 0);
	/* pthread_mutex_unlock(&sign->quadrants_mutex); */

	/* wake up cars waiting to enter intersection due to quadrants being busy */
	pthread_cond_broadcast(&sign->quadrants_turn);
	pthread_mutex_unlock(&sign->quadrants_mutex);

	pthread_mutex_lock(&sign->lane_mutex[lane_index]);
	/* wake up cars waiting to enter intersection due to not being in front */
	pthread_cond_broadcast(&sign->lane_turn[lane_index]);
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);

	/* pthread_cond_broadcast(&sign->lane_turn[lane_index]); */
}/*}}}*/

void runStopSignCar(Car* car, SafeStopSign* sign) {/*{{{*/
	addCarToLaneQueue(car, sign);
	/* printf("Car %d entered Lane %d and has token value %d.\n", car->index,
			getLaneIndex(car),
			getLane(car, &sign->base)->enterTokens[car->index].valid); */

	goThroughStopSignValid(car, sign);

	exitIntersectionValid(car, sign);
}/*}}}*/
