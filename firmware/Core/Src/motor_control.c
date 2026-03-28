/*
 * motor_control.c
 *
 *  Created on: Nov 11, 2024
 *      Author: Adrian
 */

#include "motor_control.h"
#include "stm32h7xx_hal.h"
#include "string.h"

extern TIM_HandleTypeDef htim3;

void accelerate_to_RPM(int target_speed_RPM) {

    static float u_buffer[BUFFER_SIZE] = {0}; // pentru u[k]
    static float e_buffer[BUFFER_SIZE] = {0}; // pentru e[k]
    int u_index = 0;
    int e_index = 0;


    int target_frequency = (target_speed_RPM / 60.0) * 3200;
    int current_frequency = 3000000 / (__HAL_TIM_GET_AUTORELOAD(&htim3)) + 1;

    if (target_frequency > MAX_FREQUENCY) {
        target_frequency = MAX_FREQUENCY;
    } else if (target_frequency < MIN_FREQUENCY) {
        target_frequency = MIN_FREQUENCY;
    }

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    while (current_frequency < (target_frequency - 75)) {
        // Calculul erorii
        float error = (float)(target_frequency - current_frequency);

        // Limitarea erorii
        if (error > MAX_E) error = MAX_E;
        if (error < MIN_E) error = MIN_E;

        // Calculul lui u[k]
        float u_k =
        	-u_buffer[(u_index - 4 + BUFFER_SIZE) % BUFFER_SIZE]
            + 0.3763 * u_buffer[(u_index - 3 + BUFFER_SIZE) % BUFFER_SIZE]
            + 0.8506 * u_buffer[(u_index - 2 + BUFFER_SIZE) % BUFFER_SIZE]
            + 0.0103 * u_buffer[(u_index - 1 + BUFFER_SIZE) % BUFFER_SIZE]
            + 2.2137 * e_buffer[(e_index - 4 + BUFFER_SIZE) % BUFFER_SIZE]
            - 2.7034 * e_buffer[(e_index - 3 + BUFFER_SIZE) % BUFFER_SIZE]
            - 1.4698 * e_buffer[(e_index - 2 + BUFFER_SIZE) % BUFFER_SIZE]
            + 2.7063 * e_buffer[(e_index - 1 + BUFFER_SIZE) % BUFFER_SIZE]
            -0.6650 * error;


        u_k *= 0.2372;

        // Actualizare buffer și indice
        u_buffer[u_index] = u_k;
        e_buffer[e_index] = error;
        u_index = (u_index + 1) % BUFFER_SIZE;
        e_index = (e_index + 1) % BUFFER_SIZE;


        current_frequency += (int)(u_k);

        // Limitarea frecvenței
        if (current_frequency < MIN_FREQUENCY) {
            current_frequency = MIN_FREQUENCY;
        }

        if (current_frequency > target_frequency) {
            current_frequency = target_frequency;
        }

        // Actualizează ARR cu noua valoare
        __HAL_TIM_SET_AUTORELOAD(&htim3, (3000000 / current_frequency) - 1);
        HAL_Delay(5);
    }
}

void decelerate_to_RPM(int target_speed_RPM) {
	int target_frequency = 0;
	int current_frequency  = 3000000 / (__HAL_TIM_GET_AUTORELOAD(&htim3)) +1;

	if (target_speed_RPM == 0) {
		target_frequency = MIN_FREQUENCY;
	} else {
		target_frequency = (target_speed_RPM / 60.0) * 3200;
	}

	while (current_frequency > target_frequency) {

		current_frequency -= DECELERATION_STEP;

		if (current_frequency < target_frequency) {
			current_frequency = target_frequency;
		}

		__HAL_TIM_SET_AUTORELOAD(&htim3, (3000000 / current_frequency) - 1);
		HAL_Delay(10);
	}

	if (target_speed_RPM == 0 && target_frequency == MIN_FREQUENCY) {
		HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
	}
}

void set_direction(Direction direction) {
    if (direction == DIRECTION_LEFT) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    } else if (direction == DIRECTION_RIGHT) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    }
}


