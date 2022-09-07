/*
 * common.h
 *
 *  Created on: Sep 5, 2022
 *      Author: pc
 */

#ifndef MAIN_COMMON_H_
#define MAIN_COMMON_H_

typedef struct
{
  int hour;
  int minute;
  int second;
  char time[128];
  int day;
  int month;
  int year;
  char date[128];
}DT;

extern DT DateTime;
extern uint8_t Datetime[20];

void Conver_DateTime(char *datetime, char kind);
time_t string_to_seconds(const char *timestamp_str);

#endif /* MAIN_COMMON_H_ */
