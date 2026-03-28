/*
 * encoder_control.c
 *
 *  Created on: Dec 17, 2024
 *      Author: Adrian
 */


#include "encoder_control.h"
#include "stm32h7xx_hal.h"
#include "string.h"

#include <math.h>  // bibliotecă pentru a folosi fabs()

void update_encoder(encoder_instance *encoder_value, TIM_HandleTypeDef *htim) {
    static int32_t last_time = 0;
    int32_t current_time = HAL_GetTick();
    float dt = (current_time - last_time) / 1000.0f;

    last_time = current_time;

    int32_t temp_counter = __HAL_TIM_GET_COUNTER(htim);
    static int32_t first_time = 0;

    static float velocity_buffer[5] = {0, 0, 0, 0, 0};
    static uint8_t buffer_index = 0;

    if (!first_time) {
        encoder_value->velocity = 0;
        encoder_value->rpm = 0;
        first_time = 1;
    } else {
        if (temp_counter == encoder_value->last_counter_value) {
            encoder_value->velocity = 0;
        } else if (temp_counter > encoder_value->last_counter_value) {
            if (__HAL_TIM_IS_TIM_COUNTING_DOWN(htim)) {
                encoder_value->velocity = -encoder_value->last_counter_value -
                                         (__HAL_TIM_GET_AUTORELOAD(htim) - temp_counter);
            } else {
                encoder_value->velocity = temp_counter - encoder_value->last_counter_value;
            }
        } else {
            if (__HAL_TIM_IS_TIM_COUNTING_DOWN(htim)) {
                encoder_value->velocity = temp_counter - encoder_value->last_counter_value;
            } else {
                encoder_value->velocity = temp_counter +
                                         (__HAL_TIM_GET_AUTORELOAD(htim) - encoder_value->last_counter_value);
            }
        }
    }

    // Aplici valoarea absolută pentru a te asigura că viteza este întotdeauna pozitivă
    encoder_value->velocity = fabs(encoder_value->velocity);

    velocity_buffer[buffer_index] = encoder_value->velocity;
    buffer_index = (buffer_index + 1) % 5;  // Trecem circular prin buffer

    float sum = 0.0f;
    for (uint8_t i = 0; i < 5; i++) {
        sum += velocity_buffer[i];
    }
    float average_velocity = sum / 5.0f;

    encoder_value->position += average_velocity;
    encoder_value->last_counter_value = temp_counter;

    // Conversie în RPM
    encoder_value->rpm = (average_velocity / 4000.0f) * (60.0f / dt);
}




void reset_encoder(encoder_instance *encoder_value){
	encoder_value -> velocity =0;
	encoder_value ->position = 0;
	encoder_value ->last_counter_value =0;
	encoder_value->rpm = 0;
}
