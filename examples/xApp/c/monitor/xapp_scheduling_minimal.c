#include "../../../../src/xApp/e42_xapp_api.h"
#include "../../../../src/util/alg_ds/alg/defer.h"
#include "../../../../src/util/time_now_us.h"
//#include "../../../../src/lib/e2ap/v2_03/e2ap_types/common/e2ap_global_node_id.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

static
uint64_t cnt_mac;

#define MAX_NUM_USERS 64
#define RAN_FUNCTION_MAC 142


void supported_ran_functions(e2_node_connected_xapp_t* e2_node) {
  for(int j=0; j<e2_node->len_rf; ++j) {
    printf("RAN Function ID %d supported.\n", e2_node->rf[j].id);
  }
}

global_e2_node_id_t *global_e2_node_id = NULL;

int main(int argc, char **argv)
{
  fr_args_t fr_args = init_fr_args(argc, argv);
  init_xapp_api(&fr_args);
  sleep(1);

  /*get all the e2 nodes.*/
  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();

  /*only one node for this experiment.*/
  assert(nodes.len == 1);
  e2_node_connected_xapp_t *n = &nodes.n[0];
  global_e2_node_id = &n->id;

  /*print out the RAN functions supported by this node.*/
  supported_ran_functions(n);
  
  user_resource_t* resource_allocation = calloc(1, sizeof(user_resource_t));
  for(int i=0; i<1; ++i) {
    resource_allocation[i].user_id = 11;
    resource_allocation[i].num_rb = 10;
    resource_allocation[i].mcs = 10;
  }

  /* Create the control message */
  sm_ag_if_wr_t wr;
  wr.type = CONTROL_SM_AG_IF_WR;
  wr.ctrl.type = MAC_CTRL_REQ_V0;
  wr.ctrl.mac_ctrl.hdr.dummy = 1;
  wr.ctrl.mac_ctrl.msg.action = 42;
  wr.ctrl.mac_ctrl.msg.num_users = 1;
  wr.ctrl.mac_ctrl.msg.resource_alloc = resource_allocation;

  sm_ans_xapp_t mac_ctrl_handle = control_sm_xapp_api(global_e2_node_id, RAN_FUNCTION_MAC, &wr);

  assert(mac_ctrl_handle.success == true);
  free(resource_allocation);

  /*clean-up for graceful exit*/
  xapp_wait_end_api();
  
  while(try_stop_xapp_api() == false)
    usleep(1000);

  printf("Test xApp run SUCCESSFULLY\n");
}