#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "api.h"
#include "common/constants.h"
#include "parser.h"
#include "../common/messager.h"

int main(int argc, char* argv[]) {
  if (argc < 5) {
    fprintf(stderr, "Usage: %s <request pipe path> <response pipe path> <server pipe path> <.jobs file path>\n",
            argv[0]);
    return 1;
  }
  
  // Create new session
  int session_id=ems_setup(argv[1], argv[2], argv[3]);
  if (session_id==-1) {
    fprintf(stderr, "Failed to set up EMS\n");
    return 1;
  }
  
  const char* dot = strrchr(argv[4], '.');
  if (dot == NULL || dot == argv[4] || strlen(dot) != 5 || strcmp(dot, ".jobs") ||
      strlen(argv[4]) > MAX_JOB_FILE_NAME_SIZE) {
    fprintf(stderr, "The provided .jobs file path is not valid. Path: %s\n", argv[1]);
    return 1;
  }

  char out_path[MAX_JOB_FILE_NAME_SIZE];
  strcpy(out_path, argv[4]);
  strcpy(strrchr(out_path, '.'), ".out");

  // Open input file
  int in_fd = open(argv[4], O_RDONLY);
  if (in_fd == -1) {
    fprintf(stderr, "Failed to open input file. Path: %s\n", argv[4]);
    return 1;
  }

  // Open output file
  int out_fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (out_fd == -1){
    fprintf(stderr, "Failed to open output file. Path: %s\n", out_path);
    return 1;
  }


  while (1) {
    unsigned int event_id;
    size_t num_rows, num_columns, num_coords;
    unsigned int delay = 0;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

    switch (get_next(in_fd)) {
      case CMD_CREATE: // Handle create command
        if (parse_create(in_fd, &event_id, &num_rows, &num_columns) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_create(event_id, num_rows, num_columns, session_id)) fprintf(stderr, "Failed to create event\n");
        break;

      case CMD_RESERVE: // Handle reserve command
        num_coords = parse_reserve(in_fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);

        if (num_coords == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_reserve(event_id, num_coords, xs, ys, session_id)) fprintf(stderr, "Failed to reserve seats\n");
        break;

      case CMD_SHOW: // Handle show command
        if (parse_show(in_fd, &event_id) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_show(out_fd, event_id, session_id)) fprintf(stderr, "Failed to show event\n");
        break;

      case CMD_LIST_EVENTS: // Handle list command
        if (ems_list_events(out_fd, session_id)) fprintf(stderr, "Failed to list events\n");
        break;

      case CMD_WAIT: // Handle wait command
        if (parse_wait(in_fd, &delay, NULL) == -1) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
        }

        if (delay > 0) {
            printf("Waiting...\n");
            sleep(delay);
        }
        break;

      case CMD_INVALID: // Handle invalid command
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP: // Handle help command
        printf(
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms>\n"
            "  HELP\n");

        break;

      case CMD_EMPTY: // Empty command
        break;

      case EOC: // Handle EOC command
        close(in_fd);
        close(out_fd);
        if(ems_quit(session_id, argv[1], argv[2]))
          return 1;
        return 0;
    }
  }
}