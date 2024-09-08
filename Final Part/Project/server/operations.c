#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "common/io.h"
#include "common/messager.h"
#include "eventlist.h"
#include "aux.h"
#include "signals.h"

static struct EventList* event_list = NULL;
static unsigned int state_access_delay_us = 0;

/// Gets the event with the given ID from the state.
/// @note Will wait to simulate a real system accessing a costly memory resource.
/// @param event_id The ID of the event to get.
/// @param from First node to be searched.
/// @param to Last node to be searched.
/// @return Pointer to the event if found, NULL otherwise.
static struct Event* get_event_with_delay(unsigned int event_id, struct ListNode* from, struct ListNode* to) {
  struct timespec delay = {0, state_access_delay_us * 1000};
  nanosleep(&delay, NULL);  // Should not be removed
  return get_event(event_list, event_id, from, to);
}

/// Gets the index of a seat.
/// @note This function assumes that the seat exists.
/// @param event Event to get the seat index from.
/// @param row Row of the seat.
/// @param col Column of the seat.
/// @return Index of the seat.
static size_t seat_index(struct Event* event, size_t row, size_t col) { return (row - 1) * event->cols + col - 1; }

int ems_init(unsigned int delay_us) {
  if (event_list != NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "EMS state has already been initialized\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  event_list = create_list();
  state_access_delay_us = delay_us;

  return event_list == NULL;
}

int ems_terminate() {
  if (event_list == NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "EMS state must be initialized\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  if (pthread_rwlock_wrlock(&event_list->rwl) != 0) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Error locking list rwl\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  free_list(event_list);
  pthread_rwlock_unlock(&event_list->rwl);
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  if (event_list == NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "EMS state must be initialized\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  if (pthread_rwlock_wrlock(&event_list->rwl) != 0) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Error locking list rwl\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  if (get_event_with_delay(event_id, event_list->head, event_list->tail) != NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Event already exists\n");
    pthread_mutex_unlock(&print_mutex);
    pthread_rwlock_unlock(&event_list->rwl);
    return 1;
  }

  struct Event* event = malloc(sizeof(struct Event));

  if (event == NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Error allocating memory for event\n");
    pthread_mutex_unlock(&print_mutex);

    pthread_rwlock_unlock(&event_list->rwl);
    return 1;
  }

  event->id = event_id;
  event->rows = num_rows;
  event->cols = num_cols;
  event->reservations = 0;
  if (pthread_mutex_init(&event->mutex, NULL) != 0) {
    pthread_rwlock_unlock(&event_list->rwl);
    free(event);
    return 1;
  }
  event->data = calloc(num_rows * num_cols, sizeof(unsigned int));

  if (event->data == NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Error allocating memory for event data\n");
    pthread_mutex_unlock(&print_mutex);
    pthread_rwlock_unlock(&event_list->rwl);
    free(event);
    return 1;
  }

  if (append_to_list(event_list, event) != 0) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Error appending event to list\n");
    pthread_mutex_unlock(&print_mutex);
    pthread_rwlock_unlock(&event_list->rwl);
    free(event->data);
    free(event);
    return 1;
  }

  pthread_rwlock_unlock(&event_list->rwl);
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  if (event_list == NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "EMS state must be initialized\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Error locking list rwl\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  struct Event* event = get_event_with_delay(event_id, event_list->head, event_list->tail);

  pthread_rwlock_unlock(&event_list->rwl);

  if (event == NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Event not found\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  if (pthread_mutex_lock(&event->mutex) != 0) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Error locking mutex\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  for (size_t i = 0; i < num_seats; i++) {
    if (xs[i] <= 0 || xs[i] > event->rows || ys[i] <= 0 || ys[i] > event->cols) {
      pthread_mutex_lock(&print_mutex);
      fprintf(stderr, "Seat out of bounds\n");
      pthread_mutex_unlock(&print_mutex);
      pthread_mutex_unlock(&event->mutex);
      return 1;
    }
  }

  for (size_t i = 0; i < event->rows * event->cols; i++) {
    for (size_t j = 0; j < num_seats; j++) {
      if (seat_index(event, xs[j], ys[j]) != i) {
        continue;
      }

      if (event->data[i] != 0) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Seat already reserved\n");
        pthread_mutex_unlock(&print_mutex);
        pthread_mutex_unlock(&event->mutex);
        return 1;
      }

      break;
    }
  }

  unsigned int reservation_id = ++event->reservations;

  for (size_t i = 0; i < num_seats; i++) {
    event->data[seat_index(event, xs[i], ys[i])] = reservation_id;
  }

  pthread_mutex_unlock(&event->mutex);
  return 0;
}

int ems_show(unsigned int event_id, unsigned int *vector, size_t* rows, size_t* cols) {
  if (event_list == NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "EMS state must be initialized\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Error locking list rwl\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  struct Event* event = get_event_with_delay(event_id, event_list->head, event_list->tail);

  pthread_rwlock_unlock(&event_list->rwl);

  if (event == NULL) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Event not found\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }

  if (pthread_mutex_lock(&event->mutex) != 0) {
    pthread_mutex_lock(&print_mutex);
    fprintf(stderr, "Error locking mutex\n");
    pthread_mutex_unlock(&print_mutex);
    return 1;
  }
  *rows=event->rows;
  *cols=event->cols;

  for (size_t i = 1; i <= *rows; i++) {
    for (size_t j = 1; j <= *cols; j++) {
      // set seat value in calculated Value
      unsigned int calculatedValue = event->data[seat_index(event, i, j)];

      // insert calculatedValue value in vector
      vector[(i - 1) * event->cols + (j - 1)] = calculatedValue;
    }
  }

  pthread_mutex_unlock(&event->mutex);
  return 0;
}


int ems_list_events(unsigned int* vector, size_t* num_events) {
    if (event_list == NULL) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "EMS state must be initialized\n");
        pthread_mutex_unlock(&print_mutex);
        vector[0]='\0';
        *num_events=0;
        return 0;
    }

    if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Error locking list rwl\n");
        pthread_mutex_unlock(&print_mutex);
        *num_events=0;
        return 1;
    }

    struct ListNode* to = event_list->tail;
    struct ListNode* current = event_list->head;

    *num_events = 0;

    //runs every events
    while (current != NULL) {
        //inserts event id in vector
        vector[*num_events] = (current->event)->id;
        (*num_events)++; //increments number of events
        if (*num_events >= MAX_EVENTS) {
            // Handle error: Too many events for the provided vector size
            pthread_rwlock_unlock(&event_list->rwl);
            
            return 0;
        }

        if (current == to) {
            break;
        }

        current = current->next;
    }
    pthread_rwlock_unlock(&event_list->rwl);

    return 0; // Successful operation
}


void print_event_data() {

    if (event_list == NULL) {
        fprintf(stderr, "Error: Event list is NULL\n");
        return;
    }
    //adquire server current data
    struct ListNode* current = event_list->head;

    // iterate every event
    while (current) {
        // Print the identifier of the event
        printf("Event %u:\n", current->event->id);

        //for each event print the seat display
        for (size_t i = 1; i <= current->event->rows; i++) {
            for (size_t j = 1; j <= current->event->cols; j++) {
                unsigned int seat_value = current->event->data[seat_index(current->event, i, j)];
                printf("%u ", seat_value);
                fflush(stdout);
            }
            printf("\n");
            fflush(stdout);
        }

        printf("\n");  // Add a new line between events
        fflush(stdout);

        current = current->next;
    }
}
