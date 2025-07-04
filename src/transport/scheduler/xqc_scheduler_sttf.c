/*not yet tested*/
#include "src/transport/scheduler/xqc_scheduler_sttf.h"
#include "src/transport/scheduler/xqc_scheduler_common.h"
#include "src/transport/xqc_send_ctl.h"


static size_t
xqc_sttf_scheduler_size()
{
    return 0;
}

static void
xqc_sttf_scheduler_init(void *scheduler, xqc_log_t *log, xqc_scheduler_params_t *param)
{
    return;
}

xqc_path_ctx_t *
xqc_sttf_scheduler_get_path(void *scheduler,
                            xqc_connection_t *conn, xqc_packet_out_t *packet_out, int check_cwnd, int reinject,
                            xqc_bool_t *cc_blocked)
{
    xqc_path_ctx_t *best_path[XQC_PATH_CLASS_PERF_CLASS_SIZE];
    xqc_path_perf_class_t path_class;

    xqc_list_head_t *pos, *next;
    xqc_path_ctx_t *path;
    xqc_send_ctl_t *send_ctl;

    /* STTF metrics */
    uint64_t path_transmission_time = 0;
    uint64_t min_transmission_time[XQC_PATH_CLASS_PERF_CLASS_SIZE];
    xqc_bool_t reached_cwnd_check = XQC_FALSE;
    xqc_bool_t path_can_send = XQC_FALSE;

    for (path_class = XQC_PATH_CLASS_AVAILABLE_HIGH;
         path_class < XQC_PATH_CLASS_PERF_CLASS_SIZE;
    path_class++)
         {
             best_path[path_class] = NULL;
             min_transmission_time[path_class] = UINT64_MAX;
         }

         if (cc_blocked) {
             *cc_blocked = XQC_FALSE;
         }

         xqc_list_for_each_safe(pos, next, &conn->conn_paths_list) {
             path = xqc_list_entry(pos, xqc_path_ctx_t, path_list);

             path_class = xqc_path_get_perf_class(path);

             /* skip inactive paths */
             /* skip frozen paths */
             if (path->path_state != XQC_PATH_STATE_ACTIVE
                 || path->app_path_status == XQC_APP_PATH_STATUS_FROZEN
                 || (reinject && (packet_out->po_path_id == path->path_id)))
             {
                 goto skip_path;
             }

             if (!reached_cwnd_check) {
                 reached_cwnd_check = XQC_TRUE;
                 if (cc_blocked) {
                     *cc_blocked = XQC_TRUE;
                 }
             }

             path_can_send = xqc_scheduler_check_path_can_send(path, packet_out, check_cwnd);

             if (!path_can_send) {
                 goto skip_path;
             }

             if (cc_blocked) {
                 *cc_blocked = XQC_FALSE;
             }

             /* STTF calculation */
             uint64_t path_srtt = xqc_send_ctl_get_srtt(path->path_send_ctl);
             uint64_t path_bw = xqc_send_ctl_get_est_bw(path->path_send_ctl);
             uint64_t path_cwnd = path->path_send_ctl->ctl_cong_callback->xqc_cong_ctl_get_cwnd(
                 path->path_send_ctl->ctl_cong);
             uint64_t bytes_in_flight = path->path_send_ctl->ctl_bytes_in_flight;
             uint64_t available_capacity = (bytes_in_flight < path_cwnd) ?
             (path_cwnd - bytes_in_flight) : 0;

             /* Log pre-calculation metrics */
             xqc_log(conn->log, XQC_LOG_ERROR,
                "|STTF pre-calculation|conn:%p|path_id:%ui|srtt:%ui|bw:%ui|"
                "cwnd:%ui|inflight:%ui|avail_cap:%ui|pkt_size:%ui|",
                conn, path->path_id, path_srtt, path_bw, path_cwnd,
                bytes_in_flight, available_capacity, packet_out->po_used_size);

             if (path_bw > 0 && available_capacity > 0) {
                 uint64_t available_bandwidth = xqc_min(path_bw,
                                                        (available_capacity * 1000000) / (path_srtt > 0 ? path_srtt : 1));

                 path_transmission_time = path_srtt +
                 (packet_out->po_used_size * 1000000) / (available_bandwidth > 0 ? available_bandwidth : 1);


             } else {
                 /* If bandwidth is zero, use srtt as fallback */
                 path_transmission_time = path_srtt;
             }

             if (best_path[path_class] == NULL
                 || path_transmission_time < min_transmission_time[path_class])
             {
                 best_path[path_class] = path;
                 min_transmission_time[path_class] = path_transmission_time;
             }

             /* Final path metrics log */
                xqc_log(conn->log, XQC_LOG_ERROR,
                "|path sttf result|conn:%p|path_id:%ui|srtt:%ui|bw:%ui|cwnd:%ui|"
                "inflight:%ui|avail_cap:%ui|trans_time:%ui|path_class:%d|"
                "can_send:%d|path_status:%d|path_state:%d|reinj:%d|"
                "pkt_path_id:%ui|best_path:%i|",
                conn, path->path_id, path_srtt, path_bw, path_cwnd,
                bytes_in_flight, available_capacity, path_transmission_time, path_class,
                path_can_send, path->app_path_status, path->path_state, reinject,
                packet_out->po_path_id,
                best_path[path_class] ? best_path[path_class]->path_id : -1);

             skip_path:
             xqc_log(conn->log, XQC_LOG_ERROR,
                     "|path sttf|conn:%p|path_id:%ui|srtt:%ui|bw:%ui|cwnd:%ui|"
                     "inflight:%ui|avail_cap:%ui|trans_time:%ui|path_class:%d|"
                     "can_send:%d|path_status:%d|path_state:%d|reinj:%d|"
                     "pkt_path_id:%ui|best_path:%i|",
                     conn, path->path_id, path_srtt, path_bw, path_cwnd,
                     bytes_in_flight, available_capacity, path_transmission_time, path_class,
                     path_can_send, path->app_path_status, path->path_state, reinject,
                     packet_out->po_path_id,
                     best_path[path_class] ? best_path[path_class]->path_id : -1);
         }

         for (path_class = XQC_PATH_CLASS_AVAILABLE_HIGH;
              path_class < XQC_PATH_CLASS_PERF_CLASS_SIZE;
    path_class++)
              {
                  if (best_path[path_class] != NULL) {
                      xqc_log(conn->log, XQC_LOG_ERROR, "|best path:%ui|frame_type:%s|"
                      "pn:%ui|size:%ud|reinj:%d|path_class:%d|trans_time:%ui|",
                      best_path[path_class]->path_id,
                      xqc_frame_type_2_str(conn->engine, packet_out->po_frame_types),
                              packet_out->po_pkt.pkt_num,
                              packet_out->po_used_size, reinject, path_class,
                              min_transmission_time[path_class]);
                      return best_path[path_class];
                  }
              }

              xqc_log(conn->log, XQC_LOG_ERROR, "|No available paths to schedule|conn:%p|", conn);
              return NULL;
}

const xqc_scheduler_callback_t xqc_sttf_scheduler_cb = {
    .xqc_scheduler_size             = xqc_sttf_scheduler_size,
    .xqc_scheduler_init             = xqc_sttf_scheduler_init,
    .xqc_scheduler_get_path         = xqc_sttf_scheduler_get_path,
};
