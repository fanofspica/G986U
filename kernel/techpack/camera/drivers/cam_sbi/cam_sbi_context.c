/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

#include "cam_mem_mgr.h"
#include "cam_trace.h"
#include "cam_debug_util.h"
#include "cam_sbi_context.h"


static const char sbi_dev_name[] = "cam-sbi";

static int __cam_sbi_ctx_handle_irq_in_activated(
	void *context, uint32_t evt_id, void *evt_data);

#define CAM_SBI_SET_STATE(ctx, new_state) \
{ \
	ctx->state = new_state; \
	CAM_INFO(CAM_SBI, "%d:%s", ctx->ctx_id, #new_state); \
}


static int __cam_sbi_ctx_flush_req(struct cam_context *ctx,
	struct list_head *req_list, struct cam_req_mgr_flush_request *flush_req)
{
	int i, rc;
	uint32_t cancel_req_id_found = 0;
	struct cam_ctx_request        *req;
	struct cam_ctx_request        *req_temp;
	struct cam_sbi_dev_ctx_req    *req_custom;
	struct list_head               flush_list;

	INIT_LIST_HEAD(&flush_list);
	if (list_empty(req_list)) {
		CAM_INFO(CAM_SBI, "request list is empty");
		if (flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ) {
			CAM_ERR(CAM_SBI, "no request to cancel");
			return -EINVAL;
		} else {
			return 0;
		}
	}

	CAM_INFO(CAM_SBI, "Flush [%u] in progress for req_id %llu",
		flush_req->type, flush_req->req_id);
	list_for_each_entry_safe(req, req_temp, req_list, list) {
		if (flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ) {
			if (req->request_id != flush_req->req_id) {
				continue;
			} else {
				list_del_init(&req->list);
				list_add_tail(&req->list, &flush_list);
				cancel_req_id_found = 1;
				break;
			}
		}
		list_del_init(&req->list);
		list_add_tail(&req->list, &flush_list);
	}

	list_for_each_entry_safe(req, req_temp, &flush_list, list) {
		req_custom = (struct cam_sbi_dev_ctx_req *) req->req_priv;
		for (i = 0; i < req_custom->num_fence_map_out; i++) {
			if (req_custom->fence_map_out[i].sync_id != -1) {
				CAM_INFO(CAM_SBI,
					"Flush req 0x%llx, fence %d",
					req->request_id,
					req_custom->fence_map_out[i].sync_id);
				rc = cam_sync_signal(
					req_custom->fence_map_out[i].sync_id,
					CAM_SYNC_STATE_SIGNALED_ERROR);
				if (rc)
					CAM_ERR_RATE_LIMIT(CAM_SBI,
						"signal fence failed\n");
				req_custom->fence_map_out[i].sync_id = -1;
			}
		}
		list_add_tail(&req->list, &ctx->free_req_list);
	}

	if (flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ &&
		!cancel_req_id_found)
		CAM_INFO(CAM_SBI,
			"Flush request id:%lld is not found in the list",
			flush_req->req_id);

	return 0;
}

static int
__cam_sbi_ctx_link________in_acquired_(struct cam_context *ctx,
				struct cam_req_mgr_core_dev_link_setup *link)
{
	int rc = 0;
	struct cam_sbi_dev_context *ctx_sbi =
		(struct cam_sbi_dev_context *)ctx->ctx_priv;

	CAM_DBG(CAM_SBI, "ctx_id %d link %p, link->crm_cb %p",
		ctx->ctx_id, link, link->crm_cb);

	ctx->link_hdl = link->link_hdl;
	ctx->ctx_crm_intf = link->crm_cb;
	ctx_sbi->subscribe_event = link->subscribe_event;

	/* change state only if we had the init config */
	if (ctx_sbi->init_received) {
		CAM_SBI_SET_STATE(ctx, CAM_CTX_READY);
	}

	return rc;
}


static int
__cam_sbi_ctx_unlink_in_acquired(struct cam_context *ctx,
				 struct cam_req_mgr_core_dev_link_setup *unlink)
{
	int rc = 0;

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	ctx->link_hdl = -1;
	ctx->ctx_crm_intf = NULL;

	return rc;
}


static int
__cam_sbi_ctx_get_dev_info_in_acquired(struct cam_context *ctx,
					   struct cam_req_mgr_device_info *dev_info)
{
	int rc = 0;

	dev_info->dev_hdl = ctx->dev_hdl;
	strlcpy(dev_info->name, CAM_SBI_DEV_NAME, sizeof(dev_info->name));
	dev_info->dev_id = CAM_REQ_MGR_DEVICE_SBI;
	dev_info->p_delay = 1;
	dev_info->trigger = CAM_TRIGGER_POINT_SOF;

	return rc;
}



static int __cam_sbi_ctx_apply_req(
	struct cam_context *ctx, struct cam_req_mgr_apply_request *apply)
{
	int rc = 0;
	struct cam_ctx_request          *req;
	struct cam_sbi_dev_ctx_req       *req_sbi;
	struct cam_sbi_dev_context       *sbi_ctx = NULL;
	struct cam_hw_config_args        cfg;

	if (list_empty(&ctx->pending_req_list)) {
		CAM_ERR(CAM_SBI, "No available request for Apply id %lld",
			apply->request_id);
		rc = -EFAULT;
		goto end;
	}

	sbi_ctx = (struct cam_sbi_dev_context *) ctx->ctx_priv;
	spin_lock_bh(&ctx->lock);
	req = list_first_entry(&ctx->pending_req_list, struct cam_ctx_request,
		list);
	spin_unlock_bh(&ctx->lock);

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	/*
	 * Check whether the request id is matching the tip
	 */
	if (req->request_id != apply->request_id) {
		CAM_INFO_RATE_LIMIT(CAM_SBI,
			"Invalid Request Id asking %llu existing %llu",
			apply->request_id, req->request_id);
		/* don't care about bubble state on preview - bh1212 20191007 */
		rc = 0;
		goto end;
	}

	req_sbi = (struct cam_sbi_dev_ctx_req *) req->req_priv;
	//CAM_DBG(CAM_SBI, "req_sbi %p", req_sbi);

	cfg.ctxt_to_hw_map = sbi_ctx->hw_ctx;
	cfg.request_id = req->request_id;
	cfg.hw_update_entries = req_sbi->cfg;
	cfg.num_hw_update_entries = 1; //req_sbi->num_cfg;  //for test. org
	//cfg.num_hw_update_entries = req_sbi->num_cfg;
	cfg.priv  = &req_sbi->hw_update_data;
	cfg.init_packet = 0;

	//CAM_DBG(CAM_SBI, "cfg.num_hw_update_entries %d", cfg.num_hw_update_entries);

	rc = ctx->hw_mgr_intf->hw_config(ctx->hw_mgr_intf->hw_mgr_priv, &cfg);

	if (rc) {
		CAM_ERR_RATE_LIMIT(CAM_SBI,
			"Can not apply the configuration");
	} else {
		spin_lock_bh(&ctx->lock);
		list_del_init(&req->list);
		if (!req->num_out_map_entries) {
			//CAM_DBG(CAM_SBI, "No Outmap entries");
			list_add_tail(&req->list, &ctx->free_req_list);
			spin_unlock_bh(&ctx->lock);
		} else {
			//CAM_DBG(CAM_SBI, "Outmap entries: %d", req->num_out_map_entries);
			list_add_tail(&req->list, &ctx->active_req_list);
			spin_unlock_bh(&ctx->lock);
			/*
			 * for test purposes only-this should be
			 * triggered based on irq
			 */
			 __cam_sbi_ctx_handle_irq_in_activated(ctx, 0, NULL);
		}
	}

end:
	return rc;
}


static int __cam_sbi_ctx_acquire_hw_v1(
	struct cam_context *ctx, void *args)
{
	int rc = 0;
	struct cam_acquire_hw_cmd_v1 *cmd =
		(struct cam_acquire_hw_cmd_v1 *)args;
	struct cam_hw_acquire_args         param;
	struct cam_sbi_dev_context         *ctx_sbi =
		(struct cam_sbi_dev_context *)  ctx->ctx_priv;
	struct cam_sbi_acquire_hw_info *acquire_hw_info = NULL;
	//struct cam_sbi_hw_mgr * hw_mgr_priv;

	if (!ctx->hw_mgr_intf) {
		CAM_ERR(CAM_SBI, "HW interface is not ready");
		rc = -EFAULT;
		goto end;
	}

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	//CAM_DBG(CAM_SBI,
	//	"session_hdl 0x%x, hdl type %d, res %lld",
	//	cmd->session_handle, cmd->handle_type, cmd->resource_hdl);

	if (cmd->handle_type != 1)  {
		CAM_ERR(CAM_SBI, "Only user pointer is supported");
		rc = -EINVAL;
		goto end;
	}

	/* ignore - temp bh1212 */
	//if (cmd->data_size < sizeof(*acquire_hw_info)) {
	//	CAM_ERR(CAM_SBI, "data_size is not a valid value");
	//	goto end;
	//}

	acquire_hw_info = kzalloc(cmd->data_size, GFP_KERNEL);
	if (!acquire_hw_info) {
		rc = -ENOMEM;
		goto end;
	}

	CAM_DBG(CAM_SBI, "start copy resources from user");

	if (copy_from_user(acquire_hw_info, (void __user *)cmd->resource_hdl,
		cmd->data_size)) {
		rc = -EFAULT;
		goto free_res;
	}

	memset(&param, 0, sizeof(param));
	param.context_data = ctx;
	param.event_cb = ctx->irq_cb_intf;
	param.acquire_info_size = cmd->data_size;
	param.acquire_info = (uint64_t) acquire_hw_info;

	/* call HW manager to reserve the resource */
	rc = ctx->hw_mgr_intf->hw_acquire(ctx->hw_mgr_intf->hw_mgr_priv,
		&param);
	if (rc != 0) {
		CAM_ERR(CAM_SBI, "Acquire HW failed");
		goto free_res;
	}

	ctx_sbi->hw_ctx = param.ctxt_to_hw_map;
	ctx_sbi->hw_acquired = true;
	ctx->ctxt_to_hw_map = param.ctxt_to_hw_map;

	CAM_DBG(CAM_SBI,
		"Acquire HW success on session_hdl 0x%xs for ctx_id %u",
		ctx->session_hdl, ctx->ctx_id);

	kfree(acquire_hw_info);
	return rc;

free_res:
	kfree(acquire_hw_info);
end:
	return rc;
}


static int __cam_sbi_ctx_acquire_dev_in_available(struct cam_context *ctx,
	struct cam_acquire_dev_cmd *cmd)
{
	int rc = 0;
	struct cam_create_dev_hdl  req_hdl_param;

	if (!ctx->hw_mgr_intf) {
		CAM_ERR(CAM_SBI, "HW interface is not ready");
		rc = -EFAULT;
		return rc;
	}

	if (cmd->handle_type != 1)	{
		CAM_ERR(CAM_SBI, "Only user pointer is supported");
		rc = -EINVAL;
		return rc;
	}

	CAM_DBG(CAM_SBI,
		"session_hdl 0x%x, num_resources %d, hdl type %d, res %lld",
		cmd->session_handle, cmd->num_resources,
		cmd->handle_type, cmd->resource_hdl);

	if (cmd->num_resources != CAM_API_COMPAT_CONSTANT) {
		CAM_ERR(CAM_SBI, "Invalid num_resources 0x%x",
			cmd->num_resources);
		return -EINVAL;
	}

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	req_hdl_param.session_hdl = cmd->session_handle;
	req_hdl_param.v4l2_sub_dev_flag = 0;
	req_hdl_param.media_entity_flag = 0;
	req_hdl_param.ops = ctx->crm_ctx_intf;
	req_hdl_param.priv = ctx;

	CAM_DBG(CAM_SBI, "get device handle from bridge");
	ctx->dev_hdl = cam_create_device_hdl(&req_hdl_param);
	if (ctx->dev_hdl <= 0) {
		rc = -EFAULT;
		CAM_ERR(CAM_SBI, "Can not create device handle");
		return rc;
	}

	cmd->dev_handle = ctx->dev_hdl;
	ctx->session_hdl = cmd->session_handle;
	CAM_SBI_SET_STATE(ctx, CAM_CTX_ACQUIRED);

	CAM_INFO(CAM_SBI,
		"Acquire dev success on session_hdl 0x%x for ctx_id %u, dev_hdl 0x%x",
		cmd->session_handle, ctx->ctx_id, ctx->dev_hdl);

	return rc;
}

static int __cam_sbi_ctx_release_hw__in_top_state(
	struct cam_context *ctx,
	void *cmd)
{
	int rc = 0;
	struct cam_sbi_dev_context * sbi_ctx =
		(struct cam_sbi_dev_context *) ctx->ctx_priv;
	struct cam_req_mgr_flush_request flush_req;
	struct cam_hw_release_args rel_arg;

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	if (sbi_ctx->hw_ctx) {
		rel_arg.ctxt_to_hw_map = sbi_ctx->hw_ctx;
		rc = ctx->hw_mgr_intf->hw_release(ctx->hw_mgr_intf->hw_mgr_priv,
			&rel_arg);
		sbi_ctx->hw_ctx = NULL;
		if (rc)
			CAM_ERR(CAM_SBI,
				"Failed to release HW for ctx:%u", ctx->ctx_id);
	} else {
		CAM_ERR(CAM_SBI, "No HW resources acquired for this ctx");
	}

	ctx->last_flush_req = 0;
	sbi_ctx->frame_id = 0;
	sbi_ctx->active_req_cnt = 0;
	sbi_ctx->hw_acquired = false;
	sbi_ctx->init_received = false;

	/* Flush all the pending request list  */
	flush_req.type = CAM_REQ_MGR_FLUSH_TYPE_ALL;
	flush_req.link_hdl = ctx->link_hdl;
	flush_req.dev_hdl = ctx->dev_hdl;

	CAM_DBG(CAM_SBI, "try to flush pending list");
	spin_lock_bh(&ctx->lock);
	rc = __cam_sbi_ctx_flush_req(ctx, &ctx->pending_req_list,
		&flush_req);
	spin_unlock_bh(&ctx->lock);
	CAM_SBI_SET_STATE(ctx, CAM_CTX_ACQUIRED);


	return rc;
}

static int __cam_sbi_ctx_release_dev_in_acquired_(
	struct cam_context *ctx,
	struct cam_release_dev_cmd *cmd)
{
	int rc = 0;
	struct cam_sbi_dev_context * sbi_ctx =
		(struct cam_sbi_dev_context *) ctx->ctx_priv;
	struct cam_req_mgr_flush_request flush_req;

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	if (cmd && sbi_ctx->hw_ctx) {
		rc = __cam_sbi_ctx_release_hw__in_top_state(ctx, NULL);
		if (rc)
			CAM_ERR(CAM_SBI, "Release hw failed rc=%d", rc);
	}

	ctx->ctx_crm_intf = NULL;

	ctx->last_flush_req = 0;
	sbi_ctx->frame_id = 0;
	sbi_ctx->active_req_cnt = 0;
	sbi_ctx->hw_acquired = false;
	sbi_ctx->init_received = false;

	/* Flush all the pending request list  */
	flush_req.type = CAM_REQ_MGR_FLUSH_TYPE_ALL;
	flush_req.link_hdl = ctx->link_hdl;
	flush_req.dev_hdl = ctx->dev_hdl;

	CAM_DBG(CAM_SBI, "try to flush pending list");
	spin_lock_bh(&ctx->lock);
	rc = __cam_sbi_ctx_flush_req(ctx, &ctx->pending_req_list,
		&flush_req);
	spin_unlock_bh(&ctx->lock);
	CAM_SBI_SET_STATE(ctx, CAM_CTX_AVAILABLE);

	return rc;
}


static int __cam_sbi_ctx_start___dev_in_ready____(struct cam_context *ctx,
	struct cam_start_stop_dev_cmd *cmd)
{
	int rc = 0;
	struct cam_hw_config_args        hw_config;
	struct cam_ctx_request          *req;
	struct cam_sbi_dev_ctx_req   *req_custom;
	struct cam_sbi_dev_context *ctx_custom =
		(struct cam_sbi_dev_context *) ctx->ctx_priv;

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	if (cmd->session_handle != ctx->session_hdl ||
		cmd->dev_handle != ctx->dev_hdl) {
		rc = -EPERM;
		goto end;
	}

	if (list_empty(&ctx->pending_req_list)) {
		/* should never happen */
		CAM_ERR(CAM_SBI, "Start device with empty configuration");
		rc = -EFAULT;
		goto end;
	} else {
		req = list_first_entry(&ctx->pending_req_list,
			struct cam_ctx_request, list);
	}
	req_custom = (struct cam_sbi_dev_ctx_req *) req->req_priv;

	if (!ctx_custom->hw_ctx) {
		CAM_ERR(CAM_SBI, "Wrong hw context pointer.");
		rc = -EFAULT;
		goto end;
	}

	hw_config.ctxt_to_hw_map = ctx_custom->hw_ctx;
	hw_config.request_id = req->request_id;
	hw_config.hw_update_entries = req_custom->cfg;
	hw_config.num_hw_update_entries = req_custom->num_cfg;
	hw_config.priv  = &req_custom->hw_update_data;
	hw_config.init_packet = 1;

	CAM_SBI_SET_STATE(ctx, CAM_CTX_ACTIVATED);

	rc = ctx->hw_mgr_intf->hw_start(ctx->hw_mgr_intf->hw_mgr_priv,
		&hw_config);
	if (rc) {
		/* HW failure. User need to clean up the resource */
		CAM_ERR(CAM_SBI, "Start HW failed");
		CAM_SBI_SET_STATE(ctx, CAM_CTX_READY);
		goto end;
	}

	CAM_DBG(CAM_SBI, "start device success ctx %u",
		ctx->ctx_id);

	spin_lock_bh(&ctx->lock);
	list_del_init(&req->list);
	if (req_custom->num_fence_map_out)
		list_add_tail(&req->list, &ctx->active_req_list);
	else
		list_add_tail(&req->list, &ctx->free_req_list);
	spin_unlock_bh(&ctx->lock);

end:
	return rc;
}




static int __cam_sbi_ctx_flush_dev_in_activated(struct cam_context *ctx,
	struct cam_flush_dev_cmd *cmd)
{
	int rc;

	CAM_DBG(CAM_SBI, "Enter.. ctx_id %d", ctx->ctx_id);

	rc = cam_context_flush_dev_to_hw(ctx, cmd);
	if (rc)
		CAM_ERR(CAM_SBI, "Failed to flush device");

	return rc;
}



static int __cam_sbi_ctx_stop____dev_in_activated(struct cam_context *ctx,
	struct cam_start_stop_dev_cmd *cmd)
{
	int rc = 0;

	CAM_DBG(CAM_SBI, "Enter ctx_id %d", ctx->ctx_id);

	rc = cam_context_stop_dev_to_hw(ctx);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to stop dev");
		return rc;
	}
	CAM_SBI_SET_STATE(ctx, CAM_CTX_ACQUIRED);

	return rc;
}

static int __cam_sbi_ctx_release_dev_in_activated(struct cam_context *ctx,
	struct cam_release_dev_cmd *cmd)
{
	int rc = 0;

	CAM_DBG(CAM_SBI, "Enter ctx_id %d", ctx->ctx_id);

	rc = __cam_sbi_ctx_stop____dev_in_activated(ctx, NULL);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to stop");
		return rc;
	}

	//rc = cam_context_release_dev_to_hw(ctx, cmd);
	rc = __cam_sbi_ctx_release_dev_in_acquired_(ctx, cmd);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to release");
		return rc;
	}
	//CAM_SBI_SET_STATE(ctx, CAM_CTX_AVAILABLE);

	return rc;
}

static int __cam_sbi_ctx_release_hw_in_activated_state(
	struct cam_context *ctx, void *cmd)
{
	int rc = 0;

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	rc = __cam_sbi_ctx_stop____dev_in_activated(ctx, NULL);
	if (rc)
		CAM_ERR(CAM_SBI, "Stop device failed rc=%d", rc);

	rc = __cam_sbi_ctx_release_hw__in_top_state(ctx, cmd);
	if (rc)
		CAM_ERR(CAM_SBI, "Release hw failed rc=%d", rc);

	return rc;
}

static int __cam_sbi_ctx_enqueue_request_in_order(
	struct cam_context *ctx, struct cam_ctx_request *req)
{
	struct cam_ctx_request           *req_current;
	struct cam_ctx_request           *req_prev;
	struct list_head                  temp_list;

	INIT_LIST_HEAD(&temp_list);
	spin_lock_bh(&ctx->lock);
	if (list_empty(&ctx->pending_req_list)) {
		list_add_tail(&req->list, &ctx->pending_req_list);
	} else {
		list_for_each_entry_safe_reverse(
			req_current, req_prev, &ctx->pending_req_list, list) {
			if (req->request_id < req_current->request_id) {
				list_del_init(&req_current->list);
				list_add(&req_current->list, &temp_list);
				continue;
			} else if (req->request_id == req_current->request_id) {
				CAM_WARN(CAM_SBI,
					"Received duplicated request %lld",
					req->request_id);
			}
			break;
		}
		list_add_tail(&req->list, &ctx->pending_req_list);

		if (!list_empty(&temp_list)) {
			list_for_each_entry_safe(
				req_current, req_prev, &temp_list, list) {
				list_del_init(&req_current->list);
				list_add_tail(&req_current->list,
					&ctx->pending_req_list);
			}
		}
	}
	spin_unlock_bh(&ctx->lock);
	return 0;
}



static int __cam_sbi_ctx_enqueue_init_request(struct cam_context *ctx,
						  struct cam_ctx_request *req)
{
	int rc = 0;
	struct cam_ctx_request *req_old;
	struct cam_sbi_dev_ctx_req *req_sbi_old;
	struct cam_sbi_dev_ctx_req *req_sbi_new;

	spin_lock_bh(&ctx->lock);
	if (list_empty(&ctx->pending_req_list)) {
		list_add_tail(&req->list, &ctx->pending_req_list);
		CAM_DBG(CAM_SBI, "INIT packet added req id= %d",
			req->request_id);
		goto end;
	}

	req_old = list_first_entry(&ctx->pending_req_list,
				   struct cam_ctx_request, list);
	req_sbi_old = (struct cam_sbi_dev_ctx_req *)req_old->req_priv;
	req_sbi_new = (struct cam_sbi_dev_ctx_req *)req->req_priv;
	if (req_sbi_old->hw_update_data.packet_opcode_type ==
		CAM_SBI_PACKET_INIT_DEV) {
		if ((req_sbi_old->num_cfg + req_sbi_new->num_cfg) >=
			CAM_SBI_CTX_CFG_MAX) {
			CAM_WARN(CAM_SBI, "Can not merge INIT pkt");
			rc = -ENOMEM;
		}

		if (req_sbi_old->num_fence_map_out != 0 ||
			req_sbi_old->num_fence_map_in != 0) {
			CAM_WARN(CAM_SBI, "Invalid INIT pkt sequence");
			rc = -EINVAL;
		}

		if (!rc) {
			memcpy(req_sbi_old->fence_map_out,
				   req_sbi_new->fence_map_out,
				   sizeof(req_sbi_new->fence_map_out[0]) *
					   req_sbi_new->num_fence_map_out);
			req_sbi_old->num_fence_map_out =
				req_sbi_new->num_fence_map_out;

			memcpy(req_sbi_old->fence_map_in,
				   req_sbi_new->fence_map_in,
				   sizeof(req_sbi_new->fence_map_in[0]) *
					   req_sbi_new->num_fence_map_in);
			req_sbi_old->num_fence_map_in =
				req_sbi_new->num_fence_map_in;

			memcpy(&req_sbi_old->cfg[req_sbi_old->num_cfg],
				   req_sbi_new->cfg,
				   sizeof(req_sbi_new->cfg[0]) *
					   req_sbi_new->num_cfg);
			req_sbi_old->num_cfg += req_sbi_new->num_cfg;

			req_old->request_id = req->request_id;

			list_add_tail(&req->list, &ctx->free_req_list);
		}
	} else {
		CAM_WARN(CAM_SBI,
			 "Received Update pkt before INIT pkt. req_id= %lld",
			 req->request_id);
		rc = -EINVAL;
	}
end:
	spin_unlock_bh(&ctx->lock);
	return rc;
}



static int __cam_sbi_ctx_config_dev_in_top_state(
	struct cam_context *ctx, struct cam_config_dev_cmd *cmd)
{
	int rc = 0, i;
	struct cam_ctx_request           *req = NULL;
	struct cam_sbi_dev_ctx_req       *req_sbi;
	uintptr_t                         packet_addr;
	struct cam_packet                *packet;
	size_t                            len = 0;
	size_t                            remain_len = 0;
	struct cam_hw_prepare_update_args cfg;
	struct cam_req_mgr_add_request    add_req;
	struct cam_sbi_dev_context           *ctx_sbi =
		(struct cam_sbi_dev_context *)ctx->ctx_priv;

	/* get free request */
	spin_lock_bh(&ctx->lock);
	if (!list_empty(&ctx->free_req_list)) {
		req = list_first_entry(&ctx->free_req_list,
				struct cam_ctx_request, list);
		list_del_init(&req->list);
	}
	spin_unlock_bh(&ctx->lock);

	if (!req) {
		CAM_ERR(CAM_SBI, "No more request obj free");
		return -ENOMEM;
	}

	req_sbi = (struct cam_sbi_dev_ctx_req *)req->req_priv;

	/* for config dev, only memory handle is supported */
	/* map packet from the memhandle */
	rc = cam_mem_get_cpu_buf((int32_t) cmd->packet_handle,
		&packet_addr, &len);
	if (rc != 0) {
		CAM_ERR(CAM_SBI, "Can not get packet address");
		rc = -EINVAL;
		goto free_req;
	}

	remain_len = len;
	if ((len < sizeof(struct cam_packet)) ||
		((size_t)cmd->offset >= len - sizeof(struct cam_packet))) {
		CAM_ERR(CAM_SBI, "invalid buff length: %zu or offset", len);
		rc = -EINVAL;
		goto free_req;
	}

	remain_len -= (size_t)cmd->offset;
	packet = (struct cam_packet *)(packet_addr + (uint32_t)cmd->offset);
	//CAM_DBG(CAM_SBI, "pack_handle %llx", cmd->packet_handle);
	//CAM_DBG(CAM_SBI, "packet address is 0x%zx", packet_addr);
	//CAM_DBG(CAM_SBI, "packet with length %zu, offset 0x%llx", len, cmd->offset);
	//CAM_DBG(CAM_SBI, "Packet request id %lld", packet->header.request_id);
	//CAM_DBG(CAM_SBI, "Packet size 0x%x", packet->header.size);
	//CAM_DBG(CAM_SBI, "packet op %d", packet->header.op_code);

	if ((((packet->header.op_code + 1) & 0xF) == CAM_SBI_PACKET_UPDATE_DEV)
		&& (packet->header.request_id <= ctx->last_flush_req)) {
		CAM_INFO(CAM_SBI,
			"request %lld has been flushed, reject packet",
			packet->header.request_id);
		rc = -EINVAL;
		goto free_req;
	}

	/* preprocess the configuration */
	memset(&cfg, 0, sizeof(cfg));
	cfg.packet = packet;
	cfg.remain_len = remain_len;
	cfg.ctxt_to_hw_map = ctx_sbi->hw_ctx;
	cfg.max_hw_update_entries = CAM_SBI_CTX_CFG_MAX;
	cfg.hw_update_entries = req_sbi->cfg;
	cfg.max_out_map_entries = CAM_SBI_DEV_CTX_RES_MAX;
	cfg.max_in_map_entries = CAM_SBI_DEV_CTX_RES_MAX;
	cfg.out_map_entries = req_sbi->fence_map_out;
	cfg.in_map_entries = req_sbi->fence_map_in;
	cfg.priv  = &req_sbi->hw_update_data;
	cfg.pf_data = &(req->pf_data);

	rc = ctx->hw_mgr_intf->hw_prepare_update(
		ctx->hw_mgr_intf->hw_mgr_priv, &cfg);
	if (rc != 0) {
		CAM_ERR(CAM_SBI, "Prepare config packet failed in HW layer");
		rc = -EFAULT;
		goto free_req;
	}
	//CAM_DBG(CAM_SBI, "__cam_sbi_ctx_config_dev_in_top_state 1, req_sbi : %p", req_sbi);
	//CAM_DBG(CAM_SBI, "cfg.num_hw_update_entries : %d", cfg.num_hw_update_entries);
	//CAM_DBG(CAM_SBI, "cfg.num_out_map_entries : %d", cfg.num_out_map_entries);
	//CAM_DBG(CAM_SBI, "cfg.num_in_map_entries : %d", cfg.num_in_map_entries);

	//if (0)
	{
		//CAM_DBG(CAM_SBI, "cfg.num_hw_update_entries %d", cfg.num_hw_update_entries);
		req_sbi->num_cfg = cfg.num_hw_update_entries;
		req_sbi->num_fence_map_out = cfg.num_out_map_entries;
		req_sbi->num_fence_map_in = cfg.num_in_map_entries;
		req_sbi->num_acked = 0;
		req_sbi->bubble_detected = false;


		for (i = 0; i < req_sbi->num_fence_map_out; i++) {
			rc = cam_sync_get_obj_ref(req_sbi->fence_map_out[i].sync_id);
			if (rc) {
				CAM_ERR(CAM_SBI, "Can't get ref for fence %d",
					req_sbi->fence_map_out[i].sync_id);
				goto put_ref;
			}
		}

		// CAM_DBG(CAM_SBI, "num_entry: %d, num fence out: %d, num fence in: %d",
		// 	req_sbi->num_cfg, req_sbi->num_fence_map_out,
		// 	req_sbi->num_fence_map_in);

		req->request_id = packet->header.request_id;
		req->status = 1;

		if ((packet->header.op_code & 0xFFF) == CAM_SBI_PACKET_INIT_DEV) {
			CAM_DBG(CAM_SBI, "CAM_SBI_PACKET_INIT_DEV");
			req_sbi->hw_update_data.packet_opcode_type = CAM_SBI_PACKET_INIT_DEV;
		}
		//TODO: remove CAM_SBI_PACKET_RESTART_DEV - bh1212 temp for 1st preview enable
		else if ((packet->header.op_code & 0xFFF) == CAM_SBI_PACKET_RESTART_DEV) {
			CAM_DBG(CAM_SBI, "CAM_SBI_PACKET_RESTART_DEV");
			req_sbi->hw_update_data.packet_opcode_type = CAM_SBI_PACKET_RESTART_DEV;
		}
		else {
			CAM_DBG(CAM_SBI, "CAM_SBI_PACKET_UPDATE_DEV");
			req_sbi->hw_update_data.packet_opcode_type = CAM_SBI_PACKET_UPDATE_DEV;
		}


		//CAM_DBG(CAM_SBI, "Packet request id %lld packet opcode:%d",
		//	packet->header.request_id,
		//	req_sbi->hw_update_data.packet_opcode_type);

		if (req_sbi->hw_update_data.packet_opcode_type == CAM_SBI_PACKET_OP_BASE) {
			CAM_ERR(CAM_SBI, "Rxed opcode 0");
		}

		if (req_sbi->hw_update_data.packet_opcode_type == CAM_SBI_PACKET_INIT_DEV) {
			if (ctx->state < CAM_CTX_ACTIVATED) {
				rc = __cam_sbi_ctx_enqueue_init_request(ctx, req);
				if (rc)
					CAM_ERR(CAM_SBI, "Enqueue INIT pkt failed");
				ctx_sbi->init_received = true;
			}
			else {
				rc = -EINVAL;
				CAM_ERR(CAM_SBI, "Recevied INIT pkt in wrong state");
			}
		}
		else if ((req_sbi->hw_update_data.packet_opcode_type == CAM_SBI_PACKET_UPDATE_DEV) ||
				 (req_sbi->hw_update_data.packet_opcode_type == CAM_SBI_PACKET_RESTART_DEV)) {

			//CAM_DBG(CAM_REQ,"Stage 1 : %lld", req->request_id);

			if (ctx->ctx_crm_intf && ctx->ctx_crm_intf->add_req) { //Fallback to old change
			   //CAM_DBG(CAM_REQ,"Stage 2 ");

				add_req.link_hdl = ctx->link_hdl;
				add_req.dev_hdl = ctx->dev_hdl;
				add_req.req_id = req->request_id;
				add_req.skip_before_applying = 0;
				rc = ctx->ctx_crm_intf->add_req(&add_req);
				//CAM_DBG(CAM_REQ,"SBI:: Add request: %lld", req->request_id);

				if (rc) {
					CAM_ERR(CAM_SBI, "Add req failed: req id=%llu",
						req->request_id);
				}
				else {
					__cam_sbi_ctx_enqueue_request_in_order(ctx, req);
					//CAM_DBG(CAM_SBI, "Added req success: req id=%llu", req->request_id);
				}
			}
			else {
				rc = -EINVAL;
				CAM_ERR(CAM_SBI, "Recevied Update in wrong state");
			}
		}
		if (rc)
			goto put_ref;
	}

	CAM_DBG(CAM_REQ,
		"Preprocessing Config req_id %lld successful on ctx %u",
		req->request_id, ctx->ctx_id);

	return rc;

put_ref:
	for (--i; i >= 0; i--) {
		if (cam_sync_put_obj_ref(req_sbi->fence_map_out[i].sync_id))
			CAM_ERR(CAM_CTXT, "Failed to put ref of fence %d",
				req_sbi->fence_map_out[i].sync_id);
	}
free_req:
	spin_lock_bh(&ctx->lock);
	list_add_tail(&req->list, &ctx->free_req_list);
	spin_unlock_bh(&ctx->lock);

	return rc;
}



static int __cam_sbi_ctx_config__dev_in_acquired_(struct cam_context *ctx,
						struct cam_config_dev_cmd *cmd)
{
	int rc = 0;
	struct cam_sbi_dev_context        *ctx_sbi =
		(struct cam_sbi_dev_context *) ctx->ctx_priv;

	if (!ctx_sbi->hw_acquired) {
		CAM_ERR(CAM_SBI, "HW not acquired, reject config packet");
		return -EAGAIN;
	}

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);
	rc = __cam_sbi_ctx_config_dev_in_top_state(ctx, cmd);

	if (!rc && (ctx->link_hdl >= 0)) {
		CAM_SBI_SET_STATE(ctx, CAM_CTX_READY);
		trace_cam_context_state("SBI", ctx);
	}

	return rc;
}


static int __cam_sbi_ctx_handle_irq_in_activated(void *context,
	uint32_t evt_id, void *evt_data)
{
	int rc;

	rc = cam_context_buf_done_from_hw(context, evt_data, evt_id);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed in buf done, rc=%d", rc);
		return rc;
	}

	return rc;
}

static int __cam_sbi_ctx_acquire_hw_in_acquired(
	struct cam_context *ctx, void *args)
{
	int rc = -EINVAL;
	uint32_t api_version;

	if (!ctx || !args) {
		CAM_ERR(CAM_ISP, "Invalid input pointer");
		return rc;
	}

	if (ctx->state == CAM_CTX_READY) { return 0; }

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	api_version = *((uint32_t *)args);
	if (api_version == 1)
		rc = __cam_sbi_ctx_acquire_hw_v1(ctx, args);
	else
		CAM_ERR(CAM_ISP, "Unsupported api version %d",
			api_version);

	return rc;
}

static int __cam_sbi_ctx_flush_req_in_top_state(
	struct cam_context *ctx,
	struct cam_req_mgr_flush_request *flush_req)
{
	int rc = 0;

	if (flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_ALL) {
		CAM_INFO(CAM_SBI, "Last request id to flush is %lld",
			flush_req->req_id);
		ctx->last_flush_req = flush_req->req_id;
	}

	spin_lock_bh(&ctx->lock);
	rc = __cam_sbi_ctx_flush_req(ctx, &ctx->pending_req_list, flush_req);
	spin_unlock_bh(&ctx->lock);

	return rc;
}

/* top state machine */
static struct cam_ctx_ops
	cam_sbi_ctx_state_machine[CAM_CTX_STATE_MAX] = {
	/* Uninit */
	{
		.ioctl_ops = {},
		.crm_ops = {.link = __cam_sbi_ctx_link________in_acquired_,},
		.irq_ops = __cam_sbi_ctx_handle_irq_in_activated,
	},
	/* Available */
	{
		.ioctl_ops = {
			.acquire_dev = __cam_sbi_ctx_acquire_dev_in_available,
		},
		.crm_ops = {
			.link = __cam_sbi_ctx_link________in_acquired_,
			.unlink = __cam_sbi_ctx_unlink_in_acquired,
		},
		.irq_ops = __cam_sbi_ctx_handle_irq_in_activated,
	},
	/* Acquired */
	{
		.ioctl_ops = {
			.acquire_hw = __cam_sbi_ctx_acquire_hw_in_acquired,
			.config_dev = __cam_sbi_ctx_config__dev_in_acquired_,
			.release_dev = __cam_sbi_ctx_release_dev_in_acquired_,
			.release_hw = __cam_sbi_ctx_release_hw__in_top_state,
		},
		.crm_ops = {
			.link = __cam_sbi_ctx_link________in_acquired_,
			.unlink = __cam_sbi_ctx_unlink_in_acquired,
			.get_dev_info = __cam_sbi_ctx_get_dev_info_in_acquired,
		},
		.irq_ops = __cam_sbi_ctx_handle_irq_in_activated,
	},
	/* Ready */
	{
		.ioctl_ops = {
			.acquire_hw = __cam_sbi_ctx_acquire_hw_in_acquired,
			.config_dev = __cam_sbi_ctx_config_dev_in_top_state,
			.release_dev = __cam_sbi_ctx_release_dev_in_acquired_,
			.release_hw = __cam_sbi_ctx_release_hw__in_top_state,
			.start_dev = __cam_sbi_ctx_start___dev_in_ready____,
		},
		.crm_ops = {
			.link = __cam_sbi_ctx_link________in_acquired_,
			.unlink = __cam_sbi_ctx_unlink_in_acquired,
			//.flush_req = __cam_sbi_ctx_flush_req_in_ready,
			.apply_req = __cam_sbi_ctx_apply_req,
		},
		.irq_ops = __cam_sbi_ctx_handle_irq_in_activated,
	},
	/* Flushed */
	{},
	/* Activate */
	{
		.ioctl_ops = {
			.stop_dev = __cam_sbi_ctx_stop____dev_in_activated,
			.release_dev = __cam_sbi_ctx_release_dev_in_activated,
			.config_dev = __cam_sbi_ctx_config_dev_in_top_state,
			.flush_dev = __cam_sbi_ctx_flush_dev_in_activated,
			.release_hw =
				__cam_sbi_ctx_release_hw_in_activated_state,
		},
		.crm_ops = {
			.link = __cam_sbi_ctx_link________in_acquired_,
			.unlink = __cam_sbi_ctx_unlink_in_acquired,
			.apply_req = __cam_sbi_ctx_apply_req,
			.flush_req = __cam_sbi_ctx_flush_req_in_top_state,
			//.process_evt = __cam_sbi_ctx_process_evt,
		},
		.irq_ops = __cam_sbi_ctx_handle_irq_in_activated,
	},
};

int cam_sbi_context_init(struct cam_sbi_dev_context *sbi_ctx,
	struct cam_context *ctx,
	struct cam_req_mgr_kmd_ops *crm_node_intf,
	struct cam_hw_mgr_intf *hw_intf,
	uint32_t index)
{
	int rc = 0;
	int i = 0;

	if (!ctx || !sbi_ctx) {
		CAM_ERR(CAM_SBI, "Invalid input");
		return -EINVAL;
	}

	CAM_DBG(CAM_SBI, "enter..ctx_id %d", ctx->ctx_id);

	memset(sbi_ctx, 0, sizeof(*sbi_ctx));

	for (i = 0; i < CAM_CTX_REQ_MAX; i++) {
		sbi_ctx->req_base[i].req_priv = &sbi_ctx->req_sbi[i];
		sbi_ctx->req_sbi[i].base = &sbi_ctx->req_base[i];
	}

	rc = cam_context_init(ctx, sbi_dev_name, CAM_SBI, index,
		crm_node_intf, hw_intf, sbi_ctx->req_base, CAM_CTX_REQ_MAX);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to init context");
		return rc;
	}
	sbi_ctx->base = ctx;
	sbi_ctx->index = index;
	ctx->ctx_priv = sbi_ctx;
	sbi_ctx->frame_id = 0;
	sbi_ctx->active_req_cnt = 0;
	ctx->state_machine = cam_sbi_ctx_state_machine;

	return rc;
}

int cam_sbi_context_deinit(struct cam_sbi_dev_context *sbi_ctx)
{
	int rc = 0;

	CAM_DBG(CAM_SBI, "Enter");

	if (!sbi_ctx) {
		CAM_ERR(CAM_SBI, "No ctx to deinit");
		return -EINVAL;
	}

	rc = cam_context_deinit(sbi_ctx->base);

	memset(sbi_ctx, 0, sizeof(*sbi_ctx));
	return rc;
}
