/**
* CSC369 Assignment 2 - Don't modify this file!
*
* This is the implementation for intersection logic.
*/
#include "intersection.h"
#include "common.h"
#include "car.h"

#include "stopSign.h"
#include "trafficLight.h"

void initToken(CarToken* carToken, Car* car, int tokenValue) {
	assert(!carToken->valid);

	carToken->carCopy = *car;
	carToken->token = tokenValue;
	carToken->valid = TRUE;
}

void enterLane(Car* car, EntryLane* lane) {
	int token = lane->enterCounter;

	int duration = (rand() % 750) + 250;
	nap(duration);
	lane->enterCounter = token + 1;

	/* printf("Car %d entering Lane %d with token %d.\n", 
			car->index, getLaneIndexLight(car), 
			lane->enterTokens[car->index].valid); */
	initToken(&lane->enterTokens[car->index], car, token);
}

void exitIntersection(Car* car, EntryLane* lane) {
	int duration = (rand() % 500) - 250;
	nap(duration);

	if (!lane->enterTokens[car->index].valid) {
		fprintf(stderr, "ERROR: Car %d did not enter lane %d @ %s : %s\n",
				car->index, getLaneIndex(car), __FILE__, LINE_STRING);
		/* fprintf(stderr, "@ " __FILE__ " : " LINE_STRING "\n"); */
	}

	// Car must exit in the same order it entered.
	int enterToken = lane->enterTokens[car->index].token;
	int exitToken = lane->exitCounter++;
	if (enterToken != exitToken) {

		/* fprintf(stderr, "Car did not exit in the same order as it entered "\
				"@ " __FILE__ " : " LINE_STRING "\n"); */
		/* fprintf(stderr, "ERROR: Car %d did not exit lane %d in the same order as it entered @ %s : %s", */
		fprintf(stderr, "ERROR: Car %d did not exit Lane %d in the same order as it entered. ",
				car->index, getLaneIndex(car));
		fprintf(stderr, "It had enter token %d and exit token %d.\n",
				enterToken, exitToken);
	} else {
		initToken(&lane->exitTokens[car->index], car, exitToken);
		/* printf("Car %d exited Lane %d with token %d and had enter token %d.\n",
				car->index, getLaneIndex(car), exitToken, enterToken); */
	}
}
