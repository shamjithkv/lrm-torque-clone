/*
*         OpenPBS (Portable Batch System) v2.3 Software License
*
* Copyright (c) 1999-2000 Veridian Information Solutions, Inc.
* All rights reserved.
*
* ---------------------------------------------------------------------------
* For a license to use or redistribute the OpenPBS software under conditions
* other than those described below, or to purchase support for this software,
* please contact Veridian Systems, PBS Products Department ("Licensor") at:
*
*    www.OpenPBS.org  +1 650 967-4675                  sales@OpenPBS.org
*                        877 902-4PBS (US toll-free)
* ---------------------------------------------------------------------------
*
* This license covers use of the OpenPBS v2.3 software (the "Software") at
* your site or location, and, for certain users, redistribution of the
* Software to other sites and locations.  Use and redistribution of
* OpenPBS v2.3 in source and binary forms, with or without modification,
* are permitted provided that all of the following conditions are met.
* After December 31, 2001, only conditions 3-6 must be met:
*
* 1. Commercial and/or non-commercial use of the Software is permitted
*    provided a current software registration is on file at www.OpenPBS.org.
*    If use of this software contributes to a publication, product, or
*    service, proper attribution must be given; see www.OpenPBS.org/credit.html
*
* 2. Redistribution in any form is only permitted for non-commercial,
*    non-profit purposes.  There can be no charge for the Software or any
*    software incorporating the Software.  Further, there can be no
*    expectation of revenue generated as a consequence of redistributing
*    the Software.
*
* 3. Any Redistribution of source code must retain the above copyright notice
*    and the acknowledgment contained in paragraph 6, this list of conditions
*    and the disclaimer contained in paragraph 7.
*
* 4. Any Redistribution in binary form must reproduce the above copyright
*    notice and the acknowledgment contained in paragraph 6, this list of
*    conditions and the disclaimer contained in paragraph 7 in the
*    documentation and/or other materials provided with the distribution.
*
* 5. Redistributions in any form must be accompanied by information on how to
*    obtain complete source code for the OpenPBS software and any
*    modifications and/or additions to the OpenPBS software.  The source code
*    must either be included in the distribution or be available for no more
*    than the cost of distribution plus a nominal fee, and all modifications
*    and additions to the Software must be freely redistributable by any party
*    (including Licensor) without restriction.
*
* 6. All advertising materials mentioning features or use of the Software must
*    display the following acknowledgment:
*
*     "This product includes software developed by NASA Ames Research Center,
*     Lawrence Livermore National Laboratory, and Veridian Information
*     Solutions, Inc.
*     Visit www.OpenPBS.org for OpenPBS software support,
*     products, and information."
*
* 7. DISCLAIMER OF WARRANTY
*
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT
* ARE EXPRESSLY DISCLAIMED.
*
* IN NO EVENT SHALL VERIDIAN CORPORATION, ITS AFFILIATED COMPANIES, OR THE
* U.S. GOVERNMENT OR ANY OF ITS AGENCIES BE LIABLE FOR ANY DIRECT OR INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This license will be governed by the laws of the Commonwealth of Virginia,
* without reference to its choice of law rules.
*/
#include <pbs_config.h>   /* the master config generated by configure */

#include <sys/types.h>
#include <stdio.h>
#include "libpbs.h"
#include "string.h"
#include "list_link.h"
#include "attribute.h"
#include "server_limits.h"
#include "server.h"
#include "pbs_job.h"
#include "credential.h"
#include "batch_request.h"
#include "pbs_error.h"
#include "log.h"
#include "svrfunc.h"
#include "mcom.h"

/* Global Data */

extern struct credential conn_credent[PBS_NET_MAX_CONNECTIONS];
extern char *pbs_o_host;
extern char  server_host[];
extern char *msg_permlog;
extern time_t time_now;

extern char *PJobState[];

extern int site_allow_u(char *, char *);

int svr_chk_owner_generic(struct batch_request *preq, char *owner, char *submit_host);
int svr_authorize_req(struct batch_request *preq, char *owner, char *submit_host);


/*
 * svr_chk_owner - compare a user name from a request and the name of
 * the user who owns the job.
 *
 * Return 0 if same, non 0 if user is not the job owner
 */


int svr_chk_owner(

  struct batch_request *preq,  /* I */
  job                  *pjob)  /* I */

  {
  char  owner[PBS_MAXUSER + 1];

  get_jobowner(pjob->ji_wattr[(int)JOB_ATR_job_owner].at_val.at_str, owner);


  return svr_chk_owner_generic(preq, owner, get_variable(pjob, pbs_o_host));
  }  /* END svr_chk_owner() */


int svr_chk_owner_generic(

  struct batch_request *preq,
  char *owner,
  char *submit_host)

  {

  char *pu;
  char  rmtuser[PBS_MAXUSER + 1];

  /* map user@host to "local" name */

  pu = site_map_user(preq->rq_user, preq->rq_host);

  if (pu == NULL)
    {
    /* FAILURE */

    return(-1);
    }

  strncpy(rmtuser, pu, PBS_MAXUSER);

  rmtuser[PBS_MAXUSER] = '\0';


  pu = site_map_user(owner, submit_host);

  return(strcmp(rmtuser, pu));
  }


/*
 * svr_authorize_jobreq - determine if requestor is authorized to make
 * request against the job.  This is only called for batch requests
 * against jobs, not manager requests against queues or the server.
 *
 *  Returns 0 if authorized (job owner, operator, administrator)
 *   -1 if not authorized.
 */

int svr_authorize_jobreq(

  struct batch_request *preq,  /* I */
  job                  *pjob)  /* I */

  {
  char  owner[PBS_MAXUSER + 1];

  get_jobowner(pjob->ji_wattr[(int)JOB_ATR_job_owner].at_val.at_str, owner);

  return svr_authorize_req(preq, owner, get_variable(pjob, pbs_o_host));
  }  /* END svr_authorize_jobreq() */






int svr_authorize_req(

  struct batch_request *preq,
  char *owner,
  char *submit_host)

  {
  /* does requestor have special privileges? */

  if ((preq->rq_perm & (ATR_DFLAG_OPRD | ATR_DFLAG_OPWR |
                        ATR_DFLAG_MGRD | ATR_DFLAG_MGWR)) != 0)
    {
    /* request authorized */

    return(0);
    }

  /* is requestor the job owner? */

  if (svr_chk_owner_generic(preq, owner, submit_host) == 0)
    {
    /* request authorized */

    return(0);
    }

  /* not authorized */

  return(-1);
  }





/*
 * svr_get_privilege - get privilege level of a user.
 *
 * Privilege is granted to a user at a host.  A user is automatically
 * granted "user" privilege.  The user@host pair must appear in
 * the server's administrator attribute list to be granted "manager"
 * privilege and/or appear in the operators attribute list to be
 * granted "operator" privilege.  If either acl is unset, then root
 * on the server machine is granted that privilege.
 *
 * If "PBS_ROOT_ALWAYS_ADMIN" is defined, then root always has privilege
 * even if not in the list.
 *
 * The returns are based on the access permissions of attributes, see
 * attribute.h.
 */

int svr_get_privilege(

  char *user,  /* I */
  char *host)  /* I */

  {
  char  id[] = "svr_get_privilege";
  int   is_root = 0;
  int   priv = (ATR_DFLAG_USRD | ATR_DFLAG_USWR);
  int   num_host_chars;
  char  uh[PBS_MAXUSER + PBS_MAXHOSTNAME + 2];
  char  host_no_port[PBS_MAXHOSTNAME+1];
  char *colon_loc = NULL;

  if(!user)
    {

    sprintf(log_buffer, "Invalid user: %s", "null");

    log_record(
      PBSEVENT_SECURITY,
      PBS_EVENTCLASS_SERVER,
      id,
      log_buffer);


    return(0);
    }

  /* user name cannot be longer than PBS_MAXUSER*/
  if(strlen(user) > PBS_MAXUSER)
    {
    sprintf(log_buffer, "Invalid user: %s", user);

    log_record(
      PBSEVENT_SECURITY,
      PBS_EVENTCLASS_SERVER,
      id,
      log_buffer);
     
    return(0);
    }

  if(!host)
    return(0);

  colon_loc = strchr(host, ':');

  /* if the request host has port information in it, we want to strip it out */

  if (colon_loc == NULL) 
    {
    /* no colon found */
    num_host_chars = strlen(host);

    sprintf(host_no_port, "%s", host);
    }
  else
    {
    num_host_chars = colon_loc - host;

    /* actually remove the colon for host_no_port */
    *colon_loc = '\0';
    sprintf(host_no_port,"%s",host);
    *colon_loc = ':';
    }

  /* num_host_chars cannot be more than PBS_MAXHOSTNAME */
  if (num_host_chars > PBS_MAXHOSTNAME)
    {
    snprintf(log_buffer, LOG_BUF_SIZE, "Invalid host: %s", host);
    /* log_buffer is big but just in case host was really long we need to
       null terminate the last byte. strncpy will copy LOG_BUF_SIZE but
       not null terminate the array if host is larger. */
    log_buffer[LOG_BUF_SIZE - 1] = 0;

    log_record(
      PBSEVENT_SECURITY,
      PBS_EVENTCLASS_SERVER,
      id,
      log_buffer);

    return(0);
    }

  sprintf(uh, "%s@%s", user, host);

  /* NOTE:  enable case insensitive host check (CRI) */

#ifdef __CYGWIN__
  if (IamAdminByName(user) && !strcasecmp(host_no_port, server_host))
    {
    is_root = 1;
    return(priv | ATR_DFLAG_MGRD | ATR_DFLAG_MGWR | ATR_DFLAG_OPRD | ATR_DFLAG_OPWR);
    }
#else /* __CYGWIN__ */

  if ((strcmp(user, PBS_DEFAULT_ADMIN) == 0) &&
      !strcasecmp(host_no_port, server_host))
    {
    is_root = 1;

#ifdef PBS_ROOT_ALWAYS_ADMIN
    return(priv | ATR_DFLAG_MGRD | ATR_DFLAG_MGWR | ATR_DFLAG_OPRD | ATR_DFLAG_OPWR);
#endif
    }
#endif /* __CYGWIN__ */

  if (!(server.sv_attr[(int)SRV_ATR_managers].at_flags & ATR_VFLAG_SET))
    {
    if (is_root)
      priv |= (ATR_DFLAG_MGRD | ATR_DFLAG_MGWR);
    }
  else if (acl_check(&server.sv_attr[SRV_ATR_managers], uh, ACL_User))
    {
    priv |= (ATR_DFLAG_MGRD | ATR_DFLAG_MGWR);
    }

  if (!(server.sv_attr[(int)SRV_ATR_operators].at_flags & ATR_VFLAG_SET))
    {
    if (is_root)
      priv |= (ATR_DFLAG_OPRD | ATR_DFLAG_OPWR);
    }
  else if (acl_check(&server.sv_attr[SRV_ATR_operators], uh, ACL_User))
    {
    priv |= (ATR_DFLAG_OPRD | ATR_DFLAG_OPWR);
    }

  return(priv);
  }  /* END svr_get_privilege() */





/*
 * authenticate_user - authenticate user by checking name against credential
 *         provided on connection via Authenticate User request.
 *
 * Returns 0 if user is who s/he claims, non-zero error code if not
 */

int authenticate_user(

  struct batch_request *preq,  /* I */
  struct credential    *pcred) /* I */

  {
  int  rc;
  char uath[PBS_MAXUSER + PBS_MAXHOSTNAME + 1];

  if (strncmp(preq->rq_user, pcred->username, PBS_MAXUSER))
    {
    return(PBSE_BADCRED);
    }

  if (strncmp(preq->rq_host, pcred->hostname, PBS_MAXHOSTNAME))
    {
    return(PBSE_BADCRED);
    }

  if (pcred->timestamp)
    {
    long lifetime;
    if ((server.sv_attr[SRV_ATR_CredentialLifetime].at_flags & ATR_VFLAG_SET) != 0)
      {
      /* use configured value if set */
      lifetime = server.sv_attr[SRV_ATR_CredentialLifetime].at_val.at_long;
      }
    else 
      {
      /* if not use the default */
      lifetime = CREDENTIAL_LIFETIME;
      }

    /* negative values mean that credentials have an infinite lifetime */
    if (lifetime > -1)
      {
      if ((pcred->timestamp - CREDENTIAL_TIME_DELTA > time_now) ||
          (pcred->timestamp + lifetime < time_now))
        {
        return(PBSE_EXPIRED);
        }
      }

    }

  /* If Server's Acl_User enabled, check if user in list */

  if (server.sv_attr[SRV_ATR_AclUserEnabled].at_val.at_long)
    {
    strcpy(uath, preq->rq_user);
    strcat(uath, "@");
    strcat(uath, preq->rq_host);

    if (acl_check(&server.sv_attr[SRV_ATR_AclUsers], uath, ACL_User) == 0)
      {

#ifdef __CYGWIN__
  if (!IamAdminByName(preq->rq_user) || (strcasecmp(preq->rq_host, server_host) != 0))
    {
	return(PBSE_PERM);
    }
#else /* __CYGWIN__ */
#ifdef PBS_ROOT_ALWAYS_ADMIN

      if ((strcmp(preq->rq_user, PBS_DEFAULT_ADMIN) != 0) ||
          (strcasecmp(preq->rq_host, server_host) != 0))
        {
        return(PBSE_PERM);
        }

#else /* PBS_ROOT_ALWAYS_ADMIN */
      return(PBSE_PERM);

#endif /* PBS_ROOT_ALWAYS_ADMIN */
#endif /* __CYGWIN__ */

      }
    }

  /* A site stub for additional checking */

  rc = site_allow_u(preq->rq_user, preq->rq_host);

  return(rc);
  }  /* END authenticate_user() */





/*
 * chk_job_request - check legality of a request against a job
 *
 * this checks the most conditions common to most job batch requests.
 * It also returns a pointer to the job if found and the tests pass.
 */


job *chk_job_request(

  char                 *jobid,  /* I */
  struct batch_request *preq)   /* I */

  {
  job *pjob;

  if ((pjob = find_job(jobid)) == NULL)
    {
    log_event(
      PBSEVENT_DEBUG,
      PBS_EVENTCLASS_JOB,
      jobid,
      pbse_to_txt(PBSE_UNKJOBID));

    req_reject(PBSE_UNKJOBID, 0, preq, NULL, "cannot locate job");

    return(NULL);
    }

  if (svr_authorize_jobreq(preq, pjob) == -1)
    {
    sprintf(log_buffer, msg_permlog,
            preq->rq_type,
            "Job",
            pjob->ji_qs.ji_jobid,
            preq->rq_user,
            preq->rq_host);

    log_event(
      PBSEVENT_SECURITY,
      PBS_EVENTCLASS_JOB,
      pjob->ji_qs.ji_jobid,
      log_buffer);

    req_reject(PBSE_PERM, 0, preq, NULL, "operation not permitted");

    return(NULL);
    }

  if (pjob->ji_qs.ji_state >= JOB_STATE_EXITING)
    {
    /* job has completed */

    switch (preq->rq_type)
      {

      case PBS_BATCH_Rerun:

        /* allow re-run to be executed for completed jobs */

        /* NO-OP */

        break;

      default:

        {
        char tmpLine[1024];

        sprintf(log_buffer, "%s %s",
                pbse_to_txt(PBSE_BADSTATE),
                PJobState[pjob->ji_qs.ji_state]);

        log_event(
          PBSEVENT_DEBUG,
          PBS_EVENTCLASS_JOB,
          pjob->ji_qs.ji_jobid,
          log_buffer);

        sprintf(tmpLine, "invalid state for job - %s",
                PJobState[pjob->ji_qs.ji_state]);

        req_reject(PBSE_BADSTATE, 0, preq, NULL, tmpLine);

        return(NULL);
        }

      /*NOTREACHED*/

      break;
      }  /* END switch (preq->rq_type) */
    }    /* END if (pjob->ji_qs.ji_state >= JOB_STATE_EXITING) */

  /* SUCCESS - request is valid */

  return(pjob);
  }  /* END chk_job_request() */



