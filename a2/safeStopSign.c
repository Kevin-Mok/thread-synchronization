/**
* CSC369 Assignment 2
*
* This is the source/implementation file for your safe stop sign
* submission code.
*/
#include "safeStopSign.h"

void initSafeStopSign(SafeStopSign* sign, int count) {
	initStopSign(&sign->base, count);

	// TODO: Add any initialization logic you need.
}

void destroySafeStopSign(SafeStopSign* sign) {
	destroyStopSign(&sign->base);

	// TODO: Add any logic you need to clean up data structures.
}

void runStopSignCar(Car* car, SafeStopSign* sign) {

	// TODO: Add your synchronization logic to this function.

	EntryLane* lane = getLane(car, &sign->base);
	enterLane(car, lane);

	goThroughStopSign(car, &sign->base);

	exitIntersection(car, lane);
}
