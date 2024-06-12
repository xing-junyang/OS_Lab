# OS-Lab 4

## 实验任务1 添加系统调用

- 编写系统调用代码

```c
//proc.c
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
```

- 加入系统调用表

```c
//global.c
PUBLIC    system_call sys_call_table[NR_SYS_CALL] = {
       sys_get_ticks,
       sys_write_str,
       sys_sleep,
       p_process,
       v_process
};
```

- 利用汇编处理用户调用系统调用

```assembly
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_write_str      equ 1
_NR_sleep_ms       equ    2
_NR_P            equ    3
_NR_V            equ    4
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global  get_ticks
global  write_str
global  sleep_ms
global  P
global  V

bits 32
[section .text]

write_str:
    mov    eax, _NR_write_str
    push ebx
    push ecx
    mov    ebx, [esp+12]  ;Args 1
    mov    ecx, [esp+16]  ;Args 2
    int     INT_VECTOR_SYS_CALL
    pop ecx
    pop ebx
    ret

sleep_ms:
    mov    eax, _NR_sleep_ms
    push ebx
    mov    ebx, [esp+8]
    int     INT_VECTOR_SYS_CALL
    pop ebx
    ret

P:
    push ebx
    mov    eax, _NR_P
    mov    ebx, [esp+8]
    int    INT_VECTOR_SYS_CALL
    pop ebx
    ret

V:
    mov    eax, _NR_V
    mov    ebx, [esp+4]
    int    INT_VECTOR_SYS_CALL
    ret
```

## 实验任务2 读者写者问题

- 读者优先

```c
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
```

- 写者优先

```c
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
```

- 读写平衡

```c
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
```

- 输出（使用一个观察者进程）

```c
for (int time_stamp = 1; time_stamp <= 20; time_stamp++) {
    printf("%c%d%d ", '\06', (time_stamp) / 10, (time_stamp) % 10);
    for (int i = NR_TASKS + 1; i < NR_PROCS + NR_TASKS; i++) {
        int proc_status = (proc_table + i)->status;
        printf("%c%c ", colors[proc_status], signs[proc_status]);
    }
    printf("\n");
    sleep_ms(TIME_SLICE);
}
```

### 实验任务3 生产者消费者问题

- 定义信号量

```c
EXTERN  SEMAPHORE buffer;
EXTERN  SEMAPHORE productA;
EXTERN  SEMAPHORE productB;
EXTERN  SEMAPHORE mutex;
```

- 生产者与消费者互斥

```c
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
```

- 输出（使用一个观察者进程）

```c
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
```