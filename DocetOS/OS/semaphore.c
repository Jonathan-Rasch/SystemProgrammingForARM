#include "semaphore.h"


//================================================================================
// init, new and destroy
//================================================================================

/*OS_init_semaphore initialises the semaphore to the desired values*/
void init_semaphore(OS_semaphore_t * _semaphore,uint32_t _initial_tokens, uint32_t _max_tokens){
    _semaphore->availableTokens = _initial_tokens;
    _semaphore->maxTokens = _max_tokens;
}

/*new_semaphore allocates and initialises a semaphore.
 *
 * RETURNS: pointer to a semaphore*/
OS_semaphore_t * new_semaphore(uint32_t _initial_tokens, uint32_t _max_tokens){
    OS_semaphore_t * semaphore = OS_alloc(sizeof(OS_semaphore_t)/4);
    init_semaphore(semaphore,_initial_tokens,_max_tokens);
    return semaphore;
}

/* deallocate the resources associated with the semaphore*/
uint32_t destroy_semaphore(OS_semaphore_t * _semaphore){
    OS_free((uint32_t*)_semaphore);
    return 1;
}

//================================================================================
// Exported Functions
//================================================================================

/*  OS_semaphore_acquire_token removes a token from the semaphore
 *  -> blocks until another task releases a token if no token is available.
 *  -> if successful it notifies all tasks waiting on the semaphore.
 * */
void semaphore_acquire_token(OS_semaphore_t * _semaphore){
    uint32_t exclusiveAcessFailed;
    while(1){
        uint32_t checkCode = OS_checkCode();
        uint32_t tokens = (uint32_t)__LDREXW((uint32_t*) &(_semaphore->availableTokens));
        if(tokens > 0){
            tokens--;
            exclusiveAcessFailed = __STREXW(tokens,(uint32_t*) &(_semaphore->availableTokens)); // returns 0 on success !
            if(exclusiveAcessFailed){
                continue;// try again, go back to loading
            }else{
                break; //wrote the value
            }
        }else{
            //no tokens remaining, wait for one to be released
            OS_wait(_semaphore,checkCode);
            continue;
        }
    }
    OS_notify(_semaphore);
}

/* OS_semaphore_release_token Places a token back into the semaphore.
 * -> blocks until another task removes a token if the semaphore already holds the maximum number of tokens
 * -> notifies all tasks waiting on this semaphore after successfully placing token
 * */
void semaphore_release_token(OS_semaphore_t * _semaphore){
    uint32_t exclusiveAcessFailed;
		while(1){
        uint32_t checkCode = OS_checkCode();
        uint32_t token_counter = (uint32_t)__LDREXW((uint32_t*) &(_semaphore->availableTokens));
        if(token_counter < _semaphore->maxTokens){
            token_counter++;
            exclusiveAcessFailed = __STREXW(token_counter,(uint32_t*) &(_semaphore->availableTokens)); // returns 0 on success !
            if(exclusiveAcessFailed){
                continue;// try again, go back to loading
            }else{
                break; //wrote the value
            }
        }else{
            //semaphore holds maximum number of tokens, wait for one to be taken
            OS_wait(_semaphore,checkCode);
            continue;
        }
    }
    OS_notify(_semaphore);
}


