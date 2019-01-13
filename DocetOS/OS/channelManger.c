#include "channelManger.h"

//=============================================================================
// vars
//=============================================================================

/* channelHashTable holds references to all channel structs that were created due to a request
 * from a task.
 *
 * Key: key given by task; Value: pointer to channel struct*/
static OS_hashtable_t * channelHashTable;

/* channelStatusHashTable keeps track of how many tasks have connected to this channel (how often connect was called with a specific ID).
 * this is used to decide when to deallocate a channel*/
static OS_hashtable_t * channelStatusHashTable;

static OS_channel_t * freeChannelsLinkedList = NULL;

//=============================================================================
// prototypes
//=============================================================================

//svc accessible
static OS_channel_t * channelManager_connect(uint32_t _channelID,uint32_t _capacity);
static uint32_t channelManager_disconnect(uint32_t _channelID);
static uint32_t channelManager_checkAlive(uint32_t _channelID);

//=============================================================================
// init
//=============================================================================

OS_channelManager_t const channelManager = {
        .connect_callback = channelManager_connect,
        .disconnect_callback = channelManager_disconnect,
        .isAlive_callback = channelManager_checkAlive
};

void initialize_channelManager(uint32_t _max_number_of_channels){
    channelHashTable = new_hashtable(_max_number_of_channels,NUM_BUCKETS_FOR_CHANNELS_HASHTABLE);
    channelStatusHashTable = new_hashtable(_max_number_of_channels,NUM_BUCKETS_FOR_CHANNELS_HASHTABLE);
		/*allocating channels*/
		for(int i=0;i < _max_number_of_channels;i++){
			OS_channel_t * channel = new_channel(1,MAX_ALLOWED_CHANNEL_CAPACITY);
			uint32_t * tmp_nextFreeChannel = (uint32_t*)freeChannelsLinkedList;
			freeChannelsLinkedList = channel;
			channel->channelID = (uint32_t)tmp_nextFreeChannel;/*overwriting channelID, but that is irrelevant whilst channel is not in use*/
		}
};

//=============================================================================
// callback functions
//=============================================================================

/* channelManager_link is used to enable two or more tasks to communicate by giving them access
 * to a common "channel" to which they can write to and read from. If two tasks want to get access to the
 * same channel they should use the same _channelID when calling the connect method.
 *
 * NOTE: _channelID cannot be NULL (0);
 * NOTE_2:  if the maximum number of channels is already in use the connect() function will block
 *          until another channel is released (all parties call disconnect() );
 * NOTE_3: it is the users responsibility to only call connect() once per channelID or to call disconnect
 *         an equal number of times. If this is not done the channel will not get removed even if the task exits.
 *         On the same note the user has to take care not to call disconnect() more often than they called connect
 *
 * RETURNS: OS_channel_t * if successful , if not it returns NULL (e.g if the maximum number of allowed channels has been reached or
 *          if there is insufficient memory to allocate a new channel etc)*/
static OS_channel_t * channelManager_connect(uint32_t _channelID,uint32_t _capacity){
    if(_channelID == NULL){
        printf("\r\nCHANNEL_MANAGER: ERROR 0 is not a valid channelID !\r\n");
        return NULL;
    }
		if(_capacity > MAX_ALLOWED_CHANNEL_CAPACITY){
			printf("\r\nCHANNEL_MANAGER: ERROR requested capacity (%d) exceeds the MAX_ALLOWED_CHANNEL_CAPACITY %d!\r\n",_capacity,MAX_ALLOWED_CHANNEL_CAPACITY);
        return NULL;
		}
    /* check if a channel for the given ID already exists (another task might have called connect before the current task)*/
    {
        OS_channel_t * channel = (OS_channel_t *)OS_hashtable_get(channelHashTable,_channelID);
        if(channel){
            if(channel->capacity != _capacity){
                printf("\r\nCHANNEL_MANAGER: WARNING a channel with the ID %d already exists, but its capacity (%d) differs from the requested capacity (%d).\r\n",_channelID,channel->capacity,_capacity);
            }
            /*yes a channel exists, return it after updating the channel status*/
            uint32_t num_connections = (uint32_t)OS_hashtable_remove(channelStatusHashTable,_channelID);
            num_connections += 1;
            OS_hashtable_put(channelStatusHashTable,_channelID,(uint32_t*)num_connections,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
            return channel;
        }
    }
    /*channel does not exist yet, take a free one from the linked list*/
    OS_channel_t * newChannel = freeChannelsLinkedList;//= new_channel(_channelID, _capacity);
    freeChannelsLinkedList = (OS_channel_t *)newChannel->channelID;//channelID holds pointer to next channel struct whilst the channel is not in use;
		if(newChannel == NULL){
        printf("\r\nCHANNEL_MANAGER: ERROR no free channels available!\r\n");
        return NULL; //could not allocate the memory
    }
		channel_init(newChannel,_channelID,_capacity);//reset the channel (clear values set by previous usage of this channel struct);
    /*at this point we have a valid channel, now we add it to the hashtable*/
    OS_hashtable_put(channelHashTable,_channelID,(uint32_t *)newChannel,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
    OS_hashtable_put(channelStatusHashTable,_channelID,(uint32_t*)1,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);//one task connected to channel
    return newChannel;
}

/* channelManager_checkAlive is used to check if a channel with the given ID already is known to the channel manager. This is useful
 * to avoid accidentally using a channel used by other tasks and interfering with their communications.
 *
 * RETURNS: number of connections to channel if ID is in use, 0 otherwise*/
static uint32_t channelManager_checkAlive(uint32_t _channelID){
    uint32_t numConnections = (uint32_t)OS_hashtable_get(channelStatusHashTable,_channelID);
    if(numConnections){
        return numConnections;
    }else{
        return 0;
    }
}

/*channelManager_disconnect tells the channel manager that the current task will no longer use the channel. if the total users of
 * the channel drops to 0 the channel manager will deallocate the resources associated with the channel.
 *
 * RETURNS: 1 if successful, 0 if no channel with the given ID existed.*/
static uint32_t channelManager_disconnect(uint32_t _channelID){
    if(!channelManager_checkAlive(_channelID)){
        return 0;
    }
    uint32_t num_connections = (uint32_t)OS_hashtable_remove(channelStatusHashTable,_channelID);
    num_connections -= 1;
    if(num_connections){
        OS_hashtable_put(channelStatusHashTable,_channelID,(uint32_t *)num_connections,HASHTABLE_REJECT_MULTIPLE_VALUES_PER_KEY);
        return 1;
    }else{
        /*no more connections, put channel back into linked list of free channels*/
        OS_channel_t * channel = (OS_channel_t *)OS_hashtable_remove(channelHashTable,_channelID);
        OS_channel_t * tmp_channel = freeChannelsLinkedList;
				freeChannelsLinkedList = channel;
				channel->channelID = (uint32_t)tmp_channel; 
    }
    return 1;
}
