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

static
void sm_cb_mac(sm_ag_if_rd_t const* rd)
{
  assert(rd != NULL);
  assert(rd->type ==INDICATION_MSG_AGENT_IF_ANS_V0);
  assert(rd->ind.type == MAC_STATS_V0);
 
  int64_t now = time_now_us();
  if(cnt_mac % 100 == 0){
    printf("MAC ind_msg latency = %ld μs\n", now - rd->ind.mac.msg.tstamp);
  
    uint32_t sz_stat_list = rd->ind.mac.msg.len_ue_stats;
    mac_ue_stats_impl_t *stat_list = rd->ind.mac.msg.ue_stats;

    for(int i = 0; i < sz_stat_list; ++i) {
      printf("Msg %d:\n", i);
      print_bsr(stat_list[i].bsr);
      printf("\tPHR:%d\n", stat_list[i].phr);
      printf("\tWB_CQI:%d\n", stat_list[i].wb_cqi);
      printf("\tUL_AGGR_PRB:%d\n",stat_list[i].ul_aggr_prb);
    }
  }

  cnt_mac++;
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
  const char* i_0 = "1_ms";
  sm_ans_xapp_t* mac_handle = NULL;

  if(nodes.len > 0)
    mac_handle = calloc(nodes.len, sizeof(*mac_handle));

  for(int i = 0; i < nodes.len; ++i) {
    e2_node_connected_xapp_t* n = &nodes.n[i];

    for (size_t j = 0; j < n->len_rf; j++)
      printf("Registered node %d ran func id = %d \n ", i, n->rf[j].id);
    
    if(n->id.type == ngran_gNB || n->id.type == ngran_eNB){
      mac_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 142, (void*)i_0, sm_cb_mac);
      assert(mac_handle[i].success == true);
    }
  sleep(10);
  }
}