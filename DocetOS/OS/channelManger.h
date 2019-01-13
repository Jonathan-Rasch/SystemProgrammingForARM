#ifndef DOCETOS_CHANNELMANGER_H
#define DOCETOS_CHANNELMANGER_H

#include <stdint.h>
#include "structs.h"
#include "stochasticScheduler.h"
#include "../DataStructures/hashtable.h"
#include "../DataStructures/channel.h"

#define NUM_BUCKETS_FOR_CHANNELS_HASHTABLE 8
#define MAX_ALLOWED_CHANNEL_CAPACITY 16
void initialize_channelManager(uint32_t _maxNumberOfChannels);

extern OS_channelManager_t const channelManager;

#endif //DOCETOS_CHANNELMANGER_H
