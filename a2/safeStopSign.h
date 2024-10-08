#pragma once

#include "car.h"
#include "stopSign.h"

typedef struct _LaneNode {/*{{{*/
	Car* car;
	struct _LaneNode* next;
} LaneNode;/*}}}*/

typedef struct _LaneQueue {/*{{{*/
	int count;
	LaneNode* orig_front;
	LaneNode* cur_front;
	LaneNode* back;
} LaneQueue;/*}}}*/

/**
* @brief Structure that you can modify as part of your solution to implement
* proper synchronization for the stop sign intersection.
*
* This is basically a wrapper around StopSign, since you are not allowed to 
* modify or directly access members of StopSign.
*/
typedef struct _SafeStopSign {/*{{{*/
	/**
	* @brief The underlying stop sign.
	*
	* You are not allowed to modify the underlying stop sign or directly
	* access its members. All interactions must be done through the functions
	* you have been provided.
	*/
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

/**
* @brief Free memory allocated to LaneNodes.
*
* @param sign pointer to the stop sign intersection.
*/
void destroyLaneNodes(SafeStopSign* sign);

/**
* @brief Add car to their respective LaneQueue.
*
* @param car pointer to the car.
* @param sign pointer to the stop sign intersection.
*/
void addCarToLaneQueue(Car* car, SafeStopSign* sign);

/**
* @brief Advances the front of the LaneQueue to the next car.
*
* @param sign pointer to the stop sign intersection.
* @param lane_index index of the LaneQueue.
*/
void dequeueFront(SafeStopSign* sign, int lane_index);

/**
* @brief Check if this car is in the front of its LaneQueue or else wait.
*
* @param car pointer to the car.
* @param sign pointer to the stop sign intersection.
*/
void waitForFrontOfQueue(Car* car, SafeStopSign* sign);

/**
* @brief Check if this car can safely go through the intersection.
*
* @param car pointer to the car.
* @param sign pointer to the stop sign intersection.
* @return 1 if safe, else 0.
*/
int checkIfQuadrantsSafe(Car* car, SafeStopSign* sign);

/**
* @brief Set this car's action's quadrants to specified value.
*
* @param car pointer to the car.
* @param sign pointer to the stop sign intersection.
* @param value value to set the quadrants to.
*/
void setCarQuadrants(Car* car, SafeStopSign* sign, int value);

/**
* @brief Have the car go through the intersection safely.
*
* @param car pointer to the car.
* @param sign pointer to the stop sign intersection.
*/
void goThroughStopSignValid(Car* car, SafeStopSign* sign);

/**
* @brief Have the car exit the intersection, along with other required caretaking
* duties.
*
* @param car pointer to the car.
* @param sign pointer to the stop sign intersection.
*/
void exitIntersectionValid(Car* car, SafeStopSign* sign);
