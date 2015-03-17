#ifndef SYSMAN_LOG_H
#define SYSMAN_LOG_H

#include <io/log.h>
#include <stdio.h>

/*
 * Temporarily use only simple printfs, later add some smart logging,
 * that would use logger as soon as it's ready.
 */
#define sysman_log(level, fmt, ...) printf("sysman: " fmt "\n", __VA_ARGS__)

#endif
