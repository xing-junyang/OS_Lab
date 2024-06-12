
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule() {
	PROCESS *p;
	int greatest_ticks = 0;
	
	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++) {
			if (p->sleeping > 0 || p->blocked == TRUE){
                continue;
            }
			if (p->ticks > greatest_ticks) {
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}
		if (!greatest_ticks) {
			for (p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++) {
				if (p->ticks > 0){
                    continue;
                }
				p->ticks = p->priority;
			}
		}
	}
}

//TODO: 增加的系统调用
PUBLIC void p_process(SEMAPHORE *s) {
	disable_int();
	s->value--;
	if (s->value < 0) {
		p_proc_ready->blocked = TRUE;
		p_proc_ready->status = WAITING;
		s->p_list[s->tail] = p_proc_ready;
		s->tail = (s->tail + 1) % NR_PROCS;
		schedule();
	}
	enable_int();
}

PUBLIC void v_process(SEMAPHORE *s) {
	disable_int();
	s->value++;
	if (s->value <= 0) {
		s->p_list[s->head]->blocked = FALSE;
		s->head = (s->head + 1) % NR_PROCS;
	}
	enable_int();
}

PUBLIC int sys_get_ticks() {
	return ticks;
}

PUBLIC void sys_sleep(int milli_sec) {
	int ticks = milli_sec / 1000 * HZ * 10;
	p_proc_ready->sleeping = ticks;
	schedule();
}

PUBLIC void sys_write_str(char *buf, int len) {
	CONSOLE *p_con = console_table;
	for (int i = 0; i < len; i++) {
		out_char(p_con, buf[i]);
	}
}

