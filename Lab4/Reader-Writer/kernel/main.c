
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

// Consume Slices
PRIVATE void read_proc(int slices) {
    p_proc_ready->status = WORKING;
    sleep_ms(slices * TIME_SLICE);
}

PRIVATE void write_proc(int slices) {
    p_proc_ready->status = WORKING;
    sleep_ms(slices * TIME_SLICE);
}

// Fair
void read_fair(int slices) {
    P(&S);
    P(&reader_count_mutex);
    P(&reader_mutex);
    if (++readers == 1) {
        P(&rw_mutex);
    }
    V(&reader_mutex);
    V(&S);
    read_proc(slices);
    P(&reader_mutex);
    if (--readers == 0) {
        V(&rw_mutex);
    }
    V(&reader_mutex);
    V(&reader_count_mutex);
}

void write_fair(int slices) {
    P(&S);
    P(&rw_mutex);
    write_proc(slices);
    V(&rw_mutex);
    V(&S);
}

// Reader First
void read_rf(int slices) {
    P(&reader_mutex);
    if (++readers == 1) {
        P(&writer_mutex);
    }
    V(&reader_mutex);
    P(&reader_count_mutex);
    read_proc(slices);
    V(&reader_count_mutex);
    P(&reader_mutex);
    if (--readers == 0) {
        V(&writer_mutex);
    }
    V(&reader_mutex);
}
void write_rf(int slices) {
    P(&rw_mutex);
    P(&writer_mutex);
    write_proc(slices);
    V(&writer_mutex);
    V(&rw_mutex);
}

// Writer First
void read_wf(int slices) {
    P(&reader_count_mutex);
    P(&S);
    P(&reader_mutex);
    if (++readers == 1) {
        P(&rw_mutex);
    }
    V(&reader_mutex);
    V(&S);
    read_proc(slices);
    P(&reader_mutex);
    if (--readers == 0) {
        V(&rw_mutex);
    }
    V(&reader_mutex);
    V(&reader_count_mutex);
}
void write_wf(int slices) {
    P(&writer_mutex);
    if (++writers == 1) {
        P(&S);
    }
    V(&writer_mutex);
    P(&rw_mutex);
    write_proc(slices);
    V(&rw_mutex);
    P(&writer_mutex);
    if (--writers == 0) {
        V(&S);
    }
    V(&writer_mutex);
}

typedef void    (*read_f)(int);
typedef void    (*write_f)(int);
read_f read_funcs[3] = {read_rf, read_wf, read_fair};
write_f write_funcs[3] = {write_rf, write_wf, write_fair};

void ReaderB() {
    while (1) {
        read_funcs[STRATEGY](WORKING_SLICES_B);
        p_proc_ready->status = RELAXING;
        sleep_ms(RELAX_SLICES_B * TIME_SLICE);
        p_proc_ready->status = WAITING;
    }
}

void ReaderC() {
    while (1) {
        read_funcs[STRATEGY](WORKING_SLICES_C);
        p_proc_ready->status = RELAXING;
        sleep_ms(RELAX_SLICES_C * TIME_SLICE);
        p_proc_ready->status = WAITING;
    }
}

void ReaderD() {
    while (1) {
        read_funcs[STRATEGY](WORKING_SLICES_D);
        p_proc_ready->status = RELAXING;
        sleep_ms(RELAX_SLICES_D * TIME_SLICE);
        p_proc_ready->status = WAITING;
    }
}

void WriterE() {
    while (1) {
        write_funcs[STRATEGY](WORKING_SLICES_E);
        p_proc_ready->status = RELAXING;
        sleep_ms(RELAX_SLICES_E * TIME_SLICE);
        p_proc_ready->status = WAITING;
    }
}

void WriterF() {
    while (1) {
        write_funcs[STRATEGY](WORKING_SLICES_F);
        p_proc_ready->status = RELAXING;
        sleep_ms(RELAX_SLICES_F * TIME_SLICE);
        p_proc_ready->status = WAITING;
    }
}

char colors[3] = {'\01', '\03', '\02'};
char signs[3] = {'X', 'Z', 'O'};

void ReporterA() {
    sleep_ms(TIME_SLICE);
    for (int time_stamp = 1; time_stamp <= 20; time_stamp++) {
        printf("%c%d%d ", '\06', (time_stamp) / 10, (time_stamp) % 10);
        for (int i = NR_TASKS + 1; i < NR_PROCS + NR_TASKS; i++) {
            int proc_status = (proc_table + i)->status;
            printf("%c%c ", colors[proc_status], signs[proc_status]);
        }
        printf("\n");
        sleep_ms(TIME_SLICE);
    }
    while (1) {}
}