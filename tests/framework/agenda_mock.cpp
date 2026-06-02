#include "agenda_mock.h"
#include <string.h>

MockAgendaEntry g_agenda[MOCK_MAX_AGENDA];
int g_agenda_length = 0;
uint32_t g_agenda_next_time = 0;
uint32_t g_hbl_count = 0;

void mock_agenda_init(void) {
  memset(g_agenda, 0, sizeof(g_agenda));
  g_agenda_length = 0;
  g_agenda_next_time = 0;
  g_hbl_count = 0;
}

void mock_agenda_cleanup(void) {
  g_agenda_length = 0;
}

void mock_agenda_add(MockAgendaProc action, int pause, int param) {
  if (g_agenda_length >= MOCK_MAX_AGENDA) return;
  g_agenda[g_agenda_length].perform = action;
  g_agenda[g_agenda_length].time = (uint32_t)pause;
  g_agenda[g_agenda_length].param = param;
  g_agenda_length++;
}

int mock_agenda_get_queue_pos(MockAgendaProc job) {
  for (int i = 0; i < g_agenda_length; i++) {
    if (g_agenda[i].perform == job) return i;
  }
  return -1;
}
