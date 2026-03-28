/*
 * motor_control.h
 *
 *  Created on: Nov 11, 2024
 *      Author: Adrian
 */

#ifndef INC_MOTOR_CONTROL_H_
#define INC_MOTOR_CONTROL_H_

#define MIN_FREQUENCY 2300
#define MAX_FREQUENCY 3000000
#define ACCELERATION_STEP 100
#define DECELERATION_STEP 100
#define MAX_STEP 100

#define BUFFER_SIZE 5
#define MAX_E 4500
#define MIN_E -4500

#include "main.h"

typedef enum {
    DIRECTION_LEFT,
    DIRECTION_RIGHT
} Direction;


void accelerate_to_RPM(int target_speed_RPM);
void decelerate_to_RPM(int target_speed_RPM);
void set_direction(Direction direction);

#endif /* INC_MOTOR_CONTROL_H_ */
