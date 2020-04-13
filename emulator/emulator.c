// emulator.c

#define NCURSES_WIDECHAR 1

#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h> // memset
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h> // struct sockaddr
#include <unistd.h>    // sleep
#include <wctype.h>

#include "commands.h"
#include "cpu/elliott803.h"
#include "io5/io5.h"

#if !defined(SizeOfArray)
#define SizeOfArray(a) (sizeof(a) / sizeof((a)[0]))
#endif

// main program
// displays a curses UI
int main(int argc, char *argv[]) {

  // ensure locale is properly set up
  const char *l = setlocale(LC_ALL, "");

  commands_t cmd;
  memset(&cmd, 0, sizeof(cmd));

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
  const int status_lines = 4;
  const int text_lines = LINES - 3 - status_lines;
  const int text_cols = COLS - 2;
  const int text_y = 1;
  const int text_x = 1;
  const int status_cols = COLS - 2;
  const int status_y = text_lines + 2;
  const int status_x = 1;

  WINDOW *background = newwin(LINES, COLS, 0, 0);
  keypad(background, false);

  box(background, 0, 0);

  // line above status window
  mvwhline(background, status_y - 1, 1, 0, COLS - 2);

  // waddwstr(background L"…→£");
  scrollok(background, false);
  wrefresh(background);

  // create pad data stores
  WINDOW *pad[4];
  typedef enum {
    pad_punch1 = 0,
    pad_punch2 = 1,
    pad_punch3 = 2,
    pad_console = 3,
  } pad_select_t;
  pad_select_t pad_select = pad_console;
  for (int i = 0; i < SizeOfArray(pad); ++i) {
    pad[i] = newpad(text_lines, text_cols);
    scrollok(pad[i], true);
  }

  WINDOW *text = newwin(text_lines, text_cols, text_y, text_x);
  keypad(text, true);
  scrollok(text, true);
  mvwprintw(text, 0, 0, "initialising…  `→ £´");
  mvwprintw(text, 1, 0, "locale: %s\n\n", l);
  wrefresh(text);

  // napms(500);

  WINDOW *status = newwin(status_lines, status_cols, status_y, status_x);
  scrollok(status, true);
  keypad(status, true);

  bool run = true;
  while (run) {
    wprintw(status, "command: ");
    wrefresh(status);

    wint_t buffer[100];
    int width[100];
    memset(buffer, 0, sizeof(buffer));
    memset(width, 0, sizeof(width));
    size_t index = 0;

    while (run) {

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
      if (FD_ISSET(proc_fd, &fds)) {
        char in_buffer[1024];
        ssize_t n =
            elliott803_receive(cmd.proc, in_buffer, sizeof(in_buffer) - 1);
        if (n > 0) {
          buffer[n + 1] = '\0';

          pad_select_t pad_modified = pad_console;

          if (0 == strncmp("p1 ", in_buffer, 3) ||
              0 == strncmp("p2 ", in_buffer, 3) ||
              0 == strncmp("p3 ", in_buffer, 3)) {

            pad_modified = pad_punch1;
            io5_conv_t *conv = conv_punch[0];
            io5_file_t *f = NULL;
            switch (in_buffer[1]) {
            case '1':
              f = cmd.file[commands_punch_1];
              break;
            case '2':
              pad_modified = pad_punch2;
              conv = conv_punch[1];
              f = cmd.file[commands_punch_2];
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
            wprintw(pad[pad_modified], "%s", b);

          } else if (0 == strncmp("r1 ", in_buffer, 3) ||
                     0 == strncmp("r2 ", in_buffer, 3) ||
                     0 == strncmp("r3 ", in_buffer, 3)) {

            int unit = 1;
            io5_file_t *f = NULL;

            switch (in_buffer[1]) {
            case '1':
              f = cmd.file[commands_reader_1];
              break;
            case '2':
              unit = 2;
              f = cmd.file[commands_reader_2];
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
                elliott803_send(cmd.proc, packet, i);
              }
            }

          } else {
            wprintw(pad[pad_console], "%s\n", in_buffer);
          }
          // refresh if current pad is on the screen
          if (pad_select == pad_modified) {
            prefresh(pad[pad_select], 0, 0, 1, 1, text_lines + 1,
                     text_cols + 1);
          }
          wrefresh(status); // ensure cursor is right
        }
      }

      if (!FD_ISSET(STDIN_FILENO, &fds)) {
        continue;
      }

      wint_t c = 0;
      rc = wget_wch(status, &c);
      if (OK == rc) {
        if ('\n' == c || '\r' == c) {
          wprintw(status, "\n");

          wchar_t *message = NULL;
          wprintw(pad[pad_console], "> %ls\n", buffer);
          run = commands_run(&cmd, buffer, sizeof(buffer), &message);
          if (NULL != message) {
            // wprintw(status, "%ls\n", message);
            wprintw(pad[pad_console], "%ls\n", message);
            free(message);
          }
          if (pad_console == pad_select) {
            prefresh(pad[pad_select], 0, 0, 1, 1, text_lines + 1,
                     text_cols + 1);
          }

          // clear line buffer
          index = 0;
          memset(buffer, 0, sizeof(buffer));

          break;
        }
        if (iswprint(c)) {
          if (index < sizeof(buffer) - 1) {
            wchar_t wch[2];
            wch[0] = c;
            wch[1] = L'\0';

            int x, x1, y;
            getyx(status, y, x);

            waddwstr(status, wch);

            getyx(status, y, x1);
            width[index] = x1 - x;

            buffer[index] = c;
            ++index;
            wrefresh(status);
          } else {
            beep();
          }
        }
        continue;
      }

      switch (c) {
      case KEY_BACKSPACE:
        if (0 == index) {
          beep();
        } else {
          --index;
          int x, y;
          getyx(status, y, x);
          wmove(status, y, x - 1);
          wdelch(status);
          if (2 == width[index]) {
            getyx(status, y, x);
            wmove(status, y, x - 1);
            wdelch(status);
          }

          wrefresh(status);
          buffer[index] = L'\0';
        }
        break;

      case KEY_F(1):
        pad_select = pad_punch1;
        prefresh(pad[pad_select], 0, 0, 1, 1, text_lines + 1, text_cols + 1);
        wrefresh(status); // ensure cursor is right
        break;

      case KEY_F(2):
        pad_select = pad_punch2;
        prefresh(pad[pad_select], 0, 0, 1, 1, text_lines + 1, text_cols + 1);
        wrefresh(status); // ensure cursor is right
        break;

      case KEY_F(3):
        pad_select = pad_punch3;
        prefresh(pad[pad_select], 0, 0, 1, 1, text_lines + 1, text_cols + 1);
        wrefresh(status); // ensure cursor is right
        break;

      case KEY_F(4):
        pad_select = pad_console;
        prefresh(pad[pad_select], 0, 0, 1, 1, text_lines + 1, text_cols + 1);
        wrefresh(status); // ensure cursor is right
        break;

      case KEY_F(7):
      case KEY_F(8):
        run = false;
        break;

      case KEY_F(9): {
        WINDOW *w1 = newwin(3, 40, LINES - 5, 20);
        keypad(w1, true);
        scrollok(w1, true);
        box(w1, 0, 0);
        mvwprintw(w1, 1, 1, "F3 to exit");
        wrefresh(w1);

        while (KEY_F(3) != wgetch(w1)) {
          beep();
        }
        delwin(w1);
        redrawwin(status);
        wrefresh(status);
        break;
      }

      case KEY_F(5):
        beep();
        break;
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

  for (int i = 0; i < SizeOfArray(pad); ++i) {
    delwin(pad[i]);
  }

  if (NULL != status) {
    delwin(status);
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
