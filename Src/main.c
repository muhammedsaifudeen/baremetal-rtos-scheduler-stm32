#include "main.h"
#include <stdio.h>
#include <stdint.h>


void task1_handler(void);
void task2_handler(void);
void task3_handler(void);
void task4_handler(void);



//Systick  timer Initialization

void init_systick_timer(uint32_t tick_hz);
__attribute__((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack);
__attribute__((naked)) void switch_sp_to_psp(void);
void init_tasks_stack(void);
void enable_processor_faults(void);



uint32_t psp_of_tasks[MAX_TASKS]={T1_STACK_START,T2_STACK_START,T3_STACK_START,T4_STACK_START};

uint32_t task_handlers[MAX_TASKS];

uint8_t current_task   =0; //task1 is running


int main(void)
{

	enable_processor_faults();

	init_scheduler_stack(SCHED_STACK_START);

	task_handlers[0]=(uint32_t)task1_handler;
	task_handlers[1]=(uint32_t)task2_handler;
	task_handlers[2]=(uint32_t)task3_handler;
	task_handlers[3]=(uint32_t)task4_handler;

	init_tasks_stack();

	init_systick_timer(TICK_HZ);

	switch_sp_to_psp();

	task1_handler();

    /* Loop forever */
	for(;;);
}
void task1_handler(void)
{
	while(1){
		printf("Task1 is executing:\n");
	}


}

void task2_handler(void)
{
	while(1){
		printf("Task2 is executing:\n");
	}


}

void task3_handler(void)
{
	while(1){
		printf("Task3 is executing:\n");
	}


}

void task4_handler(void)
{
	while(1){
		printf("Task4 is executing:\n");
	}
}

 void init_systick_timer(uint32_t tick_hz)
{

	uint32_t count_value=SYSTICK_TIM_CLK/TICK_HZ;
	uint32_t *pSCSR =(uint32_t*)0xE000E010; //Systick Control and Status Register
	uint32_t *pSRVR =(uint32_t*)0xE000E014; //Systick Reload value register

	//Clear the offset value of SVR register

	*pSRVR &=(0x00FFFFFFFF);

	//load the values into SVR

	*pSRVR|=count_value;


	//Settings

	*pSCSR|=(1<<1); //Enables Systick Exception request
	*pSCSR|=(1<<2); // Indicates the clock source,Processor Clock
	//Enable systick

	*pSCSR|=(1<<0); //Enables the counter
}
__attribute__((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack)
{
	__asm volatile("MSR MSP,%0": : "r"(sched_top_of_stack) : );
	__asm volatile("BX LR");
}


void init_tasks_stack(void){


	uint32_t *pPSP;
	for (int i = 0; i < MAX_TASKS; i++) {
		pPSP = (uint32_t*)psp_of_tasks[i];

		pPSP--;
		*pPSP = DUMMY_XPSR; // 0x01000000

		pPSP--;
		*pPSP = task_handlers[i]; // PC

		pPSP--;
		*pPSP = 0xFFFFFFFD; // LR (return to Thread mode using PSP)

		for (int j = 0; j < 13; j++) {
			pPSP--;
			*pPSP = 0; // R12, R3, ..., R0
		}

		psp_of_tasks[i] = (uint32_t)pPSP;
	}
}


void enable_processor_faults(void)
{
	uint32_t *pSHCSR =(uint32_t*)0xE000ED24;

	*pSHCSR |= (1<<16);  //memory manage
	*pSHCSR |= (1<<17);  //bus fault
	*pSHCSR |= (1<<18);  //usage fault

}


uint32_t get_psp_value(void)
{


	return psp_of_tasks[current_task];
}

void save_psp_value(uint32_t current_psp_value)
{

	psp_of_tasks[current_task] = current_psp_value;

}

void update_next_task(void)
{
	current_task++;
	current_task %= MAX_TASKS;
}


__attribute__((naked)) void switch_sp_to_psp(void)
{
	//1.Initialize the PSP with Task1 stack address

	//get the value of psp of current task
	__asm volatile("PUSH {LR}"); //preserve LR register, which connects backk to main(
	__asm volatile("BLE get_psp_value");
	__asm volatile("MSR PSP,R0");  //Initialize PSP
	__asm volatile("POP {LR}");  //POPS back LR value


	//2/ change SP to PSP using control register
	__asm volatile("MOV R0,#0X02");
	__asm volatile("MSR CONTROL,R0");
	__asm volatile("BX LR");
}

__attribute__((naked)) void SysTick_Handler(void)
{
/*Save the contect of current task */

	//1. Get current running task's PSP value
	__asm volatile("MRS R0,PSP");

	//2. Using the PSP value store the SF2 (R4 to R11)
	__asm volatile("STMDB R0!,{R4-R11}");

	//3. Save the current value of PSP
	__asm volatile("BL save_psp_value");


	/*Retrieve the context of next task*/

	//1. Decide next task to run

	__asm volatile("BL update_next_task");

	//2. Get its past PSP value
	__asm volatile("BL get_psp_value");

	//3. Using that PSP value retrieve SF2(R4-R11)
	__asm volatile("LDMIA R0!,{R4-R11}");

	//4. Update the PSP and exit
	__asm volatile("MSR PSP,R0");

	__asm volatile("POP {LR}");

	__asm volatile("BX LR");


}

//Implementing fault handlers


void HardFault_Handler(void){

	printf("Exception : Hardfault\n");
	while(1);
}

void MemManage_Handler(void){

	printf("Exception : MemManage\n");
	while(1);
}


void BusFault_Handler(void){

	printf("Exception : Busfault\n");
	while(1);
}

