#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "kbd.h"

static void consputc(int);

static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;

#define HISTORY_BUF 256
typedef enum { OP_INSERT, OP_DELETE } op_type;

typedef struct {
  op_type type;
  char c;
  uint pos;
} edit_op;

static struct {
  edit_op ops[HISTORY_BUF];
  uint h_index;
} history;


#define TAB_KEY         '\t'
#define MAX_COMPLETIONS 32
#define MAX_COMMANDS 64
#define MAX_CMD_LEN  32

static char dynamic_commands[MAX_COMMANDS][MAX_CMD_LEN];
static char* dynamic_command_ptrs[MAX_COMMANDS + 1];

static int tab_press_count = 0;


static int
kstrlen(const char* s)
{
    int n;
    for(n = 0; s[n]; n++)
        ;
    return n;
}

static int
kstrncmp(const char *p, const char *q, uint n)
{
    while(n > 0 && *p && *p == *q)
        n--, p++, q++;
    if(n == 0)
        return 0;
    return (uchar)*p - (uchar)*q;
}

static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}

void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}

void
panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1;
  for(;;)
    ;
}

#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);

static void
cgaputc(int c)
{
  int pos;

  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == BACKSPACE){
    if(pos > 0) --pos;
    crt[pos] = (' ' & 0xff) | 0x0700;
  } else if(c == KEY_LF) {
    if(pos > 0) --pos;
  } else if(c == KEY_RT) {
    if(pos < 24*80 - 1) ++pos;
  } else {
    crt[pos++] = (c&0xff) | 0x0700;
  }

  if(pos < 0 || pos > 25*80)
    panic("pos under/overflow");

  if((pos/80) >= 24){
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
}

void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else {
    uartputc(c);
  }
  cgaputc(c);
}


#define INPUT_BUF 128
struct {
  char buf[INPUT_BUF];
  uint r;
  uint w;
  uint e;
  uint mouse_pos;
  int start_pos;
} input;

static int selection_mark = -1;
static int selection_start = -1;
static int selection_end = -1;
static char clipboard_buf[INPUT_BUF];
static int clipboard_len = 0;


static char last_prefix[INPUT_BUF];

static void
log_op(op_type type, char c, uint pos)
{
  if (history.h_index < HISTORY_BUF) {
    history.ops[history.h_index].type = type;
    history.ops[history.h_index].c = c;
    history.ops[history.h_index].pos = pos;
    history.h_index++;
  }
}


static int
find_completions(const char* prefix, const char** completions)
{
  int count = 0;
  int prefix_len = kstrlen(prefix);

  if (prefix_len == 0) return 0;

  for (int i = 0; dynamic_command_ptrs[i] != 0 && count < MAX_COMPLETIONS; i++) {
    if (kstrncmp(prefix, dynamic_command_ptrs[i], prefix_len) == 0) {
      completions[count++] = dynamic_command_ptrs[i];
    }
  }
  return count;
}

static int
find_longest_common_prefix(const char** completions, int count, char* lcp_buf)
{
    if (count <= 0) {
        lcp_buf[0] = '\0';
        return 0;
    }

    const char* first_str = completions[0];
    int lcp_len = kstrlen(first_str);

    for (int i = 1; i < count; i++) {
        int j = 0;
        while (j < lcp_len && j < kstrlen(completions[i]) && first_str[j] == completions[i][j]) {
            j++;
        }
        if (j < lcp_len) {
            lcp_len = j;
        }
    }
    for (int i = 0; i < lcp_len; i++) {
        lcp_buf[i] = first_str[i];
    }
    lcp_buf[lcp_len] = '\0';
    return lcp_len;
}

static void
handle_tab_completion(void)
{
  char prefix[INPUT_BUF];
  int len = 0;
  for(uint i = input.w; i < input.e; i++) {
      if (input.buf[i % INPUT_BUF] == ' ') {
          return;
      }
      prefix[len++] = input.buf[i % INPUT_BUF];
  }
  prefix[len] = '\0';

  if (kstrncmp(prefix, last_prefix, INPUT_BUF) != 0) {
    tab_press_count = 0;
    memmove(last_prefix, prefix, len + 1);
  }
  tab_press_count++;

  const char* completions[MAX_COMPLETIONS];
  int num_completions = find_completions(prefix, completions);

  if (num_completions == 0) {
    return;
  } else if (num_completions == 1) {
    const char* completion = completions[0];
    int comp_len = kstrlen(completion);

    while(input.e > input.w) {
      input.e--;
      consputc(BACKSPACE);
    }
    input.mouse_pos = input.w;

    for(int i = 0; i < comp_len; i++) {
      input.buf[input.e % INPUT_BUF] = completion[i];
      input.e++;
      consputc(completion[i]);
    }

    input.buf[input.e % INPUT_BUF] = ' ';
    input.e++;
    consputc(' ');

    input.mouse_pos = input.e;
    
    tab_press_count = 0;
    last_prefix[0] = '\0';

  } else {
    if (tab_press_count == 1) {
        char lcp_buf[INPUT_BUF];
        int lcp_len = find_longest_common_prefix(completions, num_completions, lcp_buf);
        int prefix_len = kstrlen(prefix);

        if (lcp_len > prefix_len) {
            while(input.e > input.w) {
              input.e--;
              consputc(BACKSPACE);
            }
            input.mouse_pos = input.w;
            
            for(int i = 0; i < lcp_len; i++) {
              input.buf[input.e % INPUT_BUF] = lcp_buf[i];
              input.e++;
              consputc(lcp_buf[i]);
            }
            input.mouse_pos = input.e;

            memmove(last_prefix, lcp_buf, lcp_len + 1);
        }
    } else { 
      consputc('\n');
      for (int i = 0; i < num_completions; i++) {
        cprintf("%s  ", completions[i]);
      }
      consputc('\n');
      
      cprintf("$ ");
      for(uint i = input.w; i < input.e; i++) {
        consputc(input.buf[i % INPUT_BUF]);
      }
    }
  }
}

static int
get_cursor_pos(void)
{
  int pos;
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);
  return pos;
}

static void
clear_selection_highlight(void)
{
  if (selection_start == -1)
    return;

  int screen_start = input.start_pos + (selection_start - input.w);
  int screen_end = input.start_pos + (selection_end - input.w);

  for (int i = screen_start; i < screen_end; i++) {
    crt[i] = (crt[i] & 0xff) | 0x0700;
  }

  selection_start = -1;
  selection_end = -1;
  selection_mark = -1;
}


#define C(x)  ((x)-'@')
void
consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):
      doprocdump = 1;
      break;
    case C('U'):
      clear_selection_highlight();
      while(input.e != input.w &&
            input.buf[(input.e-1) % INPUT_BUF] != '\n'){
        log_op(OP_DELETE, input.buf[(input.e - 1) % INPUT_BUF], input.e - 1);
        input.e--;
        input.mouse_pos--;
        consputc(BACKSPACE);
      }
      tab_press_count = 0;
      last_prefix[0] = '\0';
      break;
    case C('H'): case '\x7f':
      if (selection_start != -1 && selection_end != -1) {
        uint del_start = selection_start;
        uint del_end = selection_end;
        int del_count = del_end - del_start;

        if (del_count > 0) {
          memmove(&input.buf[del_start % INPUT_BUF], &input.buf[del_end % INPUT_BUF], input.e - del_end);
          input.e -= del_count;
          input.mouse_pos = del_start;

          int current_screen_pos = get_cursor_pos();
          int target_screen_pos = input.start_pos + (del_start - input.w);
          for (int i = 0; i < current_screen_pos - target_screen_pos; i++) {
            consputc(KEY_LF);
          }

          for (uint i = del_start; i < input.e; i++) {
            consputc(input.buf[i % INPUT_BUF]);
          }

          for (int i = 0; i < del_count; i++) {
            consputc(' ');
          }

          current_screen_pos = get_cursor_pos();
          target_screen_pos = input.start_pos + (input.mouse_pos - input.w);
            for (int i = 0; i < current_screen_pos - target_screen_pos; i++) {
            consputc(KEY_LF);
          }
        }
        
        clear_selection_highlight();

      } else { 
        if(input.mouse_pos > input.w && input.e > input.r){
          log_op(OP_DELETE, input.buf[(input.mouse_pos - 1) % INPUT_BUF], input.mouse_pos - 1);
          if(input.mouse_pos == input.e) {
            input.e--;
            input.mouse_pos--;
            consputc(BACKSPACE);
          } else {
            int pos = input.mouse_pos;
            for(int i = pos; i < input.e; i++) {
              input.buf[(i-1) % INPUT_BUF] = input.buf[i % INPUT_BUF];
            }
            input.e--;
            input.mouse_pos--;
            consputc(KEY_LF);
            for(int i = input.mouse_pos; i < input.e; i++) {
              consputc(input.buf[i % INPUT_BUF]);
            }
            consputc(' ');
            for(int i = input.mouse_pos; i <= input.e; i++) {
              consputc(KEY_LF);
            }
          }
        }
      }
      tab_press_count = 0;
      last_prefix[0] = '\0';
      break;

    case C('S'):
      if (selection_mark == -1) {
        clear_selection_highlight();
        selection_mark = input.mouse_pos;
      } else {
        uint start = selection_mark;
        uint end = input.mouse_pos;

        if (start > end) {
          uint tmp = start;
          start = end;
          end = tmp;
        }

        if (start == end) {
          selection_mark = -1;
          break;
        }

        selection_start = start;
        selection_end = end;

        int screen_start = input.start_pos + (selection_start - input.w);
        int screen_end = input.start_pos + (selection_end - input.w);

        for (int i = screen_start; i < screen_end; i++) {
          crt[i] = (crt[i] & 0xff) | 0x7000;
        }
        selection_mark = -1;
      }
      break;

    case C('C'):
      if (selection_start != -1 && selection_end != -1) {
        int len = selection_end - selection_start;
        if (len > 0 && len < INPUT_BUF) {
          for (int i = 0; i < len; i++) {
            clipboard_buf[i] = input.buf[(selection_start + i) % INPUT_BUF];
          }
          clipboard_len = len;
          clipboard_buf[clipboard_len] = '\0';
        }
        clear_selection_highlight();
      }
      break;

    case C('V'):
      if (clipboard_len > 0 && (input.e - input.r + clipboard_len < INPUT_BUF)) {
        clear_selection_highlight();
        
        for (int i = input.e - 1; i >= (int)input.mouse_pos; i--) {
            input.buf[(i + clipboard_len) % INPUT_BUF] = input.buf[i % INPUT_BUF];
        }

        for (int i = 0; i < clipboard_len; i++) {
            input.buf[(input.mouse_pos + i) % INPUT_BUF] = clipboard_buf[i];
        }
        
        input.e += clipboard_len;
        
        for (uint i = input.mouse_pos; i < input.e; i++) {
            consputc(input.buf[i % INPUT_BUF]);
        }
        
        input.mouse_pos += clipboard_len;
        
        for (uint i = input.mouse_pos; i < input.e; i++) {
            consputc(KEY_LF);
        }
      }
      break;

    case TAB_KEY:
      clear_selection_highlight();
      handle_tab_completion();
      break;

    case KEY_LF:
      {
        uint effective_end = input.e;
        while (effective_end > input.w && input.buf[(effective_end - 1) % INPUT_BUF] == ' ') {
          effective_end--;
        }

        if (input.mouse_pos > effective_end) {
          int moves = input.mouse_pos - effective_end;
          input.mouse_pos = effective_end;
          for (int i = 0; i < moves; i++) {
            consputc(KEY_LF);
          }
        } 
        else if (input.mouse_pos > input.w) {
          input.mouse_pos--;
          consputc(KEY_LF);
        }
      }
      break;
    case KEY_RT:
      {
        uint effective_end = input.e;
        while (effective_end > input.w && input.buf[(effective_end - 1) % INPUT_BUF] == ' ') {
          effective_end--;
        }

        if (input.mouse_pos < effective_end) {
          consputc(KEY_RT);
          input.mouse_pos++;
        }
      }
      break;
    case C('Z'):
      clear_selection_highlight();
      if (history.h_index > 0) {
        history.h_index--;
        edit_op* op = &history.ops[history.h_index];

        if (op->type == OP_INSERT) {
          for (uint i = op->pos; i < input.e; i++) {
            input.buf[i % INPUT_BUF] = input.buf[(i + 1) % INPUT_BUF];
          }
          input.e--;
          
          if(input.mouse_pos > op->pos) for(uint i = 0; i < input.mouse_pos - op->pos; i++) consputc(KEY_LF);
          else for(uint i = 0; i < op->pos - input.mouse_pos; i++) consputc(KEY_RT);

          for (uint i = op->pos; i < input.e; i++) consputc(input.buf[i % INPUT_BUF]);
          consputc(' ');
          for (uint i = op->pos; i <= input.e; i++) consputc(KEY_LF);
          input.mouse_pos = op->pos;

        } else {
          if (input.e - input.r >= INPUT_BUF) {
             history.h_index++;
             break;
          }
          for (uint i = input.e; i > op->pos; i--) {
            input.buf[i % INPUT_BUF] = input.buf[(i - 1) % INPUT_BUF];
          }
          input.buf[op->pos % INPUT_BUF] = op->c;
          input.e++;
          
          if(input.mouse_pos > op->pos) for(uint i = 0; i < input.mouse_pos - op->pos; i++) consputc(KEY_LF);
          else for(uint i = 0; i < op->pos - input.mouse_pos; i++) consputc(KEY_RT);
          
          for (uint i = op->pos; i < input.e; i++) consputc(input.buf[i % INPUT_BUF]);
          for (uint i = op->pos + 1; i < input.e; i++) consputc(KEY_LF);
          input.mouse_pos = op->pos + 1;
        }
      }
      break;
    case C('D'):
          if (input.mouse_pos < input.e) {
            uint pos = input.mouse_pos;
            while (pos < input.e && input.buf[pos % INPUT_BUF] != ' ') {
              pos++;
            }
            while (pos < input.e && input.buf[pos % INPUT_BUF] == ' ') {
              pos++;
            }
            int moves = pos - input.mouse_pos;
            input.mouse_pos = pos;
            for (int i = 0; i < moves; i++) {
              consputc(KEY_RT);
            }
          }
          break;
    case C('A'):
      if (input.mouse_pos > input.w) {
        uint new_pos = input.mouse_pos;

        if (input.buf[(new_pos - 1) % INPUT_BUF] == ' ') {
          while (new_pos > input.w && input.buf[(new_pos - 1) % INPUT_BUF] == ' ') {
            new_pos--;
          }
          while (new_pos > input.w && input.buf[(new_pos - 1) % INPUT_BUF] != ' ') {
            new_pos--;
          }
        } else {
          while (new_pos > input.w && input.buf[(new_pos - 1) % INPUT_BUF] != ' ') {
            new_pos--;
          }
        }
        int moves = input.mouse_pos - new_pos;
        input.mouse_pos = new_pos;
        for (int i = 0; i < moves; i++) {
          consputc(KEY_LF);
        }
      }
      break;
    default:
      if(c != 0 && input.e - input.r < INPUT_BUF){
        if (selection_start != -1 && selection_end != -1) {
          uint del_start = selection_start;
          uint del_end = selection_end;
          int del_count = del_end - del_start;

          if (del_count > 0) {
          
            memmove(&input.buf[del_start % INPUT_BUF], &input.buf[del_end % INPUT_BUF], input.e - del_end);
            input.e -= del_count;
            input.mouse_pos = del_start;

            int current_screen_pos = get_cursor_pos();
            int target_screen_pos = input.start_pos + (del_start - input.w);
            for (int i = 0; i < current_screen_pos - target_screen_pos; i++) consputc(KEY_LF);
            for (uint i = del_start; i < input.e; i++) consputc(input.buf[i % INPUT_BUF]);
            for (int i = 0; i < del_count; i++) consputc(' ');
            current_screen_pos = get_cursor_pos();
            target_screen_pos = input.start_pos + (input.mouse_pos - input.w);
            for (int i = 0; i < current_screen_pos - target_screen_pos; i++) consputc(KEY_LF);
          }
          clear_selection_highlight();
        }
        
        if (input.w == input.e) {
            input.start_pos = get_cursor_pos();
        }

        c = (c == '\r') ? '\n' : c;
        log_op(OP_INSERT, c, input.mouse_pos);
        if(input.mouse_pos < input.e) {
          for(int i = input.e; i > input.mouse_pos; i--) {
            input.buf[i % INPUT_BUF] = input.buf[(i-1) % INPUT_BUF];
          }
        }
        input.buf[input.mouse_pos % INPUT_BUF] = c;
        input.e++;
        if(input.mouse_pos == input.e - 1) {
          consputc(c);
        } else {
          for(int i = input.mouse_pos; i < input.e; i++) {
            consputc(input.buf[i % INPUT_BUF]);
          }
          for(int i = input.mouse_pos + 1; i < input.e; i++) {
            consputc(KEY_LF);
          }
        }
        input.mouse_pos++;
        if(c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF){
          selection_mark = -1;
          history.h_index = 0;
          input.w = input.e;
          wakeup(&input.r);
        }
      }
      tab_press_count = 0;
      last_prefix[0] = '\0';
      break;
    }
  }
  release(&cons.lock);
  if(doprocdump) {
    procdump();
  }
}

int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(input.r == input.w){
      if(myproc()->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){
      if(n < target){
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
  iunlock(ip);
  acquire(&cons.lock);

  if (n > 4 && (uchar)buf[0] == 1 && (uchar)buf[1] == 1 && (uchar)buf[n-2] == 2 && (uchar)buf[n-1] == 2) {
    char *p = buf + 2;
    char *end = buf + n - 2;
    int cmd_idx = 0;
    int char_idx = 0;

    memset(dynamic_commands, 0, sizeof(dynamic_commands));
    
    while (p < end && cmd_idx < MAX_COMMANDS) {
      if (*p == '\n') {
        dynamic_commands[cmd_idx][char_idx] = '\0';
        dynamic_command_ptrs[cmd_idx] = dynamic_commands[cmd_idx];
        cmd_idx++;
        char_idx = 0;
      } else if (char_idx < MAX_CMD_LEN - 1) {
        dynamic_commands[cmd_idx][char_idx++] = *p;
      }
      p++;
    }
    dynamic_command_ptrs[cmd_idx] = 0;

    release(&cons.lock);
    ilock(ip);
    return n;
  }

  for (int i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  
  release(&cons.lock);
  ilock(ip);

  return n;
}

void
consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;
  history.h_index = 0;

  ioapicenable(IRQ_KBD, 0);
}