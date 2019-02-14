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
	for(int i = 0; i < DIRECTION_COUNT; i++) {
		cur_front = sign->lane_queue[i].orig_front;
		while (cur_front != NULL) {
			next_front = cur_front->next;
			free(cur_front);
			cur_front = next_front;
		}
	}
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
		/* link prev. back to this car if not orig_front to keep list
		 * linked */
		} else {
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
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);
}/*}}}*/

void dequeueFront(SafeStopSign* sign, int lane_index)/*{{{*/
{
	LaneQueue* cur_lane_queue = &sign->lane_queue[lane_index];

	pthread_mutex_lock(&sign->lane_mutex[lane_index]);
	if (cur_lane_queue->count == 0) { 
		return;	
	}
	if (cur_lane_queue->count == 1) {
		cur_lane_queue->cur_front = NULL;
	} else {
		cur_lane_queue->cur_front = cur_lane_queue->cur_front->next;
	}
	cur_lane_queue->count--;
	/* wake up cars waiting to enter intersection due to not being in front */
	pthread_cond_broadcast(&sign->lane_turn[lane_index]);
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);
}/*}}}*/

void waitForFrontOfQueue(Car* car, SafeStopSign* sign)/*{{{*/
{
	int lane_index = getLaneIndex(car);
	/* check if car is in front of queue before going through stop sign */
	pthread_mutex_lock(&sign->lane_mutex[lane_index]);
	while (sign->lane_queue[lane_index].cur_front->car != car) {
		pthread_cond_wait(&sign->lane_turn[lane_index],
				&sign->lane_mutex[lane_index]);
	}
	pthread_mutex_unlock(&sign->lane_mutex[lane_index]);
}/*}}}*/

int checkIfQuadrantsSafe(Car* car, SafeStopSign* sign) {/*{{{*/
	/* assume if called this function that have quadrants_mutex */
	int car_quadrants[QUADRANT_COUNT];
	int quadrant_count = getStopSignRequiredQuadrants(car, car_quadrants);
	for (int i = 0; i < quadrant_count; i++) {
		if (sign->busy_quadrants[car_quadrants[i]] == 1) { 
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
	for (i = 0; i < quadrant_count; i++) {
		sign->busy_quadrants[car_quadrants[i]] = value;
	}
}/*}}}*/

void goThroughStopSignValid(Car* car, SafeStopSign* sign) {/*{{{*/
	waitForFrontOfQueue(car, sign);

	pthread_mutex_lock(&sign->quadrants_mutex);
	while (checkIfQuadrantsSafe(car, sign) == 0) {
		pthread_cond_wait(&sign->quadrants_turn, &sign->quadrants_mutex);
	}
	/* assume at this point quadrants are free. */
	setCarQuadrants(car, sign, 1);
	pthread_mutex_unlock(&sign->quadrants_mutex);

	/* go through stop sign and broadcast to others to go after */
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

	dequeueFront(sign, lane_index);
}/*}}}*/

void runStopSignCar(Car* car, SafeStopSign* sign) {/*{{{*/
	addCarToLaneQueue(car, sign);

	goThroughStopSignValid(car, sign);

	exitIntersectionValid(car, sign);
}/*}}}*/
