/* include {{{ */

#pragma once

#include "car.h"
#include "stopSign.h"

/* }}} include */

typedef struct _LaneNode {/*{{{*/
	Car* car;
	struct _LaneNode* next;
} LaneNode;/*}}}*/

typedef struct _LaneQueue {/*{{{*/
	int count;
	int total_count;
	LaneNode* orig_front;
	LaneNode* cur_front;
	LaneNode* back;
} LaneQueue;/*}}}*/

/* SafeStopSign descr. {{{ */

/**
* @brief Structure that you can modify as part of your solution to implement
* proper synchronization for the stop sign intersection.
*
* This is basically a wrapper around StopSign, since you are not allowed to 
* modify or directly access members of StopSign.
*/

/* }}} SafeStopSign descr. */
typedef struct _SafeStopSign {/*{{{*/
	/* instr. {{{ */
	
	/**
	* @brief The underlying stop sign.
	*
	* You are not allowed to modify the underlying stop sign or directly
	* access its members. All interactions must be done through the functions
	* you have been provided.
	*/
	
	/* }}} instr. */
	StopSign base;

	// need mutex, cv and queue for each direction
	LaneQueue lane_queue[DIRECTION_COUNT];
	// used for mutex of LaneQueue 
	pthread_mutex_t lane_mutex[DIRECTION_COUNT];
	pthread_cond_t lane_turn[DIRECTION_COUNT];

	// used for setting/checking quadrants
	pthread_mutex_t quadrants_mutex;
	int busy_quadrants[QUADRANT_COUNT];
	// used for waiting for busy quadrants
	pthread_cond_t quadrants_turn;
} SafeStopSign;/*}}}*/

/* fxn def's {{{ */

/**
* @brief Initializes the safe stop sign.
*
* @param sign pointer to the instance of SafeStopSign to be initialized.
* @param carCount number of cars in the simulation.
*/
void initSafeStopSign(SafeStopSign* sign, int carCount);

/**
* @brief Destroys the safe stop sign.
*
* @param sign pointer to the instance of SafeStopSign to be freed
*/
void destroySafeStopSign(SafeStopSign* sign);

/**
* @brief Runs a car-thread in a stop-sign scenario.
*
* @param car pointer to the car.
* @param sign pointer to the stop sign intersection.
*/
void runStopSignCar(Car* car, SafeStopSign* sign);

/* }}} fxn def's */
