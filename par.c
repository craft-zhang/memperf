#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sched.h>
#include <string.h>
#include <sys/syscall.h>

#ifdef __linux__
#define CPU_SETSIZE 1024
#define CPU_SET(cpu, cpusetp) \
  ((cpusetp)->__bits[(cpu)/__NCPUBITS] |= (1UL << ((cpu) % __NCPUBITS)))
#define CPU_ZERO(cpusetp) \
  memset((cpusetp), 0, sizeof(cpu_set_t))
#endif

int par_cid;
int par_npes;
int par_semid;

int sem_p(s)
     int s;
{
  struct sembuf   sem_ds;
  int rc;

  sem_ds.sem_num=0;
  sem_ds.sem_op=-1;
  sem_ds.sem_flg=0;
  if ((rc=semop(s,&sem_ds,1))==-1)
    perror("sem_p");
  return(1);
}


int sem_v(s)
     int s;
{
  struct sembuf   sem_ds;
  int rc;

  sem_ds.sem_num=0;
  sem_ds.sem_op=1;
  sem_ds.sem_flg=0;
  if ((rc=semop(s,&sem_ds,1))==-1)
    perror("sem_v");
  return(1);
}


int barrier()
{
  int s;
  struct sembuf sem_ds;
  int rc;

  /* printf("Entry Barrier proc %d\n",par_cid);
     fflush(stdout);
  */

  for (s=0; s<par_npes; s++) {
    sem_ds.sem_num=s;
    sem_ds.sem_op=1;
    sem_ds.sem_flg=0;
    if ((rc=semop(par_semid,&sem_ds,1))==-1)
      perror("sem_vb");
  }
  sem_ds.sem_num=par_cid;
  sem_ds.sem_op=-par_npes;
  sem_ds.sem_flg=0;
  if ((rc=semop(par_semid,&sem_ds,1))==-1)
    perror("sem_pb");

  /* printf("Exit Barrier proc %d\n",par_cid);
     fflush(stdout);
  */
  return(1);
}


int sem_init() {
  /*semget(IPC_PRIVATE,16,IPC_CREAT | 0600);*/

  if ((par_semid=semget(IPC_PRIVATE,16,IPC_CREAT | 0666)) ==-1) {
    perror("semget");return(99);
  }
  return(0);
}


int sem_deinit() {
  if ((semctl(par_semid,0,IPC_RMID,0))==-1) {
    perror("SemRelease");return(99);
  }
  return(0);
}


int begin_parallel(n)
     int n;
{
  int cidl,forkid=1;

  cidl=0;
  while ((cidl+1)<n && forkid!=0) {
    cidl++;
    if ((forkid=fork())==-1) {
      printf("fork failed\n");
      fflush(stdout);
      exit(99);
    }
  }
  if (forkid==0) { /* child */
  } else cidl=0;   /* parent */

#ifdef BINDTOCPU
  pid_t pid;
  pid = syscall(SYS_gettid);
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cidl + 4, &set);
  int syscallret = syscall(__NR_sched_setaffinity, pid, sizeof(set), &set);
  if (syscallret) {
    perror("bind_to_cpu()");
    printf("cidl %d\n)",cidl+1);
    exit(1);
  }
#endif

  par_cid=cidl;
  par_npes=n;
  barrier();
  return(1);
}


int end_parallel() {

  int stat,i;

  if (par_cid!=0) exit(0);
  for (i=1; i<par_npes; i++) {
    waitpid(0,&stat,0);
    if ((stat & 0xffffffff) !=0)
      printf("Job %d crashed, exit: %d\n",i,stat);
  }
  return(1);
}
