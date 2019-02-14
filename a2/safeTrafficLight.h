#pragma once
#include "car.h"
#include "trafficLight.h"
#include "safeStopSign.h"

// imports LaneNode/Queue from safeStopSign.h

/**
* @brief Structure that you can modify as part of your solution to implement
* proper synchronization for the traffic light intersection.
*
* This is basically a wrapper around TrafficLight, since you are not allowed to 
* modify or directly access members of TrafficLight.
*/
typedef struct _SafeTrafficLight {/*{{{*/
	/**
	* @brief The underlying light.
	*
	* You are not allowed to modify the underlying traffic light or directly
	* access its members. All interactions must be done through the functions
	* you have been provided.
	*/
	TrafficLight base;

	// need mutex and queue for each direction
	LaneQueue lane_queue[TRAFFIC_LIGHT_LANE_COUNT];
	// used for mutex of LaneQueue 
	pthread_mutex_t lane_mutex[TRAFFIC_LIGHT_LANE_COUNT];
	pthread_cond_t lane_turn[TRAFFIC_LIGHT_LANE_COUNT];

	// used for sync any time intersection events happen. 
	pthread_mutex_t light_mutex;
	pthread_cond_t light_turn;
} SafeTrafficLight;/*}}}*/

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

/**
* @brief Free memory allocated to LaneNodes.
*
* @param light pointer to the traffic light intersection.
*/
void destroyLaneNodesLight(SafeTrafficLight* light);

/**
* @brief Add car to their respective LaneQueue.
*
* @param car pointer to the car.
* @param light pointer to the traffic light intersection.
*/
void addCarToLaneLight(Car* car, SafeTrafficLight* light);

/**
* @brief Check if this car is in the front of its LaneQueue or else wait.
*
* @param car pointer to the car.
* @param light pointer to the traffic light intersection.
*/
void waitForFrontLight(Car* car, SafeTrafficLight* light);

/**
* @brief Return the LightState associated with the car's position.
*
* @param car pointer to the car.
* @return LightState corresponding to car's position.
*/
int getLightStateForCar(Car* car);

/**
* @brief Have this car wait until the opposite lane has no cars going straight.
*
* @param car pointer to the car.
* @param light pointer to the traffic light intersection.
*/
void waitForOppStraight(Car* car, SafeTrafficLight* light);

/**
* @brief Have this car wait until the correct light before entering the
* intersection.
*
* @param car pointer to the car.
* @param light pointer to the traffic light intersection.
*/
void enterWhenCorrectLight(Car* car, SafeTrafficLight* light);

/**
* @brief Have this car act when safe.
*
* @param car pointer to the car.
* @param light pointer to the traffic light intersection.
*/
void actTrafficLightValid(Car* car, SafeTrafficLight* light);

/**
* @brief Advances the front of the LaneQueue to the next car.
*
* @param light pointer to the traffic light intersection.
* @param lane_index index of the LaneQueue.
*/
void dequeueFrontLight(SafeTrafficLight* light, int lane_index);

/**
* @brief Have the car exit the intersection (sync'ed), along with other required
* caretaking duties.
*
* @param car pointer to the car.
* @param light pointer to the traffic light intersection.
*/
void exitIntersectionLightValid(Car* car, SafeTrafficLight* light);
