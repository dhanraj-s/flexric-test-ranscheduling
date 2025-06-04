#include "../../../../src/xApp/e42_xapp_api.h"
#include <stdio.h>
#include <unistd.h>

const int RC_RAN_FUNCTION = 3;

static
size_t find_sm_idx(sm_ran_function_t* rf, size_t sz, bool (*f)(sm_ran_function_t const*, int const), int const id)
{
  for (size_t i = 0; i < sz; i++) {
    if (f(&rf[i], id))
      return i;
  }

  assert(0 != 0 && "SM ID could not be found in the RAN Function List");
}

static
bool eq_sm(sm_ran_function_t const *rf, int const val) {
    return (rf->id == val);
}

static
rc_sub_data_t gen_rc_subs(e2sm_rc_func_def_t const* ran_func) {
    assert(ran_func != NULL);
    assert(ran_func->ev_trig != NULL);

    rc_sub_data_t rc_sub = {0};

    // Generate event trigger.

    // Didn't check: Walk thru event trigger style seq and look for fmt 4.
    // Didn't check: Walk thru sequence of RAN parameters for L2 vars and look for BSR. 
    int report_lst_sz = ran_func->report->sz_seq_report_sty;
    for(int i = 0; i < report_lst_sz; ++i) {
        seq_report_sty_t report_sty = ran_func->report->seq_report_sty[i];
        printf("Report style: %d\n", report_sty.report_type);
        for(int j = 0; j < report_sty.sz_seq_ran_param; ++j) {
            char * param_name = cp_ba_to_str(report_sty.ran_param[j].name);
            printf("RAN param Name:%s\n", param_name);
            printf("RAN param ID:%d\n", report_sty.ran_param[j].id);
            printf("\n");
        }
        printf("\n");
    }


    rc_sub.et.format = FORMAT_4_E2SM_RC_EV_TRIGGER_FORMAT; // UE Information Change
    rc_sub.et.frmt_4.sz_ue_info_chng = 1;
    rc_sub.et.frmt_4.ue_info_chng = calloc(1, sizeof(ue_info_chng_t));

    ue_info_chng_t *ev_trig_params = &(rc_sub.et.frmt_4.ue_info_chng[0]);

    ev_trig_params->ev_trig_cond_id = 4;
    ev_trig_params->type = L2_STATE_UE_INFO_CHNG_TRIGGER_TYPE;

    // Check this
    ev_trig_params->l2_state.sz_ran_param_test = 1;
    ev_trig_params->l2_state.ran_param_test = calloc(1, sizeof(ran_param_test_t));

    // Buffer Status Report
    ev_trig_params->l2_state.ran_param_test[0].ran_param_id = 13004; 
    ev_trig_params->l2_state.ran_param_test[0].type = STRUCTURE_RAN_PARAMETER_TYPE;

    ran_param_test_strct_t *bsr = &(ev_trig_params->l2_state.ran_param_test[0].strct);
    bsr->sz_strct = 5;
    bsr->ran_param_test = calloc(5, sizeof(ran_param_test_t));

    bsr->ran_param_test[0].ran_param_id = 13005;
    bsr->ran_param_test[0].type = ELEMENT_WITH_KEY_FLAG_FALSE_RAN_PARAMETER_TYPE;
    bsr->ran_param_test[0].flag_false.test_cond.cond = PRESENCE_RAN_PARAM_TEST_COND;
    bsr->ran_param_test[0].flag_false.test_cond.presence = PRESENT_RAN_PARAM_TEST_COND_PRESENCE;

    bsr->ran_param_test[1].ran_param_id = 13006;
    bsr->ran_param_test[1].type = ELEMENT_WITH_KEY_FLAG_FALSE_RAN_PARAMETER_TYPE;
    bsr->ran_param_test[1].flag_false.test_cond.cond = PRESENCE_RAN_PARAM_TEST_COND;
    bsr->ran_param_test[1].flag_false.test_cond.presence = PRESENT_RAN_PARAM_TEST_COND_PRESENCE;

    bsr->ran_param_test[2].ran_param_id = 13007;
    bsr->ran_param_test[2].type = ELEMENT_WITH_KEY_FLAG_FALSE_RAN_PARAMETER_TYPE;
    bsr->ran_param_test[2].flag_false.test_cond.cond = PRESENCE_RAN_PARAM_TEST_COND;
    bsr->ran_param_test[2].flag_false.test_cond.presence = PRESENT_RAN_PARAM_TEST_COND_PRESENCE;

    bsr->ran_param_test[3].ran_param_id = 13008;
    bsr->ran_param_test[3].type = ELEMENT_WITH_KEY_FLAG_FALSE_RAN_PARAMETER_TYPE;
    bsr->ran_param_test[3].flag_false.test_cond.cond = PRESENCE_RAN_PARAM_TEST_COND;
    bsr->ran_param_test[3].flag_false.test_cond.presence = PRESENT_RAN_PARAM_TEST_COND_PRESENCE;
    
    bsr->ran_param_test[4].ran_param_id = 13009;
    bsr->ran_param_test[4].type = ELEMENT_WITH_KEY_FLAG_FALSE_RAN_PARAMETER_TYPE;
    bsr->ran_param_test[4].flag_false.test_cond.cond = PRESENCE_RAN_PARAM_TEST_COND;
    bsr->ran_param_test[4].flag_false.test_cond.presence = PRESENT_RAN_PARAM_TEST_COND_PRESENCE;


    // Generate action definition.
    rc_sub.sz_ad = 1;
    rc_sub.ad = calloc(1, sizeof(e2sm_rc_action_def_t));

    rc_sub.ad[0].ric_style_type = 4;
    rc_sub.ad[0].format = FORMAT_1_E2SM_RC_ACT_DEF;

    e2sm_rc_act_def_frmt_1_t *frmt_1_data = &(rc_sub.ad[0].frmt_1);

    // We only want UL MAC CE (TS 38.321 6.1.3).
    frmt_1_data->sz_param_report_def = 1;
    frmt_1_data->param_report_def = calloc(1, sizeof(param_report_def_t));

    // RAN Parameter ID for UL MAC CE (See 8.2.4)
    frmt_1_data->param_report_def[0].ran_param_id = 100;
    
    // frmt_1_data->param_report_def[0].ran_param_id = 202;
    
    return rc_sub;

}

static 
void sm_cb_rc_report(const sm_ag_if_rd_t *rd) {
    printf("Hello World!\n");
}

int main(int argc, char *argv[]) {
    fr_args_t a = init_fr_args(argc, argv);

    init_xapp_api(&a);

    usleep(10000);

    e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
    assert(nodes.len > 0);

    for(int i = 0; i < nodes.len; ++i) {
        e2_node_connected_xapp_t *node = &nodes.n[i];

        size_t idx = find_sm_idx(node->rf, node->len_rf, eq_sm, RC_RAN_FUNCTION);
        assert(node->rf[idx].defn.type == RC_RAN_FUNC_DEF_E && "RC is not the received RAN Function");

        // Check if REPORT service is supported by E2 node
        if(node->rf[idx].defn.rc.report != NULL) {
            printf("Report service supported!\n");
            rc_sub_data_t rc_sub = gen_rc_subs( &(node->rf[idx].defn.rc) );
            
            // do this properly later.
            // get handle. then rm it.
            report_sm_xapp_api(&node->id, RC_RAN_FUNCTION, &rc_sub, sm_cb_rc_report);


            // free rc sub data? how? 
            // do this later.
        }
    }
    sleep(10);

    // Stop the xApp
    while (try_stop_xapp_api() == false)
        usleep(1000);

    free_e2_node_arr_xapp(&nodes);
}