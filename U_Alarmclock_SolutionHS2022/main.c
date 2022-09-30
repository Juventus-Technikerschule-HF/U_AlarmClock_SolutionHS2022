/*
 * U_Alarmclock_SolutionHS2022.c
 *
 * Created: 27.9.2022 18:15:00
 * Author : Martin Burger
 */ 

//#include <avr/io.h>
#include "avr_compiler.h"
#include <stdio.h>
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

#include "ButtonHandler.h"

extern void vApplicationIdleHook( void );

//function forward declarations
void vTimeTask(void *pvParameters);
void vUITask(void *pvParameters);
void vButtonTask(void *pvParameters);
//EventGroup for ButtonEvents.
EventGroupHandle_t egButtonEvents = NULL;
#define BUTTON1_SHORT	0x01
#define BUTTON1_LONG	0x02
#define BUTTON2_SHORT	0x04
#define BUTTON2_LONG	0x08
#define BUTTON3_SHORT	0x10
#define BUTTON3_LONG	0x20
#define BUTTON4_SHORT	0x40
#define BUTTON4_LONG	0x80
#define BUTTON_ALL		0xFF

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
	xTaskCreate( vUITask, (const char *) "uitask", configMINIMAL_STACK_SIZE, NULL, 1, NULL); //Init UITask. Lowest Priority. Least time critical.
	xTaskCreate( vButtonTask, (const char*) "bttask", configMINIMAL_STACK_SIZE, NULL, 2, NULL); //Init ButtonTask. Medium Priority. Somehow important to time Button debouncing and timing.
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

//Modes for Finite State Machine
#define MODE_IDLE 0
#define MODE_SETTIME 1
#define MODE_SETALARM 2
#define MODE_ALARMALARM 3

void vUITask(void *pvParameters) {
	bool alarmActivated	= false; //alarmActivated state. Can be On or Off / true/false
	char timestring[20]; //Variable for temporary string
	uint8_t mode = MODE_IDLE;
	while(egButtonEvents == NULL) { //Wait for EventGroup to be initialized in other task
		vTaskDelay(10/portTICK_RATE_MS);
	}
	for(;;) {
		switch(mode) {
			case MODE_IDLE: {
				vDisplayClear(); //Clear Display before rewriting it
				vDisplayWriteStringAtPos(0,0,"AlarmClock2022"); //Draw Title
				sprintf(timestring, "%2i:%02i:%02i", hours, minutes, seconds); //Writing Time into one string
				vDisplayWriteStringAtPos(1,0,"Time:       %s", &timestring[0]); //Writing Time string onto Display
				sprintf(timestring, "%2i:%02i:%02i", alarmHours, alarmMinutes, alarmSeconds);	//Writing Alarm Time into one string
				if(alarmActivated == true) { //Writing Alarm Time string onto Display, Write Alarm On/Off depending on alarmActivated state.
					vDisplayWriteStringAtPos(2,0,"Alarm: On   %s", &timestring[0]);
				} else {
					vDisplayWriteStringAtPos(2,0,"Alarm: Off  %s", &timestring[0]);
				}
				vDisplayWriteStringAtPos(3,0,"_ | Al | SetA | SetT"); //Draw Button Info
				
				if(xEventGroupGetBits(egButtonEvents) & BUTTON2_LONG) { //If Button2 is pressed long -> Enable/Disable alarmActivated state
					if(alarmActivated == false) {
						alarmActivated = true;
					} else {
						alarmActivated = false;
					}
				}
				break;
			}
			case MODE_SETTIME:
			{	
				vDisplayClear();
				vDisplayWriteStringAtPos(0,0,"Set Time");
				sprintf(timestring, "%2i:%02i:%02i", hours, minutes, seconds); //Writing Time into one string
				vDisplayWriteStringAtPos(1,0,"Time:       %s", &timestring[0]); //Writing Time string onto Display
				vDisplayWriteStringAtPos(3,0, "Cur |Dec |Inc |Back"); //Draw Button Info
				break;
			}			
			case MODE_SETALARM:
			{
				vDisplayClear();
				vDisplayWriteStringAtPos(0,0,"Set Alarm Time");
				sprintf(timestring, "%2i:%02i:%02i", alarmHours, alarmMinutes, alarmSeconds);	//Writing Alarm Time into one string
				vDisplayWriteStringAtPos(2,0,"Alarm:      %s", &timestring[0]); //Writing Time string onto Display
				vDisplayWriteStringAtPos(3,0, "Cur |Dec |Inc |Back"); //Draw Button Info
				if(xEventGroupGetBits(egButtonEvents) & BUTTON4_LONG) {
					mode = MODE_IDLE;
				}
				break;
			}			
			case MODE_ALARMALARM:
			{
				vDisplayClear();
				vDisplayWriteStringAtPos(0,0,"ALARM! ALARM!");
				vDisplayWriteStringAtPos(3,0, " _ | _ |  _  | Back"); //Draw Button Info
				break;
			}
		}
		vTaskDelay(200/portTICK_RATE_MS);
	}
}
void vButtonTask(void *pvParameters) {
	egButtonEvents = xEventGroupCreate();
	initButtons(); //Initialize Buttons
	for(;;) {
		updateButtons();
		
		if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
			xEventGroupSetBits(egButtonEvents, BUTTON1_SHORT);
		}
		if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
			xEventGroupSetBits(egButtonEvents, BUTTON2_SHORT);
		}
		if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
			xEventGroupSetBits(egButtonEvents, BUTTON3_SHORT);
		}
		if(getButtonPress(BUTTON4) == SHORT_PRESSED) {
			xEventGroupSetBits(egButtonEvents, BUTTON4_SHORT);
		}
		if(getButtonPress(BUTTON1) == LONG_PRESSED) {
			xEventGroupSetBits(egButtonEvents, BUTTON1_LONG);
		}
		if(getButtonPress(BUTTON2) == LONG_PRESSED) {
			xEventGroupSetBits(egButtonEvents, BUTTON2_LONG);
		}
		if(getButtonPress(BUTTON3) == LONG_PRESSED) {
			xEventGroupSetBits(egButtonEvents, BUTTON3_LONG);
		}
		if(getButtonPress(BUTTON4) == LONG_PRESSED) {
			xEventGroupSetBits(egButtonEvents, BUTTON4_LONG);
		}
		vTaskDelay((1000/BUTTON_UPDATE_FREQUENCY_HZ)/portTICK_RATE_MS);
	}
}
