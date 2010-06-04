/*
 * $Id$
 * $Source$
 *
 * Copyright 2010 Brad Lanam, Walnut Creek, CA
 */

#include "config.h"
#include "di.h"

#include <stdio.h>
#if _hdr_stdlib
# include <stdlib.h>
#endif
#if _hdr_unistd
# include <unistd.h>
#endif
#if _hdr_string
# include <string.h>
#endif
#if _hdr_strings
# include <strings.h>
#endif
#if _sys_types
# include <sys/types.h>
#endif
#if _hdr_errno
# include <errno.h>
#endif
#if _sys_quota
# include <sys/quota.h>
#endif
#if _sys_fs_ufs_quota
# include <sys/fs/ufs_quota.h>
#endif
#if _hdr_ufs_quota
# include <ufs/quota.h>
#endif
#if _hdr_ufs_ufs_quota
# include <ufs/ufs/quota.h>
#endif
#if _hdr_linux_dqblk_xfs
# include <linux/dqblk_xfs.h>
#endif
  /* AIX doesn't seem to have quotactl declared.... */
  /* use their compatibility routine.               */
#if _hdr_linux_quota && _inc_conflict__sys_quota__hdr_linux_quota
# include <linux/quota.h>
#endif
#if _hdr_rpc_rpc
# include <rpc/rpc.h>
#endif
#if _hdr_rpcsvc_rquota
# include <rpcsvc/rquota.h>
#endif

#if defined(__cplusplus)
  extern "C" {
#endif

#if _hdr_rpc_rpc && _hdr_rpcsvc_rquota
static bool_t xdr_quota_get _((XDR *, getquota_args *));
static bool_t xdr_quota_rslt _((XDR *, getquota_rslt *));
static void diquota_nfs _((diQuota_t *));
#endif
#if _enable_quotas
static void di_process_quotas _((diQuota_t *, int, int, char *));
#endif

extern int debug;

#if defined(__cplusplus)
  }
#endif

#ifdef BLOCK_SIZE           /* linux */
# define DI_QUOT_BLOCK_SIZE BLOCK_SIZE
#else
# ifdef DEV_BSIZE           /* tru64, et. al. */
#  define DI_QUOT_BLOCK_SIZE DEV_BSIZE
# else
#  define DI_QUOT_BLOCK_SIZE 512
# endif
#endif

  /* rename certain structure members for portability */
#if _mem_dqb_fsoftlimit_dqblk
# define dqb_isoftlimit dqb_fsoftlimit
#endif
#if _mem_dqb_fhardlimit_dqblk
# define dqb_ihardlimit dqb_fhardlimit
#endif
#if _mem_dqb_curfiles_dqblk
# define dqb_curinodes dqb_curfiles
#endif

 /* AIX has the linux compatibility call, */
 /* but it still requires a path rather   */
 /* than the special device name          */
#if _AIX
# define special name
#endif

void
#if _proto_stdc
diquota (diQuota_t *diqinfo)
#else
diquota (diqinfo)
  diQuota_t     *diqinfo;
#endif
{
  int               rc;
  int               xfsflag;
#if _typ_struct_dqblk
  struct dqblk      qinfo;
#endif
#if _typ_fs_disk_quota_t
  fs_disk_quota_t   xfsqinfo;
#endif
#if _enable_quotas
  int               ucmd;
  int               gcmd;
  char              *qiptr;
#endif

  if (debug > 5) {
    printf ("quota: diquota\n");
  }
  rc = -1;
  xfsflag = FALSE;

  diqinfo->limit = 0;
  diqinfo->used = 0;
  diqinfo->ilimit = 0;
  diqinfo->iused = 0;

#if _enable_quotas
  if (strncmp (diqinfo->type, "nfs", 3) == 0 &&
      strcmp (diqinfo->type, "nfsd") != 0) {
# if _hdr_rpc_rpc && _hdr_rpcsvc_rquota
    diquota_nfs (diqinfo);
# endif
    return;
  }
  if (strcmp (diqinfo->type, "xfs") == 0) {
# if _hdr_linux_dqblk_xfs
    ucmd = QCMD (Q_XGETQUOTA, USRQUOTA);
    gcmd = QCMD (Q_XGETQUOTA, GRPQUOTA);
    qiptr = (char *) &xfsqinfo;
    xfsflag = TRUE;
  } else {
# endif
# ifdef QCMD
    ucmd = QCMD (Q_GETQUOTA, USRQUOTA);
    gcmd = QCMD (Q_GETQUOTA, GRPQUOTA);
#endif
    qiptr = (char *) &qinfo;
  }

# if _lib_quotactl && _quotactl_pos == 1
  rc = quotactl (diqinfo->name, ucmd,
        (int) diqinfo->uid, (caddr_t) qiptr);
# endif
# if _lib_quotactl && _quotactl_pos == 2
  rc = quotactl (ucmd, diqinfo->special,
        (int) diqinfo->uid, (caddr_t) qiptr);
# endif
# if _sys_fs_ufs_quota        /* Solaris */
  {
    int             fd;
    struct quotctl  qop;

    qop.op = Q_GETQUOTA;
    qop.uid = diqinfo->uid;
    qop.addr = (caddr_t) qiptr;
    fd = open (strcat (diqinfo->name, "/quotas"), O_RDONLY | O_NOCTTY);
    if (fd >= 0) {
      rc = ioctl (fd, Q_QUOTACTL, &qop);
      close (fd);
    } else {
      rc = fd;
    }
  }
# endif  /* _sys_fs_ufs_quota */

  di_process_quotas (diqinfo, rc, xfsflag, qiptr);

  if (rc == 0 || errno != ESRCH) {
#  if _lib_quotactl && _quotactl_pos == 1
    rc = quotactl (diqinfo->name, gcmd,
          (int) diqinfo->gid, (caddr_t) qiptr);
#  endif
#  if _lib_quotactl && _quotactl_pos == 2
    rc = quotactl (gcmd, diqinfo->special,
             (int) diqinfo->gid, (caddr_t) qiptr);
#  endif

    di_process_quotas (diqinfo, rc, xfsflag, qiptr);
  }
#endif /* _enable_quotas */
}

#if _hdr_rpc_rpc && _hdr_rpcsvc_rquota

static bool_t
# if _proto_stdc
xdr_quota_get (XDR *xp, getquota_args *args)
# else
xdr_quota_get (xp, args)
  XDR           *xp;
  getquota_args *args;
# endif
{
  if (debug > 5) {
    printf ("quota: xdr_quota_get\n");
  }

  if (! xdr_string (xp, &args->gqa_pathp, RQ_PATHLEN)) {
    return 0;
  }
  if (! xdr_int (xp, &args->gqa_uid)) {
    return 0;
  }
  return 1;
}

static bool_t
# if _proto_stdc
xdr_quota_rslt (XDR *xp, getquota_rslt *rslt)
# else
xdr_quota_rslt (xp, rslt)
  XDR           *xp;
  getquota_rslt *rslt;
# endif
{
  int       quotastat;
  rquota    *rptr;

  if (debug > 5) {
    printf ("quota: xdr_quota_rslt\n");
  }

  if (! xdr_int (xp, &quotastat)) {
    return 0;
  }
  rslt->status = quotastat;
  rptr = &rslt->getquota_rslt_u.gqr_rquota;

  if (! xdr_bool (xp, &rptr->rq_active)) {
    return 0;
  }
  if (! xdr_int (xp, &rptr->rq_bsize)) {
    return 0;
  }
  if (! xdr_u_int (xp, &rptr->rq_bhardlimit)) {
    return 0;
  }
  if (! xdr_u_int (xp, &rptr->rq_bsoftlimit)) {
    return 0;
  }
  if (! xdr_u_int (xp, &rptr->rq_curblocks)) {
    return 0;
  }
  if (! xdr_u_int (xp, &rptr->rq_fhardlimit)) {
    return 0;
  }
  if (! xdr_u_int (xp, &rptr->rq_fsoftlimit)) {
    return 0;
  }
  if (! xdr_u_int (xp, &rptr->rq_curfiles)) {
    return 0;
  }
  return (1);
}

static void
# if _proto_stdc
diquota_nfs (diQuota_t *diqinfo)
# else
diquota_nfs (diqinfo)
  diQuota_t     *diqinfo;
# endif
{
    CLIENT          *rqclnt;
    enum clnt_stat  clnt_stat;
    struct timeval  timeout = {2, 0};
    char            host [DI_SPEC_NAME_LEN];
    char            *ptr;
    char            *path;
    getquota_args   args;
    getquota_rslt   result;
    rquota          *rptr;
    _fs_size_t      tsize;
    _fs_size_t      quotBlockSize = { DI_QUOT_BLOCK_SIZE };


    if (debug > 5) {
      printf ("quota: diquota_nfs\n");
    }

    strcpy (host, diqinfo->special);
    path = host;
    ptr = index (host, ':');
    if (ptr != (char *) NULL) {
      *ptr = '\0';
      path = ptr + 1;
    }
    if (debug > 2) {
      printf ("quota: nfs: host: %s path: %s\n", host, path);
    }
	args.gqa_pathp = path;
	args.gqa_uid = (int) diqinfo->uid;

    rqclnt = clnt_create (host, RQUOTAPROG, RQUOTAVERS, "udp");
    if (rqclnt == (CLIENT *) NULL) {
      if (debug > 2) {
        printf ("quota: nfs: create failed %d\n", errno);
      }
      return;
    }
    rqclnt->cl_auth = authunix_create_default();
    clnt_stat = clnt_call (rqclnt, RQUOTAPROC_GETQUOTA,
        (xdrproc_t) xdr_quota_get, (caddr_t) &args,
        (xdrproc_t) xdr_quota_rslt, (caddr_t) &result, timeout);
    if (clnt_stat != RPC_SUCCESS) {
      if (debug > 2) {
        printf ("quota: nfs: not success\n");
      }
      if (rqclnt->cl_auth) {
        auth_destroy (rqclnt->cl_auth);
      }
      clnt_destroy (rqclnt);
      return;
    }

    if (result.status == 1) {
      if (debug > 2) {
        printf ("quota: nfs: status 1\n");
      }
      rptr = &result.getquota_rslt_u.gqr_rquota;

      /* diqinfo->blockSize is something too large to use */
      /* reset it.                                        */
      diqinfo->blockSize = 1024;

      diqinfo->used = rptr->rq_curblocks *
            quotBlockSize / diqinfo->blockSize;
      diqinfo->limit = rptr->rq_bhardlimit *
            quotBlockSize / diqinfo->blockSize;
      tsize = rptr->rq_bsoftlimit *
              quotBlockSize / diqinfo->blockSize;
      if (tsize != 0 && tsize < diqinfo->limit) {
        diqinfo->limit = tsize;
      }
      diqinfo->iused = rptr->rq_curfiles;
      diqinfo->ilimit = rptr->rq_fhardlimit;
      if (rptr->rq_fsoftlimit < diqinfo->ilimit) {
        diqinfo->ilimit = rptr->rq_fsoftlimit;
      }
    }

	if (rqclnt) {
      if (rqclnt->cl_auth) {
        auth_destroy (rqclnt->cl_auth);
      }
      clnt_destroy (rqclnt);
	}
}
#endif  /* have rpc headers */

#if _enable_quotas
static void
# if _proto_stdc
di_process_quotas (diQuota_t *diqinfo, int rc, int xfsflag, char *cqinfo)
# else
di_process_quotas (diqinfo, rc, xfsflag, cqinfo)
  diQuota_t     *diqinfo;
  int           rc;
  int           xfsflag;
  char          *cqinfo;
# endif
{
  _fs_size_t        quotBlockSize = { DI_QUOT_BLOCK_SIZE };
  _fs_size_t        tsize;
  struct dqblk      *qinfo;
# if _typ_fs_disk_quota_t
  fs_disk_quota_t   *xfsqinfo;
# endif

  qinfo = (struct dqblk *) cqinfo;
  if (xfsflag) {
# if _typ_fs_disk_quota_t
    xfsqinfo = (fs_disk_quota_t *) cqinfo;
# endif
    quotBlockSize = 512;
  }

  if (rc == 0) {
# if _mem_dqb_curspace_dqblk
    tsize = qinfo->dqb_curspace / diqinfo->blockSize;
    if (tsize > diqinfo->used || diqinfo->used == 0) {
      diqinfo->used = tsize;
    }
# endif
# if _mem_dqb_curblocks_dqblk
    if (xfsflag) {
#  if _typ_fs_disk_quota_t
      tsize = xfsqinfo->d_bcount;
#  endif
      ;
    } else {
      tsize = qinfo->dqb_curblocks;
    }
    tsize = tsize * quotBlockSize / diqinfo->blockSize;
    if (tsize > diqinfo->used || diqinfo->used == 0) {
      diqinfo->used = tsize;
    }
# endif

    if (xfsflag) {
# if _typ_fs_disk_quota_t
      tsize = xfsqinfo->d_blk_hardlimit;
# endif
      ;
    } else {
      tsize = qinfo->dqb_bhardlimit;
    }
    if (debug > 2) {
      printf ("quota: orig hard: %lld\n", tsize);
    }
    tsize = tsize * quotBlockSize / diqinfo->blockSize;
    if (tsize != 0 && (tsize < diqinfo->limit || diqinfo->limit == 0)) {
      diqinfo->limit = tsize;
    }

    if (xfsflag) {
# if _typ_fs_disk_quota_t
      tsize = xfsqinfo->d_blk_softlimit;
# endif
      ;
    } else {
      tsize = qinfo->dqb_bsoftlimit;
    }
    if (debug > 2) {
      printf ("quota: orig soft: %lld\n", tsize);
    }
    tsize = tsize * quotBlockSize / diqinfo->blockSize;
    if (tsize != 0 && (tsize < diqinfo->limit || diqinfo->limit == 0)) {
      if (debug > 2) {
        printf ("quota: using soft: %lld\n", tsize);
      }
      diqinfo->limit = tsize;
    }

    if (debug > 2) {
      printf ("quota: %s used: %lld limit: %lld\n", diqinfo->name,
          diqinfo->used, diqinfo->limit);
    }

    if (xfsflag) {
# if _typ_fs_disk_quota_t
      tsize = xfsqinfo->d_icount;
# endif
      ;
    } else {
      tsize = qinfo->dqb_curinodes;
    }
    if (tsize > diqinfo->iused || diqinfo->iused == 0) {
      diqinfo->iused = tsize;
    }

    if (xfsflag) {
# if _typ_fs_disk_quota_t
      tsize = xfsqinfo->d_ino_hardlimit;
# endif
      ;
    } else {
      tsize = qinfo->dqb_ihardlimit;
    }
    if (tsize != 0 && tsize < (diqinfo->ilimit || diqinfo->ilimit == 0)) {
      diqinfo->ilimit = tsize;
    }

    if (xfsflag) {
# if _typ_fs_disk_quota_t
      tsize = xfsqinfo->d_ino_softlimit;
# endif
      ;
    } else {
      tsize = qinfo->dqb_isoftlimit;
    }
    if (tsize != 0 && tsize < (diqinfo->ilimit || diqinfo->ilimit == 0)) {
      diqinfo->ilimit = tsize;
    }
  } else {
    if (debug > 2) {
      printf ("quota: %s errno %d\n", diqinfo->name, errno);
    }
  }
}
#endif /* _enable_quotas */
