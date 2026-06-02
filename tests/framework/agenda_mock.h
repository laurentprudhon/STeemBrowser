#pragma once
#ifndef TEST_AGENDA_MOCK_H
#define TEST_AGENDA_MOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOCK_MAX_AGENDA 32

typedef void (*MockAgendaProc)(int);

typedef struct {
  MockAgendaProc perform;
  uint32_t time;
  int param;
} MockAgendaEntry;

extern MockAgendaEntry g_agenda[MOCK_MAX_AGENDA];
extern int g_agenda_length;
extern uint32_t g_agenda_next_time;
extern uint32_t g_hbl_count;

void mock_agenda_init(void);
void mock_agenda_cleanup(void);
void mock_agenda_add(MockAgendaProc action, int pause, int param);
int  mock_agenda_get_queue_pos(MockAgendaProc job);

#ifdef __cplusplus
}
#endif

#endif // TEST_AGENDA_MOCK_H
