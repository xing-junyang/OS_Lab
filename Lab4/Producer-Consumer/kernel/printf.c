
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                              printf.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"

/******************************************************************************************
                        可变参数函数调用原理（其中涉及的数字皆为举例）
===========================================================================================

i = 0x23;
j = 0x78;
char fmt[] = "%x%d";
printf(fmt, i, j);

        push    j
        push    i
        push    fmt
        call    printf
        add     esp, 3 * 4


                ┃        HIGH        ┃                        ┃        HIGH        ┃
                ┃        ...         ┃                        ┃        ...         ┃
                ┣━━━━━━━━━━┫                        ┣━━━━━━━━━━┫
                ┃                    ┃                 0x32010┃        '\0'        ┃
                ┣━━━━━━━━━━┫                        ┣━━━━━━━━━━┫
         0x3046C┃        0x78        ┃                 0x3200c┃         d          ┃
                ┣━━━━━━━━━━┫                        ┣━━━━━━━━━━┫
   arg = 0x30468┃        0x23        ┃                 0x32008┃         %          ┃
                ┣━━━━━━━━━━┫                        ┣━━━━━━━━━━┫
         0x30464┃      0x32000 ───╂────┐       0x32004┃         x          ┃
                ┣━━━━━━━━━━┫        │              ┣━━━━━━━━━━┫
                ┃                    ┃        └──→ 0x32000┃         %          ┃
                ┣━━━━━━━━━━┫                        ┣━━━━━━━━━━┫
                ┃        ...         ┃                        ┃        ...         ┃
                ┃        LOW         ┃                        ┃        LOW         ┃

实际上，调用 vsprintf 的情形是这样的：

        vsprintf(buf, 0x32000, 0x30468);

******************************************************************************************/

int vsprintf(char *buf, const char *fmt, va_list args) {
    char *p;
    char tmp[256];
    va_list p_next_arg = args;

    for (p = buf; *fmt; fmt++) {
        if (*fmt != '%') {
            *p++ = *fmt;
            continue;
        }

        fmt++;

        switch (*fmt) {
            case 'd':
                itoa(tmp, *((int *) p_next_arg));
                strcpy(p, tmp);
                p_next_arg += 4;
                p += strlen(tmp);
                break;
            case 'c':
                *p = *p_next_arg;
                p_next_arg += 4;
                p++;
                break;
            case 's':
                break;
            default:
                break;
        }
    }

    return (p - buf);
}

/*======================================================================*
                                 printf
 *======================================================================*/
int printf(const char *fmt, ...) {
	int i;
	char buf[256];
	
	va_list arg = (va_list) ((char *) (&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	i = vsprintf(buf, fmt, arg);
	write_str(buf, i);
	return i;
}

void puts(const char *str) {
	printf(str);
	printf("\n");
}