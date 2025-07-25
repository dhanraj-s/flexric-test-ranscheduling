/*
 Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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

// File modified by https://github.com/dhanraj-s

#include <stdio.h>

#include "mac_sm_ric.h"
#include "mac_sm_id.h"

#include <assert.h>
#include <stdlib.h>

#include "enc/mac_enc_generic.h"
#include "dec/mac_dec_generic.h"

typedef struct{
  sm_ric_t base;

#ifdef ASN
  mac_enc_asn_t enc;
#elif FLATBUFFERS 
  mac_enc_fb_t enc;
#elif PLAIN
  mac_enc_plain_t enc;
#else
  static_assert(false, "No encryption type selected");
#endif
} sm_mac_ric_t;


static
sm_subs_data_t on_subscription_mac_sm_ric(sm_ric_t const* sm_ric, void* cmd)
{
  assert(sm_ric != NULL); 
  assert(cmd != NULL); 

  sm_mac_ric_t* sm = (sm_mac_ric_t*)sm_ric;  
 
  mac_sub_data_t mac = {0}; 

  const int max_str_sz = 10;
  if(strncmp(cmd, "1_ms", max_str_sz) == 0 ){
    mac.et.ms = 1;
  } else if (strncmp(cmd, "2_ms", max_str_sz) == 0 ) {
    mac.et.ms = 2;
  } else if (strncmp(cmd, "5_ms", max_str_sz) == 0 ) {
    mac.et.ms = 5;
  } else if (strncmp(cmd, "10_ms", max_str_sz) == 0 ) {
    mac.et.ms = 10;
  } else {
    assert(0 != 0 && "Invalid input");
  }
  
  // Event trigger
  const byte_array_t ba = mac_enc_event_trigger(&sm->enc, &mac.et); 

  sm_subs_data_t data = {0}; 
  
  // Event trigger IE
  data.event_trigger = ba.buf;
  data.len_et = ba.len;

  // Action Definition IE
  data.action_def = NULL;
  data.len_ad = 0;

  return data;
}

static
sm_ag_if_rd_ind_t on_indication_mac_sm_ric(sm_ric_t const* sm_ric, sm_ind_data_t const* data)
{
  assert(sm_ric != NULL); 
  assert(data != NULL); 
  sm_mac_ric_t* sm = (sm_mac_ric_t*)sm_ric;  

  sm_ag_if_rd_ind_t rd_if = {.type =  MAC_STATS_V0};

  // Header
  rd_if.mac.hdr = mac_dec_ind_hdr(&sm->enc, data->len_hdr, data->ind_hdr);

  // Message
  rd_if.mac.msg = mac_dec_ind_msg(&sm->enc, data->len_msg, data->ind_msg);

  //  call_process_id
  assert(data->call_process_id == NULL && "not implemented"); 
  rd_if.mac.proc_id = NULL;

  return rd_if;
}

static
sm_ctrl_req_data_t ric_on_control_req_mac_sm_ric(sm_ric_t const* sm_ric, void* ctrl)
{
  assert(sm_ric != NULL); 
  assert(ctrl != NULL); 

  // printf("ric_on_control_req_mac_sm_ric:\n");
  // printf("\theader dummy: %d\n", ((sm_ag_if_wr_t*)ctrl)->ctrl.mac_ctrl.hdr.dummy);
  // printf("\ttype: %d\n", ((sm_ag_if_wr_t*)ctrl)->type);
  // printf("\tctrl type: %d\n", ((sm_ag_if_wr_t*)ctrl)->ctrl.type);
  // printf("\tmsg action: %d\n", ((sm_ag_if_wr_t*)ctrl)->ctrl.mac_ctrl.msg.action);



  // BUG: Simple casting will not work. Need to manually populate
  // mac_ctrl_req_data_t
  // mac_ctrl_req_data_t const* req = (mac_ctrl_req_data_t const*)ctrl;
  
  // Manually filling in:
  sm_ag_if_wr_t* src = (sm_ag_if_wr_t*) ctrl;

  mac_ctrl_req_data_t* req = calloc(1, sizeof(mac_ctrl_req_data_t));
  req->hdr.dummy = src->ctrl.mac_ctrl.hdr.dummy;
  req->msg = cp_mac_ctrl_msg(&src->ctrl.mac_ctrl.msg);


  assert(req->hdr.dummy == 1);
  assert(req->msg.action == 42);

  sm_mac_ric_t* sm = (sm_mac_ric_t*)sm_ric;  

  byte_array_t ba = mac_enc_ctrl_hdr(&sm->enc, &req->hdr);
  
  sm_ctrl_req_data_t ret_data = {0};  
  ret_data.ctrl_hdr = ba.buf;
  ret_data.len_hdr = ba.len;

  ba = mac_enc_ctrl_msg(&sm->enc, &req->msg);
  ret_data.ctrl_msg = ba.buf;
  ret_data.len_msg = ba.len;

  return ret_data;
}

static
sm_ag_if_ans_ctrl_t ric_on_control_out_mac_sm_ric(sm_ric_t const* sm_ric,const sm_ctrl_out_data_t * out)
{
  assert(sm_ric != NULL); 
  assert(out != NULL);

  sm_mac_ric_t* sm = (sm_mac_ric_t*)sm_ric;  

  sm_ag_if_ans_ctrl_t ag_if = {.type =  MAC_AGENT_IF_CTRL_ANS_V0};  
  ag_if.mac = mac_dec_ctrl_out(&sm->enc, out->len_out, out->ctrl_out);
  assert(ag_if.mac.ans ==  MAC_CTRL_OUT_OK);

  return ag_if;
}

static
sm_ag_if_rd_e2setup_t ric_on_e2_setup_mac_sm_ric(sm_ric_t const* sm_ric, sm_e2_setup_data_t const* setup)
{
  assert(sm_ric != NULL); 
  assert(setup != NULL); 

  sm_ag_if_rd_e2setup_t dst = {.type = MAC_AGENT_IF_E2_SETUP_ANS_V0 }; 
  
  dst.mac.func_def.len = setup->len_rfd;
  if(dst.mac.func_def.len > 0){
    dst.mac.func_def.buf = calloc(dst.mac.func_def.len, sizeof(uint8_t));
    assert(dst.mac.func_def.buf != NULL && "Memory exhausted");
    memcpy(dst.mac.func_def.buf, setup->ran_fun_def, setup->len_rfd);
  }

  return dst;
}


static
sm_ag_if_rd_rsu_t on_ric_service_update_mac_sm_ric(sm_ric_t const* sm_ric, sm_ric_service_update_data_t const* rsu)
{
  assert(sm_ric != NULL); 
  assert(  rsu != NULL); 
  //  sm_mac_ric_t* sm = (sm_mac_ric_t*)sm_ric;  

  assert(0!=0 && "Not implemented");
  sm_ag_if_rd_rsu_t dst = {0};
  return dst;
}

static
void free_mac_sm_ric(sm_ric_t* sm_ric)
{
  assert(sm_ric != NULL);
  sm_mac_ric_t* sm = (sm_mac_ric_t*)sm_ric;
  free(sm);
}


//
// Allocation SM functions. The memory malloc by the SM is also freed by it.
//

static
void free_subs_data_mac_sm_ric(void* msg)
{
  assert(msg != NULL);
  assert(0!=0 && "Not implemented");
}

static
void free_ind_data_mac_sm_ric(void* msg)
{
  assert(msg != NULL);

  sm_ag_if_rd_ind_t* rd_ind  = (sm_ag_if_rd_ind_t*)msg;
  assert(rd_ind->type == MAC_STATS_V0);

  mac_ind_data_t* ind = &rd_ind->mac;

  free_mac_ind_hdr(&ind->hdr); 
  free_mac_ind_msg(&ind->msg); 
  assert(ind->proc_id == NULL && "Not implemented");
  if(ind->proc_id != NULL){
    free_mac_call_proc_id(ind->proc_id);
  }
}

static
void free_ctrl_req_data_mac_sm_ric(void* msg)
{
  assert(msg != NULL);
  assert(0!=0 && "Not implemented");
}


static
void free_ctrl_out_data_mac_sm_ric(void* msg)
{
  assert(msg != NULL);
  assert(0!=0 && "Not implemented");
}

static
void free_e2_setup_mac_sm_ric(void* msg)
{
  assert(msg != NULL);
  assert(0!=0 && "Not implemented");
}

static
void free_ric_service_update_mac_sm_ric(void* msg)
{
  assert(msg != NULL);
  assert(0!=0 && "Not implemented");
}


sm_ric_t* make_mac_sm_ric(void /* sm_io_ric_t io */)
{
  sm_mac_ric_t* sm = calloc(1, sizeof(sm_mac_ric_t));
  assert(sm != NULL && "Memory exhausted");

  *((uint16_t*)&sm->base.ran_func_id) = SM_MAC_ID; 

  sm->base.free_sm = free_mac_sm_ric;

  // Memory (De)Allocation
  sm->base.alloc.free_subs_data_msg = free_subs_data_mac_sm_ric; 
  sm->base.alloc.free_ind_data = free_ind_data_mac_sm_ric ; 
  sm->base.alloc.free_ctrl_req_data = free_ctrl_req_data_mac_sm_ric; 
  sm->base.alloc.free_ctrl_out_data = free_ctrl_out_data_mac_sm_ric; 
  sm->base.alloc.free_e2_setup = free_e2_setup_mac_sm_ric; 
  sm->base.alloc.free_ric_service_update = free_ric_service_update_mac_sm_ric; 


  // O-RAN E2SM 5 Procedures
  sm->base.proc.on_subscription = on_subscription_mac_sm_ric; 
  sm->base.proc.on_indication = on_indication_mac_sm_ric;

  // Control needs two calls
  sm->base.proc.on_control_req = ric_on_control_req_mac_sm_ric;
  sm->base.proc.on_control_out = ric_on_control_out_mac_sm_ric;

  sm->base.proc.on_e2_setup = ric_on_e2_setup_mac_sm_ric;
  sm->base.proc.on_ric_service_update = on_ric_service_update_mac_sm_ric; 
  sm->base.handle = NULL;

  assert(strlen(SM_MAC_STR) < sizeof(sm->base.ran_func_name));
  memcpy(sm->base.ran_func_name, SM_MAC_STR, strlen(SM_MAC_STR));

  return &sm->base;
}


uint16_t id_sm_mac_ric(sm_ric_t const* sm_ric)
{
  assert(sm_ric != NULL);
  sm_mac_ric_t* sm = (sm_mac_ric_t*)sm_ric;
  return sm->base.ran_func_id;
}

