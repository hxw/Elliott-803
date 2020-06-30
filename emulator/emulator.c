// emulator.c

#define NCURSES_WIDECHAR 1

#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h> // struct sockaddr
#include <unistd.h>    // sleep
#include <wctype.h>

#include "commands.h"
#include "cpu/elliott803.h"
#include "emulator.h"
#include "io5/io5.h"

#if !defined(SizeOfArray)
#define SizeOfArray(a) (sizeof(a) / sizeof((a)[0]))
#endif

// types
typedef enum {
  pad_punch1 = 0,
  pad_punch2 = 1,
  pad_punch3 = 2,
  pad_console = 3,

  pad_count = 4, // total number of pads
} pad_select_t;

typedef struct {
  const int status_lines;
  const int text_lines;
  const int text_cols;
  const int text_y;
  const int text_x;
  const int status_cols;
  const int status_y;
  const int status_x;
} layout_t;

typedef struct {
  WINDOW *pad[pad_count];
  pad_select_t select;
  WINDOW *status;
} pads_t;

typedef struct {
  wint_t buffer[100];
  int width[100];
  size_t index;
} key_buffer_t;

// handlers
static void handle_proc_fd(commands_t *cmd,
                           pads_t *pads,
                           const layout_t *layout,
                           io5_conv_t *conv_punch[3]);
static bool handle_key(commands_t *cmd,
                       pads_t *pads,
                       const layout_t *layout,
                       key_buffer_t *kb,
                       FILE *script);

void set_pad(pads_t *pads, const layout_t *layout, pad_select_t next) {
  pads->select = next;
  prefresh(pads->pad[pads->select],
           0,
           0,
           1,
           1,
           layout->text_lines + 1,
           layout->text_cols + 1);
}

// main program
// displays a curses UI
int emulator(const char *name,
             const char *version,
             FILE *script,
             bool interactive,
             int argc,
             char *argv[]) {

  // ensure locale is properly set up
  const char *l = setlocale(LC_ALL, "");

  commands_t cmd;
  memset(&cmd, 0, sizeof(cmd));
  cmd.exit_program = false;

  cmd.proc = elliott803_create("Elliott 803B");
  if (NULL == cmd.proc) {
    printf("failed to create processor\n");
    goto fail;
  }

  for (size_t i = 0; i < commands_io_count; ++i) {
    cmd.file[i] = io5_file_allocate();
    if (NULL == cmd.file[i]) {
      printf("failed to create io5_file: %zu\n", i);
      goto fail;
    }
  }

  int proc_fd = elliott803_get_fd(cmd.proc);

  io5_conv_t *conv_punch[3];
  memset(conv_punch, 0, sizeof(conv_punch));
  for (size_t i = 0; i < SizeOfArray(conv_punch); ++i) {
    conv_punch[i] = io5_conv_allocate(io5_mode_binary, io5_mode_elliott);
    if (NULL == conv_punch[i]) {
      printf("failed to create punch; %zu converter\n", i);
      goto fail;
    }
  }

  // turn off console echo
  initscr();
  cbreak();
  noecho();
  timeout(0);
  raw();

  nonl();
  int old_cursor = curs_set(2); // 0=no, 1=normal, 2=very visible
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);

  // compute the window sizes
  const layout_t layout = {
    .status_lines = 4,
    .text_lines = LINES - 3 - layout.status_lines,
    .text_cols = COLS - 2,
    .text_y = 1,
    .text_x = 1,
    .status_cols = COLS - 2,
    .status_y = layout.text_lines + 2,
    .status_x = 1,
  };
  WINDOW *background = newwin(LINES, COLS, 0, 0);
  keypad(background, false);

  box(background, 0, 0);

  // line above status window
  mvwhline(background, layout.status_y - 1, 1, 0, COLS - 2);

  scrollok(background, false);
  wrefresh(background);

  // create pad data stores
  pads_t pads;
  memset(&pads, 0, sizeof(pads));

  pads.select = pad_console;
  for (int i = 0; i < SizeOfArray(pads.pad); ++i) {
    pads.pad[i] = newpad(layout.text_lines, layout.text_cols);
    scrollok(pads.pad[i], true);
  }

  WINDOW *text =
    newwin(layout.text_lines, layout.text_cols, layout.text_y, layout.text_x);
  keypad(text, true);
  scrollok(text, true);
  mvwprintw(text, 0, 0, "%s version: %s initialising…", name, version);
  mvwprintw(text, 1, 0, "locale: %s\n\n", l);
  wrefresh(text);

  // napms(500);

  pads.status = newwin(
    layout.status_lines, layout.status_cols, layout.status_y, layout.status_x);
  scrollok(pads.status, true);
  keypad(pads.status, true);

  while (!cmd.exit_program) {
    wprintw(pads.status, "command: ");
    wrefresh(pads.status);

    key_buffer_t kb;
    memset(&kb, 0, sizeof(kb));
    kb.index = 0;

    while (!cmd.exit_program) {

      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);
      FD_SET(proc_fd, &fds);

      // timeout for select
      struct timeval tv;
      tv.tv_sec = 1;
      tv.tv_usec = 0;

      int rc = select(FD_SETSIZE, &fds, NULL, NULL, &tv);

      if (-1 == rc) {
        // if interrupted system call, just retry
        if (EINTR != errno) {
          fprintf(stderr, "select error: %s\n", strerror(errno));
          goto fail;
        }
        continue;
      }
      if (rc > 0) {
        if (FD_ISSET(proc_fd, &fds)) {
          handle_proc_fd(&cmd, &pads, &layout, conv_punch);
        }
        // only possibility here is keyboard
        if (FD_ISSET(STDIN_FILENO, &fds)) {
          if (handle_key(&cmd, &pads, &layout, &kb, script)) {
            break;
          }
        }
        continue;
      }

      // only get here on a timeout

      if (cmd.wait) {
        beep();
        sleep(1);
        elliott803_send(cmd.proc, "check", 5);
      }

      // if executing script
      if (NULL != script) {
        wchar_t buffer[100];
        memset(buffer, 0, sizeof(buffer));
        size_t index = 0;
        for (;;) {
          wint_t c = fgetwc(script);
          if (WEOF == c) {
            script = NULL;
            break;
          } else if (L'\r' == c || L'\n' == c) {
            break;
          }
          if (index < sizeof(buffer) - 1) {
            buffer[index] = c;
            ++index;
            buffer[index] = L'\0';
          }
        }
        wprintw(pads.pad[pad_console], "> %ls\n", buffer);
        commands_run(&cmd, buffer, sizeof(buffer));
        if (NULL != cmd.error) {
          // wprintw(pads.status, "%ls\n", cmd.error);
          wprintw(pads.pad[pad_console], "%ls\n", cmd.error);
          free((void *)cmd.error);
          cmd.error = NULL;
        }
        // special for screen N
        switch (cmd.screen) {
        case 1:
          set_pad(&pads, &layout, pad_punch1);
          break;
        case 2:
          set_pad(&pads, &layout, pad_punch2);
          break;
        case 3:
          set_pad(&pads, &layout, pad_punch3);
          break;
        case 4:
          set_pad(&pads, &layout, pad_console);
          break;
        default:
          break;
        }
        cmd.screen = 0;
        if (pad_console == pads.select) {
          prefresh(pads.pad[pads.select],
                   0,
                   0,
                   1,
                   1,
                   layout.text_lines + 1,
                   layout.text_cols + 1);
        }
      }
    }
  }

  if (NULL != cmd.proc) {
    elliott803_destroy(cmd.proc);
  }

  for (size_t i = 0; i < commands_io_count; ++i) {
    if (NULL == cmd.file[i]) {
      io5_file_allocate(cmd.file[i]);
    }
  }

  for (size_t i = 0; i < SizeOfArray(conv_punch); ++i) {
    if (NULL != conv_punch[i]) {
      io5_conv_deallocate(conv_punch[i]);
    }
  }

  for (int i = 0; i < SizeOfArray(pads.pad); ++i) {
    delwin(pads.pad[i]);
  }

  if (NULL != pads.status) {
    delwin(pads.status);
  }
  if (NULL != text) {
    delwin(text);
  }
  if (NULL != background) {
    delwin(background);
  }
  curs_set(old_cursor);

  endwin();

  return 0;

fail:
  return EXIT_FAILURE;
}

void handle_proc_fd(commands_t *cmd,
                    pads_t *pads,
                    const layout_t *layout,
                    io5_conv_t *conv_punch[3]) {
  char in_buffer[1024];
  ssize_t n = elliott803_receive(cmd->proc, in_buffer, sizeof(in_buffer) - 1);
  if (n > 0) {
    in_buffer[n + 1] = '\0';

    pad_select_t pad_modified = pad_console;

    if (0 == strncmp("check ", in_buffer, 6)) {
      if (cmd->wait) {
        switch (in_buffer[6]) {
        case 'r':
          break;
        case 's':
          cmd->wait = false;
          cmd->wait_delay = 0;
          break;
        default:
          if (cmd->wait_delay < 1) {
            cmd->wait = false;
            cmd->wait_delay = 0;
          } else {
            --cmd->wait_delay;
          }
          break;
        }
      }

    } else if (0 == strncmp("p1 ", in_buffer, 3) ||
               0 == strncmp("p2 ", in_buffer, 3) ||
               0 == strncmp("p3 ", in_buffer, 3)) {

      pad_modified = pad_punch1;
      io5_conv_t *conv = conv_punch[0];
      io5_file_t *f = NULL;
      switch (in_buffer[1]) {
      case '1':
        f = cmd->file[commands_punch_1];
        break;
      case '2':
        pad_modified = pad_punch2;
        conv = conv_punch[1];
        f = cmd->file[commands_punch_2];
        break;
      case '3':
        pad_modified = pad_punch3;
        conv = conv_punch[2];
        break;
      }
      int c = 0;
      sscanf(&in_buffer[3], "%02x", &c);

      uint8_t b[256];
      b[0] = c;

      if (NULL != f) {
        io5_file_write(f, b, 1);
      }

      io5_conv_put(conv, b, 1);
      ssize_t n = io5_conv_get(conv, b, sizeof(b));
      b[n] = '\0';

      if ('\r' == b[0]) {
        b[0] = '\0';
      }
      wprintw(pads->pad[pad_modified], "%s", b);

    } else if (0 == strncmp("r1 ", in_buffer, 3) ||
               0 == strncmp("r2 ", in_buffer, 3) ||
               0 == strncmp("r3 ", in_buffer, 3)) {

      int unit = 1;
      io5_file_t *f = NULL;

      switch (in_buffer[1]) {
      case '1':
        f = cmd->file[commands_reader_1];
        break;
      case '2':
        unit = 2;
        f = cmd->file[commands_reader_2];
        break;
      case '3':
        unit = 3;
        break;
      }
      if (NULL != f) {
        uint8_t read_buffer[32]; // max bytes is 32
        ssize_t n = io5_file_read(f, read_buffer, sizeof(read_buffer));
        if (n >= 1) {
          char packet[256];
          size_t l = sizeof(packet);
          memset(packet, 0, sizeof(packet));
          int i = snprintf(packet, sizeof(packet), "reader %d ", unit);
          l -= i;
          for (int j = 0; j < n; ++j) {
            int k = snprintf(&packet[i], l, "%02x", read_buffer[j]);
            i += k;
            l -= k;
          }
          elliott803_send(cmd->proc, packet, i);
        }
      }

    } else {
      wprintw(pads->pad[pad_console], "%s\n", in_buffer);
    }
    // refresh if current pad is on the screen
    if (pads->select == pad_modified) {
      prefresh(pads->pad[pads->select],
               0,
               0,
               1,
               1,
               layout->text_lines + 1,
               layout->text_cols + 1);
    }
    wrefresh(pads->status); // ensure cursor is right
  }
}

bool handle_key(commands_t *cmd,
                pads_t *pads,
                const layout_t *layout,
                key_buffer_t *kb,
                FILE *script) {

  wint_t c = 0;
  int rc = wget_wch(pads->status, &c);
  if (OK == rc) {
    if (cmd->wait || NULL != script) {
      memset(kb->buffer, 0, sizeof(kb->buffer));
      kb->index = 0;
      beep();
      return false;
    }
    if ('\n' == c || '\r' == c) {
      wprintw(pads->status, "\n");

      wprintw(pads->pad[pad_console], "> %ls\n", kb->buffer);
      commands_run(cmd, kb->buffer, sizeof(kb->buffer));
      if (NULL != cmd->error) {
        // wprintw(pads->status, "%ls\n", cmd.error);
        wprintw(pads->pad[pad_console], "%ls\n", cmd->error);
        free((void *)cmd->error);
        cmd->error = NULL;
      }
      if (pad_console == pads->select) {
        prefresh(pads->pad[pads->select],
                 0,
                 0,
                 1,
                 1,
                 layout->text_lines + 1,
                 layout->text_cols + 1);
      }

      // clear line buffer
      kb->index = 0;
      memset(kb->buffer, 0, sizeof(kb->buffer));

      // special for screen N
      switch (cmd->screen) {
      case 1:
        set_pad(pads, layout, pad_punch1);
        break;
      case 2:
        set_pad(pads, layout, pad_punch2);
        break;
      case 3:
        set_pad(pads, layout, pad_punch3);
        break;
      case 4:
        set_pad(pads, layout, pad_console);
        break;
      default:
        break;
      }
      cmd->screen = 0;
      wrefresh(pads->status); // ensure cursor is right
      return true;
    }
    if (iswprint(c)) {
      if (kb->index < sizeof(kb->buffer) - 1) {
        wchar_t wch[2];
        wch[0] = c;
        wch[1] = L'\0';

        int x, x1, y;
        getyx(pads->status, y, x);

        waddwstr(pads->status, wch);

        getyx(pads->status, y, x1);
        kb->width[kb->index] = x1 - x;

        kb->buffer[kb->index] = c;
        ++(kb->index);
        wrefresh(pads->status);
      } else {
        beep();
      }
    }
    return false;
  }

  switch (c) {
  case KEY_BACKSPACE:
    if (0 == kb->index) {
      beep();
    } else {
      --(kb->index);
      int x, y;
      getyx(pads->status, y, x);
      wmove(pads->status, y, x - 1);
      wdelch(pads->status);
      if (2 == kb->width[kb->index]) {
        getyx(pads->status, y, x);
        wmove(pads->status, y, x - 1);
        wdelch(pads->status);
      }
      kb->buffer[kb->index] = L'\0';
    }
    break;

  case KEY_F(1):
    set_pad(pads, layout, pad_punch1);
    break;

  case KEY_F(2):
    set_pad(pads, layout, pad_punch2);
    break;

  case KEY_F(3):
    set_pad(pads, layout, pad_punch3);
    break;

  case KEY_F(4):
    set_pad(pads, layout, pad_console);
    break;

  case KEY_F(7):
    cmd->wait_delay = 0;
    cmd->wait = false;
    break;

  case KEY_F(8):
    cmd->exit_program = true;
    break;
  }

  // ensure cursor is right
  wrefresh(pads->status);
  return false;
}
