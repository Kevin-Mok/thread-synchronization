/**
* CSC369 Assignment 2 - This file may be replaced when we run your code.
*
* This contains the implementation of the (unsafe) stop sign.
*/
#include "stopSign.h"

void initStopSign(StopSign* stopSign, int carCount) {

	// Initialize lanes.
	for (int i = 0; i < DIRECTION_COUNT; i++) {
		size_t size = sizeof(CarToken) * carCount;
		stopSign->entryLanes[i].enterTokens = (CarToken*)malloc(size);
		memset(stopSign->entryLanes[i].enterTokens, 0, size);
		stopSign->entryLanes[i].exitTokens = (CarToken*)malloc(size);
		memset(stopSign->entryLanes[i].exitTokens, 0, size);

		stopSign->entryLanes[i].enterCounter = 0;
		stopSign->entryLanes[i].exitCounter = 0;
	}

	// Initialize tokens for the intersection.
	size_t size = sizeof(CarToken) * carCount;
	stopSign->tokens = (CarToken*)malloc(size);
	memset(stopSign->tokens, 0, size);

	stopSign->tokenCounter = 0;

	// Initialize quadrants of the intersection.
	for (int i = 0; i < QUADRANT_COUNT; i++) {
		initMutexAccessValidator(&stopSign->quadrants[i].validator);
	}
}

void destroyStopSign(StopSign* stopSign) {

	// free lanes.
	for (int i = 0; i < DIRECTION_COUNT; i++) {
		free(stopSign->entryLanes[i].enterTokens);
		free(stopSign->entryLanes[i].exitTokens);
	}

	free(stopSign->tokens);

	// free quadrants of the intersection.
	for (int i = 0; i < QUADRANT_COUNT; i++) {
		destructMutexAccessValidator(&stopSign->quadrants[i].validator);
	}

}

int getLaneIndex(Car* car) {
	return car->position;
}

EntryLane* getLane(Car* car, StopSign* intersection) {
	int laneIndex = getLaneIndex(car);
	return &intersection->entryLanes[laneIndex];
}

void goThroughStopSign(Car* car, StopSign* intersection) {

	// Mark down that this car is travelling through each quadrant
	int quadrants[QUADRANT_COUNT];
	int quadrantCount = getStopSignRequiredQuadrants(car, quadrants);
	for (int i = 0; i < quadrantCount; i++) {
		enterMutexAccessValidator(&intersection->quadrants[i].validator, car);
	}

	// Sleep, representing the car moving through the intersection.
	nap(3000);

	// Collect a token for passing through the intersection.
	EntryLane* lane = getLane(car, intersection);
	if (!lane->enterTokens[car->index].valid || 
			lane->exitTokens[car->index].valid) {
		fprintf(stderr, "Car either has not entered a lane or has already exited. "\
				"@ " __FILE__ " : " LINE_STRING "\n");
	}
	int token = intersection->tokenCounter++;
	initToken(&intersection->tokens[car->index], car, token);

	// Mark down that this car is is done travelling through each quadrant.
	for (int i = 0; i < quadrantCount; i++) {
		exitMutexAccessValidator(&intersection->quadrants[i].validator, car);
	}
}

int getStopSignRequiredQuadrants(Car* car, int* quadrants) {

	// Assume its going east. We can rotate after.
	int quadrantCount;
	if (car->action == LEFT_TURN) {
		quadrantCount = 3;
		quadrants[0] = 2;
		quadrants[1] = 3;
		quadrants[2] = 1;
	} else if (car->action == RIGHT_TURN) {
		quadrantCount = 1;
		quadrants[0] = 2;
	} else if (car->action == STRAIGHT) {
		quadrantCount = 2;
		quadrants[0] = 2;
		quadrants[1] = 3;
	} else {

		// Should not go here.
		assert(FALSE);
	}

	// Now rotate.
	int rotation = (int)car->position;
	for (int i = 0; i < quadrantCount; i++) {
		quadrants[i] = (quadrants[i] + rotation) % DIRECTION_COUNT;
	}

	return quadrantCount;
}
