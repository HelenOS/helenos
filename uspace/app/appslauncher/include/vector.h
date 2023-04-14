/*
 * Copyright (c) 2023 SimonJRiddix
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/******************************************************************************

 Vector in c

*******************************************************************************/

#ifndef SJR_VECTOR_H
#define SJR_VECTOR_H

#include <stdio.h>
#include <stdlib.h>
#define VECTOR_INIT_CAPACITY 2
#define UNDEFINE  -1
#define SUCCESS 0
#define VECTOR_INIT(vec) vector vec;\
 vector_init(&vec)

//Store and track the stored data
typedef struct sVectorList
{
    void** items;
    int capacity;
    int total;
} sVectorList;

//structure contain the function pointer
typedef struct sVector vector;
struct sVector
{
    sVectorList vectorList;
    //function pointers
    int (*Count)(vector*);
    int (*Resize)(vector*, int);
    int (*Add)(vector*, void*);
    int (*Set)(vector*, int, void*);
    void* (*Get)(vector*, int);
    int (*Delete)(vector*, int);
    int (*Clear)(vector*);
};

int vector_total(vector* v);
int vector_resize(vector* v, int capacity);
int vector_push_back(vector* v, void* item);
int vector_set(vector* v, int index, void* item);
void* vector_get(vector* v, int index);
int vector_delete(vector* v, int index);
int vector_clear(vector* v);
void vector_init(vector* v);

#endif
