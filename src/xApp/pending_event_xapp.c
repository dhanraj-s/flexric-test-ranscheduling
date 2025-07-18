
/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdio.h>
#include <time.h>
#include "pending_event_xapp_sem.h"

#include <assert.h>
#include <stdbool.h>
#include "pending_event_xapp.h"

#include "../util/compare.h"
#include "../util/alg_ds/ds/lock_guard/lock_guard.h"
#include "../util/alg_ds/alg/alg.h"

extern struct timespec control_req_time;
struct timespec add_end_time, rm_start_time;
sem_t non_empty;

bool eq_pending_event_xapp(pending_event_xapp_t* m0, pending_event_xapp_t* m1)
{
  assert(m0 != NULL);
  assert(m1 != NULL);

  if(m0->ev != m1->ev)
    return false;


  if(eq_ric_gen_id(&m0->id, &m1->id) == false)
    return false;

  return true;
}


static inline
void free_fd(void* key, void* value)
{
  assert(key != NULL);
  assert(value != NULL);
  int* fd = (int*) key;
  assert(*fd > 0 );
  free(value);
}

static inline
void free_pending_ev(void* key, void* value)
{
  assert(key != NULL);
  assert(value != NULL);
  pending_event_xapp_t* ev = (pending_event_xapp_t*)key;
  assert(valid_pending_event(ev->ev));
  free(value);
}

void init_pending_events(pending_event_xapp_ds_t* p)
{
  assert(p != NULL);

  size_t fd_sz = sizeof(int);
  size_t event_sz = sizeof( pending_event_xapp_t );
  sem_init(&non_empty, 0, 0);
  bi_map_init(&p->pending , fd_sz, event_sz, cmp_fd, cmp_pending_event, free_fd, free_pending_ev );

  pthread_mutexattr_t *mtx_attr = NULL;
#ifdef DEBUG
  *mtx_attr = PTHREAD_MUTEX_ERRORCHECK; 
#endif

  int rc = pthread_mutex_init(&p->pend_mtx, mtx_attr);
  assert(rc == 0);
}

void free_pending_events(pending_event_xapp_ds_t* p)
{
  assert(p != NULL);

  bi_map_free(&p->pending);
  sem_destroy(&non_empty);
  int const rc = pthread_mutex_destroy(&p->pend_mtx);
  assert(rc == 0);
}

void add_pending_event(pending_event_xapp_ds_t* p, int fd ,pending_event_xapp_t* ev)
{
  assert(p != NULL);
  assert(ev != NULL);
  assert(fd > 0);

  //printf("adding event fd = %d ev-> %d \n", fd, ev->ev );
  //printf("add_pending_event (pending event type=%d, fd=%d)\n",ev->ev, fd);
  lock_guard(&p->pend_mtx);
  bi_map_insert(&p->pending, &fd, sizeof(fd), ev, sizeof(*ev));
  //clock_gettime(CLOCK_REALTIME, &add_end_time);
  //printf("add_time: %lf us", (add_end_time.tv_sec-control_req_time.tv_sec)*1e6+(add_end_time.tv_nsec-control_req_time.tv_nsec)/1000);
}

static inline
bool eq_int(const void* m0_v, const void* m1_v)
{
  assert(m0_v != NULL);
  assert(m1_v != NULL);

  return *(int*)m0_v == *(int*)m1_v;
}

bool find_pending_event_fd(pending_event_xapp_ds_t* p, int fd)
{
  assert(p != NULL);
  assert(fd > 0);

  lock_guard(&p->pend_mtx);

  assoc_rb_tree_t* map = &p->pending.left;

  void* it = assoc_front(map);
  void* end = assoc_end(map);
  it = find_if(map, it, end, &fd, eq_int);
  return it != end;


/*
  bml_iter_t it = bi_map_front_left(&p->pending); 
  printf("Start it value = %d \n", *(int*)assoc_value(&p->pending.left, it.it )  );
  bml_iter_t end = bi_map_end_left(&p->pending); 

  find_if( );

  it = find_if_bi_map_left(&p->pending, it, end, &fd, eq_int);
  return it.it != end.it;
  */
}

static inline
bool eq_ev(const void* m0_v, const void* m1_v)
{
  assert(m0_v != NULL);
  assert(m1_v != NULL);

  pending_event_xapp_t* m0 = (pending_event_xapp_t*)m0_v;
  pending_event_xapp_t* m1 = (pending_event_xapp_t*)m1_v;

  return eq_pending_event_xapp(m0, m1);
}

bool find_pending_event_ev(pending_event_xapp_ds_t* p, pending_event_xapp_t* ev)
{
  assert(p != NULL);
  assert(ev != NULL);

  lock_guard(&p->pend_mtx);

  bmr_iter_t it = bi_map_front_right(&p->pending); 
  bmr_iter_t end = bi_map_end_right(&p->pending); 
  it = find_if_bi_map_right(&p->pending, it, end, ev, eq_ev);
  return it.it != end.it;
}


void print_pending_ev(pending_event_xapp_t const *ev) {
  switch(ev->ev) {
    case SETUP_REQUEST_PENDING_EVENT:
    printf("SETUP_REQUEST_PENDING_EVENT\n");
    break;
    case SUBSCRIPTION_REQUEST_PENDING_EVENT:
    printf("SUBSCRIPTION_REQUEST_PENDING_EVENT\n");
    break;
    case SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT:
    printf("SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT\n");
    break;
    case CONTROL_REQUEST_PENDING_EVENT:
    printf("CONTROL_REQUEST_PENDING_EVENT\n");
    break;
    case E42_SETUP_REQUEST_PENDING_EVENT:
    printf("E42_SETUP_REQUEST_PENDING_EVENT\n");
    break;
    case E42_RIC_SUBSCRIPTION_REQUEST_PENDING_EVENT:
    printf("E42_RIC_SUBSCRIPTION_REQUEST_PENDING_EVENT\n");
    break;
    case RIC_SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT:
    printf("RIC_SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT\n");
    break;
    case E42_RIC_SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT:
    printf("E42_RIC_SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT\n");
    break;
    case E42_RIC_CONTROL_REQUEST_PENDING_EVENT:
    printf("E42_RIC_CONTROL_REQUEST_PENDING_EVENT\n");
    break;
  }
}

int* rm_pending_event_ev(pending_event_xapp_ds_t* p, pending_event_xapp_t* ev )
{
  assert(p != NULL);
  assert(ev != NULL);

  int* fd = NULL; 
  {
    lock_guard(&p->pend_mtx);
    size_t sz = bi_map_size(&p->pending); 
    //printf("Pending event size before remove = %ld \n", sz);

    // It returns the void* of key1. the void* of the key2 is freed
    void (*free_pending_event_xapp)(void*) = NULL;
    //clock_gettime(CLOCK_REALTIME, &rm_start_time);
    //printf("rm_pending_event_ev: removal start %lf us after sending CONTROL request.\n", 
      //(rm_start_time.tv_sec-control_req_time.tv_sec)*1e6+(rm_start_time.tv_nsec-control_req_time.tv_nsec)/1000);
    //printf("rm_pending_event_ev (pending_event_type=%d): bi_map_extract_right called below.\n", ev->ev);
    fd = bi_map_extract_right(&p->pending, ev , sizeof(*ev), free_pending_event_xapp);
    //printf("rm_pending_event_ev: Removed (pending_event_type=%d, fd=%p)\n", ev->ev, fd);
    assert(sz == bi_map_size(&p->pending) + 1 );
  }
  assert(fd != NULL && *fd > 0);
  return fd;
}

void rm_pending_event_fd(pending_event_xapp_ds_t* p, int fd)
{
  assert(p != NULL);
  assert(fd > 0);

  pending_event_xapp_t* ev = NULL; 
  {
    lock_guard(&p->pend_mtx);
    // It returns the void* of key2. the void* of the key1 is freed
    
    void (*free_fd)(void*) = NULL; 
    ev = bi_map_extract_left(&p->pending, &fd, sizeof(int), free_fd);
  }
  assert(ev != NULL);
  free(ev);
}


