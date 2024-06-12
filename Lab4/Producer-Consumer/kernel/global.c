
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "proc.h"
#include "global.h"
#include "proto.h"


PUBLIC    PROCESS proc_table[NR_TASKS + NR_PROCS];

PUBLIC    TASK task_table[NR_TASKS] = {
		{task_tty, STACK_SIZE_TTY, "tty"}};

PUBLIC  TASK user_proc_table[NR_PROCS] = {
		{ReporterA, STACK_SIZE_TESTA, "ReporterA"},
		{Producer1,   STACK_SIZE_TESTB, "Producer1"},
		{Producer2,   STACK_SIZE_TESTC, "Producer2"},
		{Consumer1,   STACK_SIZE_TESTD, "Consumer1"},
		{Consumer2,   STACK_SIZE_TESTE, "Consumer2"},
		{Consumer3,   STACK_SIZE_TESTF, "Consumer3"}
};

PUBLIC    char task_stack[STACK_SIZE_TOTAL];

PUBLIC    TTY tty_table[NR_CONSOLES];
PUBLIC    CONSOLE console_table[NR_CONSOLES];

PUBLIC    irq_handler irq_table[NR_IRQ];

PUBLIC    system_call sys_call_table[NR_SYS_CALL] = {
		sys_get_ticks,
		sys_write_str,
		sys_sleep,
		p_process,
		v_process
};

PUBLIC  SEMAPHORE buffer = {MAX_PRODUCTS, 0, 0};
PUBLIC  SEMAPHORE productA = {0, 0, 0};
PUBLIC  SEMAPHORE productB = {0, 0, 0};
PUBLIC  SEMAPHORE mutex = {1, 0, 0};
