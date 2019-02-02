/**
* CSC369 Assignment 2
*
* This is the source/implementation file for your safe traffic light 
* submission code.
*/
#include "safeTrafficLight.h"

void initSafeTrafficLight(SafeTrafficLight* light, int horizontal, int vertical) {
	initTrafficLight(&light->base, horizontal, vertical);

	// TODO: Add any initialization logic you need.
}

void destroySafeTrafficLight(SafeTrafficLight* light) {
	destroyTrafficLight(&light->base);

	// TODO: Add any logic you need to free data structures
}

void runTrafficLightCar(Car* car, SafeTrafficLight* light) {

	// TODO: Add your synchronization logic to this function.

	EntryLane* lane = getLaneLight(car, &light->base);
	enterLane(car, lane);

	// Enter and act are separate calls because a car turning left can first
	// enter the intersection before it needs to check for oncoming traffic.
	enterTrafficLight(car, &light->base);
	actTrafficLight(car, &light->base, NULL, NULL, NULL);

	exitIntersection(car, lane);

}
