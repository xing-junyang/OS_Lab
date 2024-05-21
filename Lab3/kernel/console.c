
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


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

PRIVATE void set_cursor(unsigned int position);

PRIVATE void set_video_start_addr(u32 addr);

PRIVATE void flush(CONSOLE *p_con);

PUBLIC int backspace(CONSOLE *p_con) {
    u8 *p_vmem = (u8 *) (V_MEM_BASE + p_con->cursor * 2);
    int diff = 0;
    if(write_mode==1&&p_con->cursorLineStack[p_con->cursorLineStackTop-1]<p_con->cursorPos){
        return p_con->cursor;
    }
    if (p_con->cursorLineStackTop) {
        diff = p_con->cursor - p_con->cursorLineStack[--p_con->cursorLineStackTop];
    } else {
        return -1;
    }
    for (int i = 0; i < diff; i++) {
        if (p_con->cursor > p_con->original_addr) {
            p_con->cursor--;
            *(p_vmem - 2) = ' ';
            *(p_vmem - 1) = DEFAULT_CHAR_COLOR;
        }
    }
    return p_con->cursor;
}

PUBLIC void reset(CONSOLE *p_con) {
    for(int i = p_con->original_addr; i < p_con->cursorPos; i++){
        if(*(u8 *) (V_MEM_BASE + 2 * i)!=' '){
            *(u8 *) (V_MEM_BASE + 2 * i + 1) = DEFAULT_CHAR_COLOR;
        }
    }
    set_cursor(p_con->cursorPos);
}

PUBLIC int cmp(u8 *a, u8 *b, int len) {
    for (int i = 0; i < len*2; i += 2) {
        if (a[i] != b[i]) {
            return 0;
        }
        if (a[i] == ' '&&a[i + 1] != b[i + 1]) {
            return 0;
        }
        if (a[i] == '\n'||b[i] == '\n'){
            return 0;
        }
    }
    return 1;
}

int min(int a,int b){return a>b?b:a;}

PUBLIC void search(CONSOLE *p_con) {
    int len = p_con->cursor - p_con->cursorPos;
    if (!len) {
        return;
    }
    for (int i = p_con->original_addr; i <= p_con->cursorPos - len; i++) {
        if (cmp((u8 *) (V_MEM_BASE + 2 * i), (u8 *) (V_MEM_BASE + p_con->cursorPos* 2), len)) {
            for (int j = 0; j < len; j++) {
                if (*(u8 *) (V_MEM_BASE + 2 * i + 2 * j) != ' ') {
                    *(u8 *) (V_MEM_BASE + 2 * i + 2 * j + 1) = RED;
                }
            }
        }
    }
}

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty) {
    int nr_tty = p_tty - tty_table;
    p_tty->p_console = console_table + nr_tty;

    int v_mem_size = V_MEM_SIZE >> 1;    /* 显存总大小 (in WORD) */

    int con_v_mem_size = v_mem_size / NR_CONSOLES;
    p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
    p_tty->p_console->v_mem_limit = con_v_mem_size;
    p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

    /* 默认光标位置在最开始处 */
    p_tty->p_console->cursor = p_tty->p_console->original_addr;
    p_tty->p_console->cursorPos = p_tty->p_console->cursor;
    p_tty->p_console->cursorLineStackTop = 0;
    p_tty->p_console->cursorLineStack[p_tty->p_console->cursorLineStackTop] = 0;
    p_tty->p_console->character.pos = 0;
    for (int i = 0; i < SCREEN_SIZE; i++) {
        p_tty->p_console->character.content[i] = ' ';
    }
    set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con) {
    return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch) {
    u8 *p_vmem = (u8 *) (V_MEM_BASE + p_con->cursor * 2);

    if (write_mode == 2) {
        if (ch == '\r') {
            p_con->character.pos = p_con->character.escPos;
            while (backspace(p_con) != p_con->cursorPos);
            reset(p_con);
            write_mode = 0;
        }
        return;
    }

    switch (ch) {
        case '\n':
            if (write_mode == 0) {
                if (p_con->cursor < p_con->original_addr +
                                    p_con->v_mem_limit - SCREEN_WIDTH) {
                    p_con->cursorLineStack[p_con->cursorLineStackTop++] = p_con->cursor;
                    p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
                                                           ((p_con->cursor - p_con->original_addr) /
                                                            SCREEN_WIDTH + 1);
                }
            } else if (write_mode == 1) {
                write_mode = 2;
                search(p_con);
            }

            break;
        case '\b':
            if (write_mode == 0 || write_mode == 1) {
                backspace(p_con);
            }
            break;
        case '\t':
            if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - TAB_WIDTH) {
                p_con->cursorLineStack[p_con->cursorLineStackTop++] = p_con->cursor;
                for (int i = 0; i < TAB_WIDTH; i++) {
                    *p_vmem++ = ' ';
                    *p_vmem++ = BRIGHT;
                    p_con->cursor++;
                }
            }
            break;
        case '\r':
            if (write_mode == 0) {
                p_con->cursorPos = p_con->cursor;
                write_mode = 1;
            } else if (write_mode == 1) {
                while (backspace(p_con) != p_con->cursorPos);
                write_mode = 0;
            }
            break;
        default:
            if (p_con->cursor <
                p_con->original_addr + p_con->v_mem_limit - 1) {
                p_con->cursorLineStack[p_con->cursorLineStackTop++] = p_con->cursor;
                *p_vmem++ = ch;
                if (write_mode == 1 && ch != ' ') {
                    *p_vmem++ = RED;
                } else {
                    *p_vmem++ = DEFAULT_CHAR_COLOR;
                }
                p_con->cursor++;
            }
            break;
    }

    while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
        scroll_screen(p_con, SCR_DN);
    }

    flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con) {
    set_cursor(p_con->cursor);
    set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position) {
    disable_int();
    out_byte(CRTC_ADDR_REG, CURSOR_H);
    out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, CURSOR_L);
    out_byte(CRTC_DATA_REG, position & 0xFF);
    enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr) {
    disable_int();
    out_byte(CRTC_ADDR_REG, START_ADDR_H);
    out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, START_ADDR_L);
    out_byte(CRTC_DATA_REG, addr & 0xFF);
    enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)    /* 0 ~ (NR_CONSOLES - 1) */
{
    if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
        return;
    }
    nr_current_console = nr_console;

    set_cursor(console_table[nr_console].cursor);
    set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction) {
    if (direction == SCR_UP) {
        if (p_con->current_start_addr > p_con->original_addr) {
            p_con->current_start_addr -= SCREEN_WIDTH;
        }
    } else if (direction == SCR_DN) {
        if (p_con->current_start_addr + SCREEN_SIZE <
            p_con->original_addr + p_con->v_mem_limit) {
            p_con->current_start_addr += SCREEN_WIDTH;
        }
    }

    set_video_start_addr(p_con->current_start_addr);
    set_cursor(p_con->cursor);
}

