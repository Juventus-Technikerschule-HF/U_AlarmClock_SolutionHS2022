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
//Time in hours, minutes and seconds. Each as uint8_t to prevent corrupt data
uint8_t hours = 18;
uint8_t minutes = 15;
uint8_t seconds = 00;

uint8_t alarmHours = 21;
uint8_t alarmMinutes = 35;
uint8_t alarmSeconds = 00;

void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    vInitClock();
	vInitDisplay();
	xTaskCreate( vTimeTask, (const char *) "timetask", configMINIMAL_STACK_SIZE, NULL, 3, NULL); //Init TimeTask. Highest Priority to maximize Time accuracy
	vTaskStartScheduler();
	return 0;
}

void vTimeTask(void *pvParameters) {
	for(;;) {
		seconds++; //Increment Seconds
		if(seconds >= 60) { //When seconds overflow, set to 0 and increment minutes
			seconds = 0;
			minutes++;
		}
		if(minutes >= 60) { //When minutes overflow, set to 0 and increment hours
			minutes = 0;
			hours++;
		}
		if(hours >= 24) { //When hours overflow, set to 0
			hours = 0;
		}
		vTaskDelay(1000/portTICK_RATE_MS);
	}
}
