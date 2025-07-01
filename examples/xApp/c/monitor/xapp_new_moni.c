#include "../../../../src/xApp/e42_xapp_api.h"
#include "../../../../src/util/alg_ds/alg/defer.h"
#include "../../../../src/util/time_now_us.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

static
uint64_t cnt_mac;

void print_bsr(uint32_t bsr) {
  printf("\tBSR:");
  for(int i = 0; i < 32; ++i) {
    if((i % 8) == 0) printf(" ");
    printf( (bsr & (1 << (31-i)) ) ? "1" : "0" );
  }
  printf(" (= %u)\n", bsr);
}

#define MAX_NUM_USERS 64

static int rnti[MAX_NUM_USERS];
static int num_users;

e2_node_arr_xapp_t g_nodes;

static
void sm_cb_mac(sm_ag_if_rd_t *rd)
{ 
  assert(rd != NULL);
  assert(rd->type ==INDICATION_MSG_AGENT_IF_ANS_V0);
  assert(rd->ind.type == MAC_STATS_V0);
 
  //int64_t now = time_now_us();
  //printf("MAC ind_msg latency = %ld μs\n", now - rd->ind.mac.msg.tstamp);

  uint32_t sz_stat_list = rd->ind.mac.msg.len_ue_stats;
  mac_ue_stats_impl_t *stat_list = rd->ind.mac.msg.ue_stats;
  num_users = sz_stat_list;
  for(int i = 0; i < sz_stat_list; ++i) {
  //  printf("Msg %d:\n", i);
  //  print_bsr(stat_list[i].bsr);
  //  printf("\tRNTI:%d\n", stat_list[i].rnti);
  //  printf("\tPHR:%d\n", stat_list[i].phr);
  //  printf("\tWB_CQI:%d\n", stat_list[i].wb_cqi);
  //  printf("\tUL_AGGR_PRB:%d\n",stat_list[i].ul_aggr_prb);
  //  printf("\tFRAME: %d\n", stat_list->frame);
  //  printf("\tSLOT: %d\n", stat_list->slot);
    rnti[i] = stat_list[i].rnti;
  }

  cnt_mac++;
}

void fill_mac_ctrl_msg(sm_ag_if_wr_t *wr) {
  wr->type = CONTROL_SM_AG_IF_WR;
  wr->ctrl.mac_ctrl.hdr.dummy = 1;
  wr->ctrl.mac_ctrl.msg.action = 42;

  wr->ctrl.mac_ctrl.msg.num_users = num_users;
  wr->ctrl.mac_ctrl.msg.resource_alloc = calloc(num_users, sizeof(user_resource_t));
  for(int i=0; i<num_users; ++i) {
    wr->ctrl.mac_ctrl.msg.resource_alloc[i].mcs = 10;
    wr->ctrl.mac_ctrl.msg.resource_alloc[i].num_rb = 10;
    wr->ctrl.mac_ctrl.msg.resource_alloc[i].user_id = rnti[i];
  }
}

int main(int argc, char **argv)
{
  fr_args_t args = init_fr_args(argc, argv);
  //Init the xApp
  init_xapp_api(&args);
  sleep(1);
  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
  defer({ free_e2_node_arr_xapp(&nodes); });
  assert(nodes.len > 0);
  // MAC indication
  const char* i_0 = "5_ms";
  sm_ans_xapp_t* mac_handle_report = NULL;
  // MAC control
  sm_ans_xapp_t* mac_handle_control = NULL;
  if(nodes.len > 0) {
    mac_handle_report = calloc(nodes.len, sizeof(*mac_handle_report));
    mac_handle_control = calloc(nodes.len, sizeof(*mac_handle_control));
  }
  for(int i = 0; i < nodes.len; ++i) {
    e2_node_connected_xapp_t* n = &nodes.n[i];

    for (size_t j = 0; j < n->len_rf; j++)
      printf("Registered node %d ran func id = %d \n ", i, n->rf[j].id);
    
    if(n->id.type == ngran_gNB || n->id.type == ngran_eNB){
      mac_handle_report[i] = report_sm_xapp_api(&nodes.n[i].id, 142, (void*)i_0, sm_cb_mac);
      assert(mac_handle_report[i].success == true);

      mac_ctrl_req_data_t mac_data; //= {.hdr.dummy=1, .msg.action=42, .msg.num_users=1, .msg.resource_alloc=NULL};
      
      usleep(500);
      while(num_users==0);
      printf("Found %d users.\n", num_users);
      sm_ag_if_wr_t wr;
      fill_mac_ctrl_msg(&wr);
      mac_handle_control[i] = control_sm_xapp_api(&nodes.n[i].id, 142, &wr);
      assert(mac_handle_control[i].success == true);
    }
  }
  xapp_wait_end_api();

  for(int i = 0; i < nodes.len; ++i){
    // Remove the handle previously returned
    if(mac_handle_report[i].u.handle != 0 )
      rm_report_sm_xapp_api(mac_handle_report[i].u.handle);
  }

  if(nodes.len > 0){
    free(mac_handle_report);
  }

  //Stop the xApp
  while(try_stop_xapp_api() == false)
    usleep(1000);

  printf("Test xApp run SUCCESSFULLY\n");
}