
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
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
#include "keyboard.h"
#include "proto.h"

#define TTY_FIRST    (tty_table)
#define TTY_END        (tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY *p_tty);
PRIVATE void tty_do_read(TTY *p_tty);
PRIVATE void tty_do_write(TTY *p_tty);
PRIVATE void put_key(TTY *p_tty, u32 key);

PUBLIC void disp_info(){
    if (STRATEGY == READER_FIRST) {
        printf("Current Strategy: Reader First\n");
    } else if (STRATEGY == WRITER_FIRST) {
        printf("Current Strategy: Writer First\n");
    } else if (STRATEGY == FAIR) {
        printf("Current Strategy: Fair\n");
    }
    printf("MAX Co-Readers: %d  MAX Co-Writers: %d\n", MAX_READERS, MAX_WRITERS);
    printf("Working slices: %d %d %d %d %d\n", WORKING_SLICES_B, WORKING_SLICES_C, WORKING_SLICES_D, WORKING_SLICES_E,
           WORKING_SLICES_F);
    printf("Relaxing slices: %d %d %d %d %d\n", RELAX_SLICES_B, RELAX_SLICES_C, RELAX_SLICES_D, RELAX_SLICES_E,
           RELAX_SLICES_F);
}

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty() {
	TTY *p_tty;
	
	int start = get_ticks();
    disp_info();
	while (1) {
		// if((get_ticks() - start) >= 20000){
		// 	clean(console_table);
		// 	start = get_ticks();
		// }
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY *p_tty) {
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
	
	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY *p_tty, u32 key) {
	char output[2] = {'\0', '\0'};
	
	if (!(key & FLAG_EXT)) {
		put_key(p_tty, key);
	} else {
		int raw_code = key & MASK_RAW;
		switch (raw_code) {
			case ENTER:
				put_key(p_tty, '\n');
				break;
			case BACKSPACE:
				put_key(p_tty, '\b');
				break;
			case UP:
				if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
					scroll_screen(p_tty->p_console, SCR_DN);
				}
				break;
			case DOWN:
				if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
					scroll_screen(p_tty->p_console, SCR_UP);
				}
				break;
			case F1:
			case F2:
			case F3:
			case F4:
			case F5:
			case F6:
			case F7:
			case F8:
			case F9:
			case F10:
			case F11:
			case F12:
				/* Alt + F1~F12 */
				if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
					select_console(raw_code - F1);
				}
				break;
			default:
				break;
		}
	}
}

/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY *p_tty, u32 key) {
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY *p_tty) {
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY *p_tty) {
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;
		
		out_char(p_tty->p_console, ch);
	}
}

/*======================================================================*
                              tty_write
*======================================================================*/
// PUBLIC void tty_write(TTY* p_tty, char* buf, u8 color)
// {
//         char* p = buf;
//         int i = strlen(buf);

//         while (i) {
//                 out_char(p_tty->p_console, *p++, color);
//                 i--;
//         }
// }

/*======================================================================*
                              sys_write
*======================================================================*/
// PUBLIC int sys_write(char* buf, int color, PROCESS* p_proc)
// {
//         tty_write(&tty_table[p_proc->nr_tty], buf, (u8)color);
//         return 0;
// }

// PUBLIC void out_str(char* buf, int color)
// {
// 	CONSOLE* p_con = console_table;
// 	char* p = buf;
// 	while(*p){
// 		out_char(p_con, *p, color);
// 	}
// }

