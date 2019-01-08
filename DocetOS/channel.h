#ifndef DOCETOS_CHANNEL_H
#define DOCETOS_CHANNEL_H

#include "structs.h"
#include "os.h"
#include "queue.h"
#include "semaphore.h"
#include "mutex.h"

OS_channel_t * new_channel(uint32_t _channelID, uint32_t _capacity);
uint32_t destroy_channel(OS_channel_t * _channel);
void channel_write(OS_channel_t * _channel, uint32_t _word);
uint32_t channel_read(OS_channel_t * _channel);


#endif //DOCETOS_CHANNEL_H
