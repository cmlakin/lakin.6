#pragma once

#include "config.h"
#include "shm.h"
#include "oss.h"
#include "osclock.h"
#include "queue.h"

static int id;

void uprocInitialize();
void attachSharedMemory();

void loop();
