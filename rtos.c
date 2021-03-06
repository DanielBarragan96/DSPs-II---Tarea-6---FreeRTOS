/**
 * @file rtos.c
 * @author ITESO
 * @date Feb 2018
 * @brief Implementation of rtos API
 *
 * This is the implementation of the rtos module for the
 * embedded systems II course at ITESO
 */

#include "rtos.h"
#include "rtos_config.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

#ifdef RTOS_ENABLE_IS_ALIVE
#include "fsl_gpio.h"
#include "fsl_port.h"
#endif
/**********************************************************************************/
// Module defines
/**********************************************************************************/

#define FORCE_INLINE    __attribute__((always_inline)) inline

#define STACK_FRAME_SIZE            8
#define STACK_PC_OFFSET             2
#define STACK_PSR_OFFSET            1
#define STACK_PSR_DEFAULT           0x01000000

#define RTOS_INVALID_TASK           -1

/**********************************************************************************/
// IS ALIVE definitions
/**********************************************************************************/

#ifdef RTOS_ENABLE_IS_ALIVE
#define CAT_STRING(x,y)         x##y
#define alive_GPIO(x)           CAT_STRING(GPIO,x)
#define alive_PORT(x)           CAT_STRING(PORT,x)
#define alive_CLOCK(x)          CAT_STRING(kCLOCK_Port,x)
static void init_is_alive (void);
static void refresh_is_alive (void);
#endif

/**********************************************************************************/
// Type definitions
/**********************************************************************************/

typedef enum
{
    S_READY = 0, S_RUNNING, S_WAITING, S_SUSPENDED
} task_state_e;
typedef enum
{
    kFromISR = 0, kFromNormalExec
} task_switch_type_e;

typedef struct
{
    uint8_t priority;
    task_state_e state;
    uint32_t *sp;
    void (*task_body) ();
    rtos_tick_t local_tick;
    uint32_t stack[RTOS_STACK_SIZE + 10];
    //para evitar que el printf cambie los otros aspectos de la estructura
} rtos_tcb_t;

/**********************************************************************************/
// Global (static) task list
/**********************************************************************************/

struct
{
    uint8_t nTasks;
    rtos_task_handle_t current_task;
    rtos_task_handle_t next_task;
    rtos_tcb_t tasks[RTOS_MAX_NUMBER_OF_TASKS + 1];
    rtos_tick_t global_tick;
} task_list =
{ 0 };

/**********************************************************************************/
// Local methods prototypes
/**********************************************************************************/

static void reload_systick (void);
static void dispatcher (task_switch_type_e type);
static void activate_waiting_tasks ();
FORCE_INLINE static void context_switch (task_switch_type_e type);
static void idle_task (void);

/**********************************************************************************/
// API implementation
/**********************************************************************************/

void rtos_start_scheduler (void)
{
#ifdef RTOS_ENABLE_IS_ALIVE
    init_is_alive ();
    task_list.global_tick = 0;
    rtos_create_task (idle_task, 0, kAutoStart);
    task_list.current_task = RTOS_INVALID_TASK;
#endif
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk
            | SysTick_CTRL_ENABLE_Msk;
    reload_systick ();
    for (;;)
        ;
}

rtos_task_handle_t rtos_create_task (void (*task_body) (), uint8_t priority,
        rtos_autostart_e autostart)
{
    rtos_task_handle_t retval = -1;
    if (RTOS_MAX_NUMBER_OF_TASKS > task_list.nTasks)
    {
        task_list.tasks[task_list.nTasks].priority = priority;
        task_list.tasks[task_list.nTasks].local_tick = 0;
        task_list.tasks[task_list.nTasks].task_body = task_body; //apuntador a la tarea. cuando se asigna, tiene la direccion de la tarea
        task_list.tasks[task_list.nTasks].sp =
                &(task_list.tasks[task_list.nTasks].stack[RTOS_STACK_SIZE - 1
                        - STACK_FRAME_SIZE]); //asigna los valores para el PSR
        task_list.tasks[task_list.nTasks].state =
                kStartSuspended == autostart ? S_SUSPENDED : S_READY;
        task_list.tasks[task_list.nTasks].stack[RTOS_STACK_SIZE
                - STACK_PC_OFFSET] = (uint32_t) task_body;
        task_list.tasks[task_list.nTasks].stack[RTOS_STACK_SIZE
                - STACK_PSR_OFFSET] = STACK_PSR_DEFAULT;
        retval = task_list.nTasks;
        task_list.nTasks++;
    }
    return retval;
}

rtos_tick_t rtos_get_clock (void)
{
    return task_list.global_tick;
}

void rtos_delay (rtos_tick_t ticks)
{
    task_list.tasks[task_list.current_task].state = S_WAITING;
    task_list.tasks[task_list.current_task].local_tick = ticks;
    dispatcher (kFromNormalExec);
}

void rtos_suspend_task (void)
{
    task_list.tasks[task_list.current_task].state = S_SUSPENDED;
    dispatcher (kFromNormalExec);
}

void rtos_activate_task (rtos_task_handle_t task)
{
    task_list.tasks[task].state = S_READY;
    dispatcher (task);
}

/**********************************************************************************/
// Local methods implementation
/**********************************************************************************/

static void reload_systick (void)
{
    SysTick->LOAD = USEC_TO_COUNT(RTOS_TIC_PERIOD_IN_US,
            CLOCK_GetCoreSysClkFreq ());
    SysTick->VAL = 0;
}

static void dispatcher (task_switch_type_e type)
{ //busca cual es la de mas alta prioridad
    rtos_task_handle_t next_task = RTOS_INVALID_TASK;
    uint8_t index;
    int8_t highest = -1;
    for (index = 0; index < task_list.nTasks; index++)
    {
        if (highest < task_list.tasks[index].priority
                && (S_READY == task_list.tasks[index].state
                        || S_RUNNING == task_list.tasks[index].state))
        {
            next_task = index;
            highest = task_list.tasks[index].priority;
        }
    }

    if (task_list.current_task != next_task)
    {
        task_list.next_task = next_task;
        context_switch (type);
    }
}

FORCE_INLINE static void context_switch (task_switch_type_e type)
{
    register uint32_t *sp asm("sp");
    volatile static int first = 1;
    if (!first)
    {
        if (kFromISR == type)
        {
            task_list.tasks[task_list.current_task].sp = sp + 9;
        }
        else
        {
            task_list.tasks[task_list.current_task].sp = sp - 9;
        }
    }
    else
    {
        first = 0;
    }
    task_list.current_task = task_list.next_task;
    task_list.tasks[task_list.current_task].state = S_RUNNING;
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

static void activate_waiting_tasks ()
{ //aqui decrementamos los del numero de ticks y cuando terminamos se activan las tareas lo del -9 que había puesto en iddle task (activate task)
    uint8_t index;
    for (index = 0; index < task_list.nTasks; index++)
    {
        if (S_WAITING == task_list.tasks[index].state)
        {
            task_list.tasks[index].local_tick--;
            if (0 == task_list.tasks[index].local_tick)
            {
                task_list.tasks[index].state = S_READY;
            }
        }
    }
}

/**********************************************************************************/
// IDLE TASK
/**********************************************************************************/

static void idle_task (void)
{
    for (;;)
    {

    }
}

/**********************************************************************************/
// ISR implementation
/**********************************************************************************/

void xPortSysTickHandler (void)
{
#ifdef RTOS_ENABLE_IS_ALIVE
    refresh_is_alive ();
#endif
    task_list.global_tick++;
    activate_waiting_tasks ();
    dispatcher (kFromISR);
    reload_systick ();
}

void xPortPendSVHandler (void)
{
    register uint32_t *r0 asm("r0"); //r7 para engañar al compilador
    SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
    r0 = task_list.tasks[task_list.current_task].sp;
    asm("mov r7, r0");
}

/**********************************************************************************/
// IS ALIVE SIGNAL IMPLEMENTATION
/**********************************************************************************/

#ifdef RTOS_ENABLE_IS_ALIVE
static void init_is_alive (void)
{
    gpio_pin_config_t gpio_config =
    { kGPIO_DigitalOutput, 1, };

    port_pin_config_t port_config =
    { kPORT_PullDisable, kPORT_FastSlewRate, kPORT_PassiveFilterDisable,
            kPORT_OpenDrainDisable, kPORT_LowDriveStrength, kPORT_MuxAsGpio,
            kPORT_UnlockRegister, };
    CLOCK_EnableClock (alive_CLOCK(RTOS_IS_ALIVE_PORT));
    PORT_SetPinConfig (alive_PORT(RTOS_IS_ALIVE_PORT), RTOS_IS_ALIVE_PIN,
            &port_config);
    GPIO_PinInit (alive_GPIO(RTOS_IS_ALIVE_PORT), RTOS_IS_ALIVE_PIN,
            &gpio_config);
}

static void refresh_is_alive (void)
{
    static uint8_t state = 0;
    static uint32_t count = 0;
    SysTick->LOAD = USEC_TO_COUNT(RTOS_TIC_PERIOD_IN_US,
            CLOCK_GetCoreSysClkFreq ());
    SysTick->VAL = 0;
    if (RTOS_IS_ALIVE_PERIOD_IN_US / RTOS_TIC_PERIOD_IN_US - 1 == count)
    {
        GPIO_WritePinOutput (alive_GPIO(RTOS_IS_ALIVE_PORT), RTOS_IS_ALIVE_PIN,
                state);
        state = state == 0 ? 1 : 0;
        count = 0;
    }
    else
    {
        count++;
    }
}
#endif
