#include "safeTrafficLight.h"/*{{{*/
#include "car.h"
#include "trafficLight.h"/*}}}*/

void initSafeTrafficLight(SafeTrafficLight* light, int horizontal, int vertical) {/*{{{*/
	int i;
	initTrafficLight(&light->base, horizontal, vertical);

	for(i = 0; i < TRAFFIC_LIGHT_LANE_COUNT; i++) {
		/* init LaneQueue */
		light->lane_queue[i].count = 0;
		/* don't need to malloc mem for pointers in LaneQueue struct since will
		 * malloc when creating LaneNode */
		light->lane_queue[i].orig_front = NULL;

		light->lane_mutex[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		light->lane_turn[i] = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	}
	light->light_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	light->light_turn = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}/*}}}*/

void destroyLaneNodesLight(SafeTrafficLight* light) {/*{{{*/
	/* only need to free LaneNode's inside LaneQueue since all other var's will
	 * be freed in testing when this light pointer is freed */
	LaneNode* cur_front;
	LaneNode* next_front;
	for(int i = 0; i < TRAFFIC_LIGHT_LANE_COUNT; i++) {
		cur_front = light->lane_queue[i].orig_front;
		while (cur_front != NULL) {
			next_front = cur_front->next;
			free(cur_front);
			cur_front = next_front;
		}
	}
}/*}}}*/

void destroySafeTrafficLight(SafeTrafficLight* light) {/*{{{*/
	destroyTrafficLight(&light->base);
	destroyLaneNodesLight(light);
}/*}}}*/

void addCarToLaneLight(Car* car, SafeTrafficLight* light)/*{{{*/
{
	int lane_index = getLaneIndexLight(car);

	/* create LaneNode for current Car */
	LaneNode* cur_car_node = (LaneNode*)malloc(sizeof(LaneNode));
	cur_car_node->car = car;
	cur_car_node->next = NULL;

	LaneQueue* cur_lane_queue = &light->lane_queue[lane_index];
	/* add cur_car_node to cur_lane_queue after acquiring lock for cur lane*/
	pthread_mutex_lock(&light->lane_mutex[lane_index]);
	if (cur_lane_queue->count == 0) {
		if (cur_lane_queue->orig_front == NULL) {
			cur_lane_queue->orig_front = cur_car_node;
		/* link prev. back to this car if not orig_front to keep list
		 * linked */
		} else {
			cur_lane_queue->back->next = cur_car_node;
		}
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
	pthread_mutex_unlock(&light->lane_mutex[lane_index]);
}/*}}}*/

void waitForFrontLight(Car* car, SafeTrafficLight* light) {/*{{{*/
	int lane_index = getLaneIndexLight(car);
	/* check if car is in front of queue before entering */
	pthread_mutex_lock(&light->lane_mutex[lane_index]);
	while (light->lane_queue[lane_index].cur_front->car != car) {
		pthread_cond_wait(&light->lane_turn[lane_index],
				&light->lane_mutex[lane_index]);
	}
	pthread_mutex_unlock(&light->lane_mutex[lane_index]);
}/*}}}*/

int getLightStateForCar(Car* car) {/*{{{*/
	return (car->position == EAST || car->position == WEST) ?
		EAST_WEST : NORTH_SOUTH;
}/*}}}*/

void enterWhenCorrectLight(Car* car, SafeTrafficLight* light) {/*{{{*/
	/* check/wait for light to be same dir. as car by waiting for
	 * light_turn signal since light could change after every car */
	pthread_mutex_lock(&light->light_mutex);
	while (getLightState(&light->base) != getLightStateForCar(car)) {
		pthread_cond_wait(&light->light_turn, &light->light_mutex);
	}
	enterTrafficLight(car, &light->base);
	pthread_mutex_unlock(&light->light_mutex);
}/*}}}*/

void actTrafficLightValid(Car* car, SafeTrafficLight* light)/*{{{*/
{
	pthread_mutex_lock(&light->light_mutex);
	/* only wait to act if car is going left */
	if (car->action == LEFT_TURN) {
		while (getStraightCount(
					&light->base, getOppositePosition(car->position)) != 0) {
			pthread_cond_wait(&light->light_turn, &light->light_mutex);
		}
	}
	actTrafficLight(car, &light->base, (void *)&pthread_mutex_unlock, 
			(void *)&pthread_mutex_lock, &light->light_mutex);
	pthread_mutex_unlock(&light->light_mutex);
}/*}}}*/

void dequeueFrontLight(SafeTrafficLight* light, int lane_index)/*{{{*/
{
	LaneQueue* cur_lane_queue = &light->lane_queue[lane_index];

	pthread_mutex_lock(&light->lane_mutex[lane_index]);
	if (cur_lane_queue->count == 0) { 
		return;	
	}
	if (cur_lane_queue->count == 1) {
		cur_lane_queue->cur_front = NULL;
	} else {
		cur_lane_queue->cur_front = cur_lane_queue->cur_front->next;
	}
	cur_lane_queue->count--;
	pthread_cond_broadcast(&light->lane_turn[lane_index]);
	pthread_mutex_unlock(&light->lane_mutex[lane_index]);
}/*}}}*/

void exitIntersectionLightValid(Car* car, SafeTrafficLight* light) {/*{{{*/
	pthread_mutex_lock(&light->light_mutex);
	exitIntersection(car, getLaneLight(car, &light->base));
	/* wake up cars waiting for left and light */
	pthread_cond_broadcast(&light->light_turn);
	pthread_mutex_unlock(&light->light_mutex);
	dequeueFrontLight(light, getLaneIndexLight(car)); 
}/*}}}*/

void runTrafficLightCar(Car* car, SafeTrafficLight* light) {/*{{{*/
	addCarToLaneLight(car, light);

	/* Enter and act are separate calls because a car turning left can first
	enter the intersection before it needs to check for oncoming traffic. */
	waitForFrontLight(car, light);
	enterWhenCorrectLight(car, light);
	actTrafficLightValid(car, light);

	exitIntersectionLightValid(car, light);
}/*}}}*/
