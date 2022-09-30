/*
 * U_Alarmclock_SolutionHS2022.c
 *
 * Created: 27.9.2022 18:15:00
 * Author : Martin Burger
 */ 

//#include <avr/io.h>
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

extern void vApplicationIdleHook( void );

//function forward declarations
void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    vInitClock();
	vInitDisplay();
	vTaskStartScheduler();
	return 0;
}
