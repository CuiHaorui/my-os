#define VGA_BUFFER 0xB8000
#define WIDTH  80
#define HEIGHT 25
int cursor_x = 0;
int cursor_y = 0;

unsigned char inb(unsigned short port)
{
    unsigned char ret;
    asm("inb %1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

void outb(unsigned short port, unsigned char data)
{
    asm("outb %1, %0" :: "d"(port), "a"(data));
}

void hide_cursor()
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void clear_screen()
{
    unsigned short *vga = (unsigned short*)0xB8000;
    for(int i=0; i < WIDTH * HEIGHT; i++){
        vga[i] = 0x0700;
    }
    cursor_x = cursor_y = 0;
}

void put_char(char c, unsigned char color = 0x07)
{
    unsigned char* vga = (unsigned char*)VGA_BUFFER;
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    if (cursor_x >= WIDTH)
    {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= HEIGHT)
    {
        clear_screen();
    }
    int pos = (cursor_y * WIDTH + cursor_x) * 2;
    vga[pos]     = c;
    vga[pos + 1] = color;
    cursor_x++;
}

void print(const char* str, unsigned char color = 0x07)
{
    while (*str)
    {
        put_char(*str, color);
        str++;
    }
}

char scancode_to_char(unsigned char sc)
{
    switch(sc)
    {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';

        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';

        case 0x39: return ' ';

        default: return 0;
    }
}

#define CMD_BUF_LEN 256
char cmd_buf[CMD_BUF_LEN];
int cmd_idx = 0;

void handle_cmd()
{
    cmd_buf[cmd_idx] = 0;
    if(cmd_buf[0] == char(0)){
        return;
    }

    // 关键修复：打印前先把光标移到下一行的行首
    put_char('\n');
    cursor_x = 0;

    if (cmd_buf[0] == 'c' && cmd_buf[1] == 'l' && cmd_buf[2] == 'e' && cmd_buf[3] == 'a' && cmd_buf[4] == 'r')
    {
        clear_screen();
        return;
    }

    if (cmd_buf[0] == 'h' && cmd_buf[1] == 'e' && cmd_buf[2] == 'l' && cmd_buf[3] == 'p')
    {
        print("Commands: help, clear", 10);
        return;
    }

    print("Unknown command", 12);
}

bool isShift = false;
bool isCtrl = false;
bool isAlt = false;

extern "C" void kernel_main()
{
    hide_cursor();
    clear_screen();
    print("MyOS C++ Kernel\n", 10);
    print("Real working OS CLI\n\n", 7);

    bool isCapsLock = false;

    while (1)
    {
        print("> ", 11);
        cmd_idx = 0;

        while (1)
        {
            if (!(inb(0x64) & 1)) continue;
            unsigned char sc = inb(0x60);

            // 按下 / 松开状态
            if (!(sc & 0x80)) {
                if (sc == 0x2A || sc == 0x36) isShift = true;
                if (sc == 0x1D) isCtrl = true;
                if (sc == 0x38) isAlt = true;
            } else {
                unsigned char code = sc - 0x80;
                if (code == 0x2A || code == 0x36) isShift = false;
                if (code == 0x1D) isCtrl = false;
                if (code == 0x38) isAlt = false;
            }

            if (sc & 0x80) continue;

            // 安全锁：防止输爆
            if (cmd_idx >= CMD_BUF_LEN - 1) continue;

            // CapsLock
            if (sc == 0x3A) isCapsLock = !isCapsLock;

            // 回车
            if (sc == 0x1C) {
                if(cmd_buf[0] == char(0)){
                    put_char('\n');
                    break;
                }
                handle_cmd();
                put_char('\n');
                break;
            }

            char c = scancode_to_char(sc);

            // 字母
            if ((c >= 'a' && c <= 'z')) {
                char out = c;
                if (isCapsLock) out = c - ' ';
                if (isShift)   out = c - ' ';

                cmd_buf[cmd_idx++] = out;
                put_char(out);
                continue;
            }

            // 数字
            if ((c >= '0' && c <= '9')) {
                if (isShift) {
                    switch(c) {
                        case '1': c='!'; break;
                        case '2': c='@'; break;
                        case '3': c='#'; break;
                        case '4': c='$'; break;
                        case '5': c='%'; break;
                        case '6': c='^'; break;
                        case '7': c='&'; break;
                        case '8': c='*'; break;
                        case '9': c='('; break;
                        case '0': c=')'; break;
                    }
                }
                cmd_buf[cmd_idx++] = c;
                put_char(c);
                continue;
            }

            // 符号
            if (isShift) {
                switch(sc) {
                    case 0x29: cmd_buf[cmd_idx++]='~'; put_char('~'); break;
                    case 0x0C: cmd_buf[cmd_idx++]='_'; put_char('_'); break;
                    case 0x0D: cmd_buf[cmd_idx++]='+'; put_char('+'); break;
                    case 0x1A: cmd_buf[cmd_idx++]='{'; put_char('{'); break;
                    case 0x1B: cmd_buf[cmd_idx++]='}'; put_char('}'); break;
                    case 0x2B: cmd_buf[cmd_idx++]='|'; put_char('|'); break;
                    case 0x27: cmd_buf[cmd_idx++]=':'; put_char(':'); break;
                    case 0x28: cmd_buf[cmd_idx++]='"'; put_char('"'); break;
                    case 0x33: cmd_buf[cmd_idx++]='<'; put_char('<'); break;
                    case 0x34: cmd_buf[cmd_idx++]='>'; put_char('>'); break;
                    case 0x35: cmd_buf[cmd_idx++]='?'; put_char('?'); break;
                }
            } else {
                switch(sc) {
                    case 0x29: cmd_buf[cmd_idx++]='`'; put_char('`'); break;
                    case 0x0C: cmd_buf[cmd_idx++]='-'; put_char('-'); break;
                    case 0x0D: cmd_buf[cmd_idx++]='='; put_char('='); break;
                    case 0x1A: cmd_buf[cmd_idx++]='['; put_char('['); break;
                    case 0x1B: cmd_buf[cmd_idx++]=']'; put_char(']'); break;
                    case 0x2B: cmd_buf[cmd_idx++]='\\';put_char('\\');break;
                    case 0x27: cmd_buf[cmd_idx++]=';'; put_char(';'); break;
                    case 0x28: cmd_buf[cmd_idx++]='\'';put_char('\'');break;
                    case 0x33: cmd_buf[cmd_idx++]=','; put_char(','); break;
                    case 0x34: cmd_buf[cmd_idx++]='.'; put_char('.'); break;
                    case 0x35: cmd_buf[cmd_idx++]='/'; put_char('/'); break;
                }
            }
        }
    }
}