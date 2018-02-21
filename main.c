/*
 * Copyright (c) 2017, NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    Tarea 6.c
 * @brief   Application entry point.
 */
#include <freertos/source/tasks.c>
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include "rtos_config.h"
#include "rtos.h"
#include "task.h"
/* TODO: insert other include files here. */

void dummy_task1 (void)
{
    static uint8_t counter;
    static rtos_tick_t retardo;
    for (;;)
    {
        retardo = rtos_get_clock ();
        PRINTF ("IN TASK 1: %i +++++++++++++++\r\n", counter);
        counter++;
        retardo -= rtos_get_clock ();
        vTaskDelay (2000 - retardo);
    }
}
void dummy_task2 (void)
{
    static uint8_t counter;
    static rtos_tick_t retardo;
    for (;;)
    {
        retardo = rtos_get_clock ();
        PRINTF ("IN TASK 2: %i +++++++++++++++\r\n", counter);
        counter++;
        retardo -= rtos_get_clock ();
        vTaskDelay (1000 - retardo);
    }
}
void dummy_task3 (void)
{
    static uint8_t counter;
    static rtos_tick_t retardo;
    for (;;)
    {
        retardo = rtos_get_clock ();
        PRINTF ("IN TASK 3: %i +++++++++++++++\r\n", counter);
        counter++;
        retardo -= rtos_get_clock ();
        vTaskDelay (4000 - retardo);
    }
}

/* TODO: insert other definitions and declarations here. */

/*
 * @brief   Application entry point.
 */
int main(void) {

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();


    TaskFunction_t pxTaskCode = (void *) &dummy_task1;//function direction
    const char * const pcName[20];//identifier name
    const uint16_t usStackDepth = 100*4;//stack size
    void * pvParameters = 0;//values to the task
    UBaseType_t uxPriority;//task priority
    TaskHandle_t * const pxCreatedTask = NULL;//handle task, not used

    TaskFunction_t pxTaskCode2 = (void *) &dummy_task2;//function direction
    TaskFunction_t pxTaskCode3 = (void *) &dummy_task3;//function direction

    xTaskCreate(pxTaskCode, "Dummy One", usStackDepth, &pvParameters, 1, pxCreatedTask);
    xTaskCreate(pxTaskCode2, "Dummy Two", usStackDepth, &pvParameters, 1, pxCreatedTask);
    xTaskCreate(pxTaskCode3, "Dummy Three", usStackDepth, &pvParameters, 1, pxCreatedTask);
    vTaskStartScheduler();

//    rtos_create_task (dummy_task1, 1, kAutoStart);
//    rtos_create_task (dummy_task2, 2, kAutoStart);
//    rtos_create_task (dummy_task3, 1, kAutoStart);
//    rtos_start_scheduler ();

    /* Force the counter to be placed into memory. */
    volatile static int i = 0 ;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
        i++ ;
    }
    return 0 ;
}
