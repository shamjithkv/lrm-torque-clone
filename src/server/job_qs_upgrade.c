/* job_qs_upgrade.c -
 *
 * The following public functions are provided:
 *  job_qs_upgrade()   -
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "pbs_ifl.h"
#include "list_link.h"
#include "attribute.h"
#include "server_limits.h"
#include "pbs_job.h"
#include "array.h"

extern char *path_jobs;
extern char *path_arrays;


int upgrade_2_1_X(job *pj, int fds);
int upgrade_2_2_X(job *pj, int fds);
int upgrade_2_3_X(job *pj, int fds);

/* switched to new naming scheme */
int upgrade_v1(job *pj, int fds);



typedef struct
  {
  int     ji_state;  /* internal copy of state */
  int     ji_substate; /* job sub-state */
  int     ji_svrflags; /* server flags */
  int     ji_numattr;  /* number of attributes in list */
  int     ji_ordering; /* special scheduling ordering */
  int     ji_priority; /* internal priority */
  time_t  ji_stime;  /* time job started execution */
  char    ji_jobid[80];   /* job identifier */
  char    ji_fileprefix[12];  /* job file prefix */
  char    ji_queue[16];  /* name of current queue */
  char    ji_destin[87]; /* dest from qmove/route */
  int     ji_un_type;  /* type of ji_un union */
  union   /* depends on type of queue currently in */
    {
    struct   /* if in execution queue .. */
      {
      pbs_net_t ji_momaddr;  /* host addr of Server */
      int       ji_exitstat; /* job exit status from MOM */
      } ji_exect;

    struct
      {
      time_t  ji_quetime;        /* time entered queue */
      time_t  ji_rteretry;       /* route retry time */
      } ji_routet;

    struct
      {
      pbs_net_t  ji_fromaddr;     /* host job coming from   */
      int        ji_fromsock; /* socket job coming over */
      int        ji_scriptsz; /* script size */
      } ji_newt;

    struct
      {
      pbs_net_t ji_svraddr;  /* host addr of Server */
      int       ji_exitstat; /* job exit status from MOM */
      uid_t     ji_exuid;    /* execution uid */
      gid_t     ji_exgid;    /* execution gid */
      } ji_momt;
    } ji_un;
  } ji_qs_2_1_X;

typedef struct
  {
  int     qs_version;
  int     ji_state;           /* internal copy of state */
  int     ji_substate;        /* job sub-state */
  int     ji_svrflags;        /* server flags */
  int     ji_numattr;         /* number of attributes in list */
  int     ji_ordering;        /* special scheduling ordering */
  int     ji_priority;        /* internal priority */
  time_t  ji_stime;           /* time job started execution */
  char    ji_jobid[86];   /* job identifier */
  char    ji_fileprefix[12];  /* job file prefix */
  char    ji_queue[16];  /* name of current queue */
  char    ji_destin[87]; /* dest from qmove/route */
  int     ji_un_type;         /* type of ji_un union */
  union       /* depends on type of queue currently in */
    {
    struct          /* if in execution queue .. */
      {
      pbs_net_t ji_momaddr;  /* host addr of Server */
      int       ji_exitstat; /* job exit status from MOM */
      } ji_exect;

    struct
      {
      time_t  ji_quetime;               /* time entered queue */
      time_t  ji_rteretry;              /* route retry time */
      } ji_routet;

    struct
      {
      pbs_net_t  ji_fromaddr;     /* host job coming from   */
      int        ji_fromsock;     /* socket job coming over */
      int        ji_scriptsz;     /* script size */
      } ji_newt;

    struct
      {
      pbs_net_t ji_svraddr;  /* host addr of Server */
      int       ji_exitstat; /* job exit status from MOM */
      uid_t     ji_exuid;    /* execution uid */
      gid_t     ji_exgid;    /* execution gid */
      } ji_momt;
    } ji_un;
  } ji_qs_2_2_X;
  
typedef struct
  {
  int     qs_version;
  int     ji_state;           /* internal copy of state */
  int     ji_substate;        /* job sub-state */
  int     ji_svrflags;        /* server flags */
  int     ji_numattr;         /* number of attributes in list */
  int     ji_ordering;        /* special scheduling ordering */
  int     ji_priority;        /* internal priority */
  time_t  ji_stime;           /* time job started execution */
  char    ji_jobid[86];   /* job identifier */
  char    ji_fileprefix[62];  /* job file prefix */
  char    ji_queue[16];  /* name of current queue */
  char    ji_destin[87]; /* dest from qmove/route */
  int     ji_un_type;         /* type of ji_un union */
  union       /* depends on type of queue currently in */
    {
    struct          /* if in execution queue .. */
      {
      pbs_net_t ji_momaddr;  /* host addr of Server */
      int       ji_exitstat; /* job exit status from MOM */
      } ji_exect;

    struct
      {
      time_t  ji_quetime;               /* time entered queue */
      time_t  ji_rteretry;              /* route retry time */
      } ji_routet;

    struct
      {
      pbs_net_t  ji_fromaddr;     /* host job coming from   */
      int        ji_fromsock;     /* socket job coming over */
      int        ji_scriptsz;     /* script size */
      } ji_newt;

    struct
      {
      pbs_net_t ji_svraddr;  /* host addr of Server */
      int       ji_exitstat; /* job exit status from MOM */
      uid_t     ji_exuid;    /* execution uid */
      gid_t     ji_exgid;    /* execution gid */
      } ji_momt;
    } ji_un;
  } ji_qs_2_3_X;

typedef struct
  {
  int     qs_version;
  int     ji_state;           /* internal copy of state */
  int     ji_substate;        /* job sub-state */
  int     ji_svrflags;        /* server flags */
  int     ji_numattr;         /* number of attributes in list */
  int     ji_ordering;        /* special scheduling ordering */
  int     ji_priority;        /* internal priority */
  time_t  ji_stime;           /* time job started execution */
  char    ji_jobid[1046];   /* job identifier */
  char    ji_fileprefix[62];  /* job file prefix */
  char    ji_queue[16];  /* name of current queue */
  char    ji_destin[1047]; /* dest from qmove/route */
  int     ji_un_type;         /* type of ji_un union */
  union       /* depends on type of queue currently in */
    {
    struct          /* if in execution queue .. */
      {
      pbs_net_t ji_momaddr;  /* host addr of Server */
      int       ji_exitstat; /* job exit status from MOM */
      } ji_exect;

    struct
      {
      time_t  ji_quetime;               /* time entered queue */
      time_t  ji_rteretry;              /* route retry time */
      } ji_routet;

    struct
      {
      pbs_net_t  ji_fromaddr;     /* host job coming from   */
      int        ji_fromsock;     /* socket job coming over */
      int        ji_scriptsz;     /* script size */
      } ji_newt;

    struct
      {
      pbs_net_t ji_svraddr;  /* host addr of Server */
      int       ji_exitstat; /* job exit status from MOM */
      uid_t     ji_exuid;    /* execution uid */
      gid_t     ji_exgid;    /* execution gid */
      } ji_momt;
    } ji_un;
  } ji_qs_v1;

/* this function will upgrade a ji_qs struct to the
   newest version.

   */

int job_qs_upgrade(

  job *pj,     /* I */
  int fds,    /* I */
  char *path, /* I */
  int version) /* I */

  {
  char *id = "job_qs_upgrade";
  char namebuf[MAXPATHLEN];
  FILE *source; 
  FILE *backup;
  int c;

  /* reset the file descriptor */
  if (lseek(fds, 0, SEEK_SET) != 0)
    {
    sprintf(log_buffer, "unable to reset fds\n");
    log_err(-1, id, log_buffer);

    return (-1);
    }
  strcpy(namebuf, path); /* .JB path */
  namebuf[strlen(path) - strlen(JOB_FILE_SUFFIX)] = '\0'; /* cut off the .JB by replacing the '.' with a NULL */
  
  if (strlen(namebuf) + strlen(JOB_FILE_BACKUP) > MAXPATHLEN - 1)
    {
    sprintf(log_buffer, "ERROR: path too long for buffer, unable to backup!\n");
    log_err(-1, id, log_buffer);
    return (-1);
    }

  strcat(namebuf, JOB_FILE_BACKUP);

  source = fdopen(dup(fds), "r");

  if((backup = fopen(namebuf, "wb")) == NULL)
    {
    sprintf(log_buffer, "Cannot open backup file.\n");
    log_err(errno, id, log_buffer);
    return -1;
    }

  while ((c = fgetc(source)) != EOF)
    {
    fputc(c, backup);
    }

  fclose(backup);
  fclose(source);

  sprintf(log_buffer, "backed up to %s\n", namebuf);
  log_err(-1, id, log_buffer); 

  /* reset the file descriptor */
  if (lseek(fds, 0, SEEK_SET) != 0)
    {
    sprintf(log_buffer, "unable to reset fds\n");
    log_err(-1, id, log_buffer);

    return (-1);
    }

  if (version > PBS_QS_VERSION)
    {
    sprintf(log_buffer, "job struct appears to be from an unknown "
            "version of TORQUE and can not be converted");
    log_err(-1, "job_qs_upgrade", log_buffer);
    return (-1);
    }
  else if (version - PBS_QS_VERSION_BASE == 1)
    {
    return upgrade_v1(pj, fds);
    }
  /* old style version numbers... */
  else if (version == 0x00020300)
    {
    return  upgrade_2_3_X(pj, fds);
    }
  else if (version == 0x00020200)
    {
    return  upgrade_2_2_X(pj, fds);
    }
  /* predates versioning of ji_qs. assume 2.1.x format */
  else
    {
    return upgrade_2_1_X(pj, fds);
    }


  }


/* upgrader functions - these upgrade from a specific version of the job ji_qs struct
   to the _current_ version of the struct.  If the current version changes, these
   may need to be updated to reflect those changes */


int upgrade_v1(

  job *pj,   /* I */
  int  fds)  /* I */

  {
  ji_qs_v1 qs_old;


  if (read(fds, (char*)&qs_old, sizeof(qs_old)) != sizeof(qs_old))
    {
    return (-1);
    }

  pj->ji_qs.qs_version  = PBS_QS_VERSION;

  pj->ji_qs.ji_state    = qs_old.ji_state;
  pj->ji_qs.ji_substate = qs_old.ji_substate;
  pj->ji_qs.ji_svrflags = qs_old.ji_svrflags;
  pj->ji_qs.ji_numattr  = qs_old.ji_numattr;
  pj->ji_qs.ji_ordering = qs_old.ji_ordering;
  pj->ji_qs.ji_priority = qs_old.ji_priority;
  pj->ji_qs.ji_stime    = qs_old.ji_stime;

  strcpy(pj->ji_qs.ji_jobid, qs_old.ji_jobid);
  strcpy(pj->ji_qs.ji_fileprefix, qs_old.ji_fileprefix);
  strcpy(pj->ji_qs.ji_queue, qs_old.ji_queue);
  strcpy(pj->ji_qs.ji_destin, qs_old.ji_destin);



  pj->ji_qs.ji_un_type  = qs_old.ji_un_type;

  memcpy(&pj->ji_qs.ji_un, &qs_old.ji_un, sizeof(qs_old.ji_un));

  return 0;
  }


   
int upgrade_2_3_X(

  job *pj,   /* I */
  int  fds)  /* I */

  {
  ji_qs_2_3_X qs_old;


  if (read(fds, (char*)&qs_old, sizeof(qs_old)) != sizeof(qs_old))
    {
    return (-1);
    }

  pj->ji_qs.qs_version  = PBS_QS_VERSION;

  pj->ji_qs.ji_state    = qs_old.ji_state;
  pj->ji_qs.ji_substate = qs_old.ji_substate;
  pj->ji_qs.ji_svrflags = qs_old.ji_svrflags;
  pj->ji_qs.ji_numattr  = qs_old.ji_numattr;
  pj->ji_qs.ji_ordering = qs_old.ji_ordering;
  pj->ji_qs.ji_priority = qs_old.ji_priority;
  pj->ji_qs.ji_stime    = qs_old.ji_stime;

  strcpy(pj->ji_qs.ji_jobid, qs_old.ji_jobid);
  strcpy(pj->ji_qs.ji_fileprefix, qs_old.ji_fileprefix);
  strcpy(pj->ji_qs.ji_queue, qs_old.ji_queue);
  strcpy(pj->ji_qs.ji_destin, qs_old.ji_destin);



  pj->ji_qs.ji_un_type  = qs_old.ji_un_type;

  memcpy(&pj->ji_qs.ji_un, &qs_old.ji_un, sizeof(qs_old.ji_un));

  return 0;
  }


/* NOTE,  stripped out the code that updates the job array request attribute
   from the old 2.2 style to the current stile. Array's changed so much it 
   isn't practicle to try to upgrade them. We don't support upgrading 
   with these ond prototype job arrays queued */
   
int upgrade_2_2_X(

  job *pj,   /* I */
  int  fds)  /* I */

  {
  ji_qs_2_2_X qs_old;



  if (read(fds, (char*)&qs_old, sizeof(qs_old)) != sizeof(qs_old))
    {
    return (-1);
    }

  pj->ji_qs.qs_version  = PBS_QS_VERSION;

  pj->ji_qs.ji_state    = qs_old.ji_state;
  pj->ji_qs.ji_substate = qs_old.ji_substate;
  pj->ji_qs.ji_svrflags = qs_old.ji_svrflags;
  pj->ji_qs.ji_numattr  = qs_old.ji_numattr;
  pj->ji_qs.ji_ordering = qs_old.ji_ordering;
  pj->ji_qs.ji_priority = qs_old.ji_priority;
  pj->ji_qs.ji_stime    = qs_old.ji_stime;

  strcpy(pj->ji_qs.ji_jobid, qs_old.ji_jobid);
  strcpy(pj->ji_qs.ji_fileprefix, qs_old.ji_fileprefix);
  strcpy(pj->ji_qs.ji_queue, qs_old.ji_queue);
  strcpy(pj->ji_qs.ji_destin, qs_old.ji_destin);



  pj->ji_qs.ji_un_type  = qs_old.ji_un_type;

  memcpy(&pj->ji_qs.ji_un, &qs_old.ji_un, sizeof(qs_old.ji_un));



  return (0);
  }

int upgrade_2_1_X(

  job *pj,  /* I */
  int  fds) /* I */

  {
  ji_qs_2_1_X qs_old;


  if (read(fds, (char*)&qs_old, sizeof(qs_old)) != sizeof(qs_old))
    {
    return (-1);
    }

  pj->ji_qs.qs_version  = PBS_QS_VERSION;

  pj->ji_qs.ji_state    = qs_old.ji_state;
  pj->ji_qs.ji_substate = qs_old.ji_substate;
  pj->ji_qs.ji_svrflags = qs_old.ji_svrflags;
  pj->ji_qs.ji_numattr  = qs_old.ji_numattr;
  pj->ji_qs.ji_ordering = qs_old.ji_ordering;
  pj->ji_qs.ji_priority = qs_old.ji_priority;
  pj->ji_qs.ji_stime    = qs_old.ji_stime;

  strcpy(pj->ji_qs.ji_jobid, qs_old.ji_jobid);
  strcpy(pj->ji_qs.ji_fileprefix, qs_old.ji_fileprefix);
  strcpy(pj->ji_qs.ji_queue, qs_old.ji_queue);
  strcpy(pj->ji_qs.ji_destin, qs_old.ji_destin);

  pj->ji_qs.ji_un_type  = qs_old.ji_un_type;

  memcpy(&pj->ji_qs.ji_un, &qs_old.ji_un, sizeof(qs_old.ji_un));

  return (0);
  }
