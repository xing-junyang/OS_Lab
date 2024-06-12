
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

/*======================================================================*
                            init_tasks
 *======================================================================*/

PRIVATE void init_tasks() {
    init_screen(tty_table);
    clean(console_table);

    int prior[7] = {1, 1, 1, 1, 1, 1, 1};
    for (int i = 0; i < 7; ++i) {
        proc_table[i].ticks = prior[i];
        proc_table[i].priority = prior[i];
    }

    // init
    k_reenter = 0;
    ticks = 0;
    readers = 0;
    p_proc_ready = proc_table;
}

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main() {
    disp_str("-----\"kernel_main\" begins-----\n");

    TASK *p_task = task_table;
    PROCESS *p_proc = proc_table;
    char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
    u16 selector_ldt = SELECTOR_LDT_FIRST;
    u8 privilege;
    u8 rpl;
    int eflags;
    for (int i = 0; i < NR_TASKS + NR_PROCS; i++) {
        if (i < NR_TASKS) {     /* Task */
            p_task = task_table + i;
            privilege = PRIVILEGE_TASK;
            rpl = RPL_TASK;
            eflags = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
        } else {                  /* Usr */
            p_task = user_proc_table + (i - NR_TASKS);
            privilege = PRIVILEGE_USER;
            rpl = RPL_USER;
            eflags = 0x202; /* IF=1, bit 2 is always 1 */
        }
        strcpy(p_proc->p_name, p_task->name);   // name of the process
        p_proc->pid = i;                                    // pid
        p_proc->sleeping = 0;
        p_proc->blocked = FALSE;
        p_proc->status = RELAXING;
        p_proc->ldt_sel = selector_ldt;
        memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
        p_proc->ldts[0].attr1 = DA_C | privilege << 5;
        memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
        p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
        p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.ds = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.es = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.fs = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.ss = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
        p_proc->regs.eip = (u32) p_task->initial_eip;
        p_proc->regs.esp = (u32) p_task_stack;
        p_proc->regs.eflags = eflags;
        p_proc->nr_tty = 0;
        p_task_stack -= p_task->stacksize;
        p_proc++;
        p_task++;
        selector_ldt += 1 << 3;
    }

    init_tasks();
    init_clock();
    init_keyboard();
    restart();

    while (1) {}
}

int stat[5];

void work(int i){
    stat[i]++;
    sleep_ms(TIME_SLICE);
}

void Producer1() {
    while (1) {
        P(&buffer);
        P(&mutex);
        work(0);
        V(&mutex);
        V(&productA);
    }
}

void Producer2() {
    while (1) {
        P(&buffer);
        P(&mutex);
        work(1);
        V(&mutex);
        V(&productB);
    }
}

void Consumer1() {
    while (1) {
        P(&productA);
        P(&mutex);
        work(2);
        V(&mutex);
        V(&buffer);
    }
}

void Consumer2() {
    while (1) {
        P(&productB);
        P(&mutex);
        work(3);
        V(&mutex);
        V(&buffer);
    }
}

void Consumer3() {
    while (1) {
        P(&productB);
        P(&mutex);
        work(4);
        V(&mutex);
        V(&buffer);
    }
}

/* print a 2-digit number */
void printNumber(int number) {
    char temp[2] = {number / 10 + '0', number % 10 + '0'};
    if (temp[0] != '0') {
        printf("\06%c", temp[0]);
    }
    printf("\06%c ", temp[1]);
}

void ReporterA() {
    sleep_ms(TIME_SLICE);
    for (int time_stamp = 1; time_stamp <= 20; time_stamp++) {
        printNumber(time_stamp);
        for (int i = 0; i < 5; i++) {
            printNumber(stat[i]);
        }
        printf("\n");
        sleep_ms(TIME_SLICE);
    }
    while (1) {}
}