/*
 * linux/include/linux/sunrpc/svc_xprt.h
 *
 * RPC server transport I/O
 */

#ifndef SUNRPC_SVC_XPRT_H
#define SUNRPC_SVC_XPRT_H

#include <linux/sunrpc/svc.h>

struct svc_xprt_ops {
	struct svc_xprt	*(*xpo_create)(struct svc_serv *,
				       struct sockaddr *, int,
				       int);
	struct svc_xprt	*(*xpo_accept)(struct svc_xprt *);
	int		(*xpo_has_wspace)(struct svc_xprt *);
	int		(*xpo_recvfrom)(struct svc_rqst *);
	void		(*xpo_prep_reply_hdr)(struct svc_rqst *);
	int		(*xpo_sendto)(struct svc_rqst *);
	void		(*xpo_release_rqst)(struct svc_rqst *);
	void		(*xpo_detach)(struct svc_xprt *);
	void		(*xpo_free)(struct svc_xprt *);
};

struct svc_xprt_class {
	const char		*xcl_name;
	struct module		*xcl_owner;
	struct svc_xprt_ops	*xcl_ops;
	struct list_head	xcl_list;
	u32			xcl_max_payload;
};

struct svc_xprt {
	struct svc_xprt_class	*xpt_class;
	struct svc_xprt_ops	*xpt_ops;
};

int	svc_reg_xprt_class(struct svc_xprt_class *);
void	svc_unreg_xprt_class(struct svc_xprt_class *);
void	svc_xprt_init(struct svc_xprt_class *, struct svc_xprt *);
int	svc_create_xprt(struct svc_serv *, char *, unsigned short, int);

#endif /* SUNRPC_SVC_XPRT_H */
