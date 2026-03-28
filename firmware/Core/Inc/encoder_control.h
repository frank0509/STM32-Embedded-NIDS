/*
 * encoder_control.h
 *
 *  Created on: Dec 17, 2024
 *      Author: Adrian
 */

#ifndef INC_ENCODER_CONTROL_H_
#define INC_ENCODER_CONTROL_H_

#include "main.h"

typedef struct{
	int32_t velocity;
	int32_t position;
	int32_t last_counter_value;
	float rpm;
} encoder_instance;

void update_encoder(encoder_instance *encoder_value, TIM_HandleTypeDef *htim);
void reset_encoder(encoder_instance *encoder_value);

#endif /* INC_ENCODER_CONTROL_H_ */
