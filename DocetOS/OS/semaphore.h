//
// Created by user on 07/01/19.
//

#ifndef DOCETOS_SEMAPHORE_H
#define DOCETOS_SEMAPHORE_H

#include "../structs.h"
#include <stdio.h>
#include "os_internal.h"
#include "os.h"

void semaphore_acquire_token(OS_semaphore_t * _semaphore);
void semaphore_release_token(OS_semaphore_t * _semaphore);
void init_semaphore(OS_semaphore_t * _semaphore,uint32_t _initial_tokens, uint32_t _max_tokens);
OS_semaphore_t * new_semaphore(uint32_t _initial_tokens, uint32_t _max_tokens);
uint32_t destroy_semaphore(OS_semaphore_t * _semaphore);

#endif //DOCETOS_SEMAPHORE_H
