/**
* CSC369 Assignment 2
*
* This is the source/implementation file for your safe stop sign
* submission code.
*/
#include "safeStopSign.h"
#include "stopSign.h"
#include "intersection.h"
#include "mutexAccessValidator.h"

void initSafeStopSign(SafeStopSign* sign, int count) {
	int i;
	initStopSign(&sign->base, count);

	for(i = 0; i < DIRECTION_COUNT; i++) {
		(*sign).car_entering[i] = 0;
	}
	(*sign).lane_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	(*sign).lane_turn = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	(*sign).quadrants_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	(*sign).quadrants_turn = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}

void destroySafeStopSign(SafeStopSign* sign) {
	destroyStopSign(&sign->base);

	// TODO: Add any logic you need to clean up data structures.
}

/* enter lane when possible */
/* enterLaneValid {{{ */

void enterLaneValid(Car* car, SafeStopSign* sign) {
	int car_lane_index; 

	pthread_mutex_lock(&sign->lane_mutex);
	car_lane_index = getLaneIndex(car);
	/* wait while other cars entering or someone already entering */
	while(sign->car_entering[car_lane_index] == 1) {
		pthread_cond_wait(&sign->lane_turn, &sign->lane_mutex);
	}
	/* assume at this point lane is free and no other cars entering. then,
	 * enter lane */
	enterLane(car, getLane(car, &sign->base));
	sign->car_entering[car_lane_index] = 1;
	/* allow other cars to try and enter lanes */
	pthread_cond_broadcast(&sign->lane_turn);
	pthread_mutex_unlock(&sign->lane_mutex);
}

/* }}} enterLaneValid */

int checkIfQuadrantsSafe(Car* car, StopSign* intersection) {
	int quadrants[QUADRANT_COUNT];
	int quadrantCount = getStopSignRequiredQuadrants(car, quadrants);
	MutexAccessValidator curValidator;
	for (int i = 0; i < quadrantCount; i++) {
		curValidator = intersection->quadrants[quadrants[i]].validator;
		pthread_mutex_lock(&curValidator.lock);
		if (curValidator.current != NULL) {
			return 0;
		}
		pthread_mutex_unlock(&curValidator.lock);
	}
	return 1;
}

void goThroughStopSignValid(Car* car, SafeStopSign* sign) {
	pthread_mutex_lock(&sign->quadrants_mutex);
	while (checkIfQuadrantsSafe(car, &sign->base) == 0) {
		pthread_cond_wait(&sign->quadrants_turn, &sign->quadrants_mutex);
	}
	/* assume at this point quadrants are free. */

	/* set entering car lane to empty */
	pthread_mutex_lock(&sign->lane_mutex);
	sign->car_entering[getLaneIndex(car)] = 0;
	pthread_cond_broadcast(&sign->lane_turn);
	pthread_mutex_unlock(&sign->lane_mutex);

	/* go through stop sign and broadcast to others to go after */
	goThroughStopSign(car, &sign->base);
	pthread_cond_broadcast(&sign->quadrants_turn);
	pthread_mutex_unlock(&sign->quadrants_mutex);
}

void runStopSignCar(Car* car, SafeStopSign* sign) {
	enterLaneValid(car, sign);

	goThroughStopSignValid(car, sign);

	/* TODO: ensure exit in same order */
	exitIntersection(car, getLane(car, &sign->base));
}
