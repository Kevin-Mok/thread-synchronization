#include "safeTrafficLight.h"/*{{{*/
/* #include "safeStopSign.h" */
#include "car.h"
#include "trafficLight.h"/*}}}*/

void initSafeTrafficLight(SafeTrafficLight* light, int horizontal, int vertical) {/*{{{*/
	int i;
	initTrafficLight(&light->base, horizontal, vertical);

	for(i = 0; i < TRAFFIC_LIGHT_LANE_COUNT; i++) {
		/* init LaneQueue */
		light->lane_queue[i].count = 0;
		light->lane_queue[i].orig_front = malloc(sizeof(LaneNode));
		light->lane_queue[i].cur_front = malloc(sizeof(LaneNode));
		light->lane_queue[i].back = malloc(sizeof(LaneNode));

		light->lane_mutex[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		/* light->lane_turn[i] = (pthread_cond_t)PTHREAD_COND_INITIALIZER; */
	}
	light->light_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	light->light_turn = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}/*}}}*/

void destroySafeTrafficLight(SafeTrafficLight* light) {/*{{{*/
	destroyTrafficLight(&light->base);

	/* TODO: free malloc's */
}/*}}}*/

void addCarToLaneLight(Car* car, SafeTrafficLight* light)/*{{{*/
{
	int lane_index = getLaneIndexLight(car);
	/* EntryLane* lane = getLaneLight(car, &light->base); */

	/* create LaneNode for current Car */
	LaneNode* cur_car_node = malloc(sizeof(LaneNode*));
	cur_car_node->car = malloc(sizeof(Car*));
	cur_car_node->next = malloc(sizeof(LaneNode*));
	cur_car_node->car = car;

	LaneQueue* cur_lane_queue = &light->lane_queue[lane_index];
	/* add cur_car_node to cur_lane_queue after acquiring lock for cur lane*/
	pthread_mutex_lock(&light->lane_mutex[lane_index]);
	if (cur_lane_queue->count == 0) {
		cur_lane_queue->orig_front = cur_car_node;
		cur_lane_queue->cur_front = cur_car_node;
		cur_lane_queue->back = cur_car_node;
	} else {
		cur_lane_queue->back->next = cur_car_node;
		cur_lane_queue->back = cur_car_node;
	}
	cur_lane_queue->count++;
	pthread_mutex_lock(&light->light_mutex);
	enterLane(car, getLaneLight(car, &light->base));
	pthread_mutex_unlock(&light->light_mutex);
	/* enterLane(car, getLane(car, &light->base)); */
	/* printf("Added car %d to LaneQueue %d at pos %d.\n", car->index, lane_index,
			cur_lane_queue->count - 1); */
	pthread_mutex_unlock(&light->lane_mutex[lane_index]);
}/*}}}*/

void waitForFrontLight(Car* car, SafeTrafficLight* light) {/*{{{*/
	int lane_index = getLaneIndexLight(car);
	/* check if car is in front of queue before entering */
	pthread_mutex_lock(&light->lane_mutex[lane_index]);
	while (light->lane_queue[lane_index].cur_front->car != car) {
		/* printf("Waiting for Car %d in Lane %d to be in front. Currently %d cars with Car %d in front.\n",
				car->index, getLaneIndexLight(car),
				light->lane_queue[lane_index].count,
				light->lane_queue[lane_index].cur_front->car->index); */
		/* printf("Currently %d cars with Car %d in front.\n",
				light->lane_queue[lane_index].count,
				light->lane_queue[lane_index].cur_front->car->index); */
		/* pthread_cond_wait(&light->lane_turn[lane_index],
				&light->lane_mutex[lane_index]); */
		pthread_cond_wait(&light->light_turn, &light->lane_mutex[lane_index]);
	}
	pthread_mutex_unlock(&light->lane_mutex[lane_index]);
}/*}}}*/

int getDirNum(Car* car) {/*{{{*/
	/* helper function to get index num. for light_mutex/turn array, hardcoded values
	based on LightState and CarPosition values */
	/* since E/W = 0/2 and E/W LightState = 1 */
	return (car->position % 2 == 0) ? 1 : 0;
}/*}}}*/

void waitForOppStraight(Car* car, SafeTrafficLight* light) {/*{{{*/
	/* assume have light_mutex when calling this */
	CarPosition opp_dir = getOppositePosition(car->position);
	/* based on given CarAction values */
	/* int opp_straight_lane = opp_dir * 3; */

	while (getStraightCount(&light->base, opp_dir) != 0) {
		/* printf("Waiting for left for Car %d in Lane %d. %d straight cars in opp. lane %d.\n",
				car->index, getLaneIndex(car), 
				getStraightCount(&light->base, opp_dir),
				opp_straight_lane); */
		pthread_cond_wait(&light->light_turn, &light->light_mutex);
	}
}/*}}}*/

void enterWhenCorrectLight(Car* car, SafeTrafficLight* light) {/*{{{*/
	/* check/wait for light to be same dir. as car by waiting for
	 * light_turn signal since light could change after every car */
	pthread_mutex_lock(&light->light_mutex);
	while (getLightState(&light->base) != getDirNum(car)) {
		/* printf("Waiting for Light %d for Car %d in Lane %d.\n",
				getLightState(&light->base), car->index, getLaneIndex(car)); */
		pthread_cond_wait(&light->light_turn, &light->light_mutex);
	}
	/* printf("Car %d attempting to enter Lane %d while Light is %d.\n", 
			car->index, getLaneIndexLight(car), getLightState(&light->base)); */
	enterTrafficLight(car, &light->base);
	pthread_mutex_unlock(&light->light_mutex);
}/*}}}*/

void enterTrafficLightValid(Car* car, SafeTrafficLight* light) {/*{{{*/
	waitForFrontLight(car, light);
	enterWhenCorrectLight(car, light);
}/*}}}*/

void actTrafficLightValid(Car* car, SafeTrafficLight* light)/*{{{*/
{
	pthread_mutex_lock(&light->light_mutex);
	/* only wait to act if car is going left */
	if (car->action == LEFT_TURN) {
		while (getStraightCount(
					&light->base, getOppositePosition(car->position)) != 0) {
			/* printf("Waiting for left for Car %d in Lane %d. %d straight cars in opp. lane %d.\n",
					car->index, getLaneIndex(car), 
					getStraightCount(&light->base, opp_dir),
					opp_straight_lane); */
			pthread_cond_wait(&light->light_turn, &light->light_mutex);
		}
		/* waitForOppStraight(car, light); */
	}
	actTrafficLight(car, &light->base, NULL, NULL, NULL);
	pthread_mutex_unlock(&light->light_mutex);
}/*}}}*/

void dequeueFrontLight(SafeTrafficLight* light, int lane_index)/*{{{*/
{
	LaneQueue* cur_lane_queue = &light->lane_queue[lane_index];
	/* used for debugging msg */
	/* LaneNode* cur_front = cur_lane_queue->cur_front; */

	pthread_mutex_lock(&light->lane_mutex[lane_index]);
	if (cur_lane_queue->count == 0) { 
		return;	
	}
	if (cur_lane_queue->count == 1) {
		cur_lane_queue->cur_front = NULL;
		cur_lane_queue->back = NULL;
	} else {
		cur_lane_queue->cur_front = cur_lane_queue->cur_front->next;
	}
	cur_lane_queue->count--;
	/* printf("Dequeued Car %d from Lane %d. New count is %d.\n",
			cur_front->car->index, lane_index,
			cur_lane_queue->count); */
	/* pthread_cond_broadcast(&light->lane_turn[lane_index]); */
	/* pthread_cond_broadcast(&light->light_turn); */
	pthread_mutex_unlock(&light->lane_mutex[lane_index]);
}/*}}}*/

void exitIntersectionLightValid(Car* car, SafeTrafficLight* light) {/*{{{*/
	pthread_mutex_lock(&light->light_mutex);
	exitIntersection(car, getLaneLight(car, &light->base));
	pthread_mutex_unlock(&light->light_mutex);
	/* printf("Car %d exited from Lane %d. %d cars left in intersection "
			"while Light is %d. %d cars left before light change.\n",
			car->index, getLaneIndexLight(car), light->base.carsInside,
			getLightState(&light->base), light->base.carsLeft); */
	/* printf("Car %d exited from Lane %d. Light is %d and %d "
			"cars left before light change.\n",
			car->index, getLaneIndexLight(car), getLightState(&light->base),
			light->base.carsLeft); */
	dequeueFrontLight(light, getLaneIndexLight(car)); 
	pthread_cond_broadcast(&light->light_turn);
}/*}}}*/

void runTrafficLightCar(Car* car, SafeTrafficLight* light) {/*{{{*/
	addCarToLaneLight(car, light);

	/* Enter and act are separate calls because a car turning left can first
	enter the intersection before it needs to check for oncoming traffic. */
	enterTrafficLightValid(car, light);
	actTrafficLightValid(car, light);
	/* enterAndActLightValid(car, light); */

	exitIntersectionLightValid(car, light);
}/*}}}*/

/* enterAndActLightValid - deprecated {{{ */

void enterAndActLightValid(Car* car, SafeTrafficLight* light) {
	int lane_index = getLaneIndexLight(car);
	/* check if car is in front of queue before entering */
	pthread_mutex_lock(&light->lane_mutex[lane_index]);
	while (light->lane_queue[lane_index].cur_front->car != car) {
		pthread_cond_wait(&light->light_turn, &light->lane_mutex[lane_index]);
	}
	pthread_mutex_unlock(&light->lane_mutex[lane_index]);

	/* check/wait for light to be same dir. as car by waiting for
	 * light_turn signal since light could change after every car */
	pthread_mutex_lock(&light->light_mutex);
	while (getLightState(&light->base) != getDirNum(car)) {
		/* printf("Waiting for Light %d for Car %d in Lane %d.\n",
				getLightState(&light->base), car->index, getLaneIndex(car)); */
		pthread_cond_wait(&light->light_turn, &light->light_mutex);
	}
	/* printf("Car %d entering Lane %d while Light is %d.\n",
			car->index, getLaneIndexLight(car), getLightState(&light->base)); */
	enterTrafficLight(car, &light->base);

	/* trying to give up mutex after each step */
	pthread_mutex_unlock(&light->light_mutex);
	pthread_mutex_lock(&light->light_mutex);

	/* only wait to act if car is going left */
	if (car->action == LEFT_TURN) {
		while (getStraightCount(
					&light->base, getOppositePosition(car->position)) != 0) {
			/* printf("Waiting for left for Car %d in Lane %d. %d straight cars in opp. lane %d.\n",
					car->index, getLaneIndex(car), 
					getStraightCount(&light->base, opp_dir),
					opp_straight_lane); */
			pthread_cond_wait(&light->light_turn, &light->light_mutex);
		}
	}
	actTrafficLight(car, &light->base, NULL, NULL, NULL);
	pthread_mutex_unlock(&light->light_mutex);
}

/* }}} enterAndActLightValid - deprecated */

