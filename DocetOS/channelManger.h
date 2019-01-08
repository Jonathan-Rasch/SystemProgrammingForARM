#ifndef DOCETOS_CHANNELMANGER_H
#define DOCETOS_CHANNELMANGER_H

#include <stdint.h>
#include "structs.h"
#include "stochasticScheduler.h"
#include "hashtable.h"
#include "channel.h"

#define NUM_BUCKETS_FOR_CHANNELS_HASHTABLE 8
void initialize_channelManager(uint32_t _max_number_of_channels);

extern OS_channelManager_t const channelManager;

#endif //DOCETOS_CHANNELMANGER_H
