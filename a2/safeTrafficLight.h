#pragma once
#include "car.h"
#include "trafficLight.h"
#include "safeStopSign.h"

// imported from safeStopSign.h
/* typedef struct _LaneNode {
	Car* car;
	struct _LaneNode* next;
} LaneNode;

typedef struct _LaneQueue {
	int count;
	LaneNode* orig_front;
	LaneNode* cur_front;
	LaneNode* back;
} LaneQueue; */

/* SafeTrafficLight instr. {{{ */

/**
* @brief Structure that you can modify as part of your solution to implement
* proper synchronization for the traffic light intersection.
*
* This is basically a wrapper around TrafficLight, since you are not allowed to 
* modify or directly access members of TrafficLight.
*/

/* }}} SafeTrafficLight instr. */
typedef struct _SafeTrafficLight {

	/* instr. {{{ */
	
	/**
	* @brief The underlying light.
	*
	* You are not allowed to modify the underlying traffic light or directly
	* access its members. All interactions must be done through the functions
	* you have been provided.
	*/
	
	/* }}} instr. */
	TrafficLight base;

	// TODO: Add any members you need for synchronization here.
	// need mutex and queue for each direction
	LaneQueue lane_queue[TRAFFIC_LIGHT_LANE_COUNT];
	// used for mutex of LaneQueue 
	pthread_mutex_t lane_mutex[TRAFFIC_LIGHT_LANE_COUNT];
} SafeTrafficLight;

/**
* @brief Initializes the safe traffic light.
*
* @param light pointer to the instance of SafeTrafficLight to be initialized.
* @param eastWest total number of cars moving east-west.
* @param northSouth total number of cars moving north-south.
*/
void initSafeTrafficLight(SafeTrafficLight* light, int eastWest, int northSouth);

/**
* @brief Destroys the safe traffic light.
*
* @param light pointer to the instance of SafeTrafficLight to be destroyed.
*/
void destroySafeTrafficLight(SafeTrafficLight* light);

/**
* @brief Runs a car-thread in a traffic light scenario.
*
* @param car pointer to the car.
* @param light pointer to the traffic light intersection.
*/
void runTrafficLightCar(Car* car, SafeTrafficLight* light);
