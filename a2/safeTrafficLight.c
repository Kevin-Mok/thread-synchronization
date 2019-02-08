#include "safeTrafficLight.h"/*{{{*/
/* #include "safeStopSign.h" */
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
	}
}/*}}}*/

void destroySafeTrafficLight(SafeTrafficLight* light) {/*{{{*/
	destroyTrafficLight(&light->base);

	// TODO: Add any logic you need to free data structures
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
	enterLane(car, getLaneLight(car, &light->base));
	/* enterLane(car, getLane(car, &light->base)); */
	printf("Added car %d to LaneQueue %d at pos %d.\n", car->index, lane_index,
			cur_lane_queue->count - 1);
	pthread_mutex_unlock(&light->lane_mutex[lane_index]);
}/*}}}*/

void runTrafficLightCar(Car* car, SafeTrafficLight* light) {/*{{{*/

	// TODO: Add your synchronization logic to this function.
	EntryLane* lane = getLaneLight(car, &light->base);

	/* put in addCarToLaneQueue */
	/* EntryLane* lane = getLaneLight(car, &light->base);
	enterLane(car, lane); */
	addCarToLaneLight(car, light);

	// Enter and act are separate calls because a car turning left can first
	// enter the intersection before it needs to check for oncoming traffic.
	enterTrafficLight(car, &light->base);
	actTrafficLight(car, &light->base, NULL, NULL, NULL);

	exitIntersection(car, lane);

}/*}}}*/
