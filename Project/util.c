#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>

#include "opt_conf.h"


struct student{
	int matricola[POP_SIZE];
	int voto_AdE[POP_SIZE];
	int voto_SO[POP_SIZE];
	int nof_elements[POP_SIZE];
	int turno[POP_SIZE];
	int ruolo[POP_SIZE];
	int rifiuti[POP_SIZE];
	int inviti[POP_SIZE];
	int nAdepti[POP_SIZE];
	int Adepti[POP_SIZE][3];
};

/*STRUC PER CODDI MESSAGGI*/
struct msgbuf {
	long mtype;             /* message type, must be > 0 */
	int testo;    		/* message data */
};


/*UNION NECERSSARIA PER ALLOCARE SEMAFORI*/
union semun{
int val;
struct semid_ds* buf;
unsigned short* array;
#if defined(__linux__)
	struct seminfo* __buf;
#endif
};

/*ALLARMI*/
int esc;
void alarmHandler(int sig ) {
	kill(0,SIGUSR1);
}
void sigurs1Handler(int sig){
	esc =1;
}
//intecettazione segnali
void catch_SIGUSR1(int allarme){
  	if(signal(allarme, sigurs1Handler)==SIG_ERR){
    		printf("\nErrore della disposizione dell'handler\n");
    		exit(EXIT_SUCCESS);
  	}  	
}
void catch_ALLARM(int allarme){
  	if (signal(allarme, alarmHandler)==SIG_ERR){
    		printf("\nErrore della disposizione dell'handler\n");
    		exit(EXIT_SUCCESS);
  	}
}

/*STRUCT PER ESEGUIRE OPERAZIONI SUI SEMAFORI*/
/*struct sembuf{
unsigned short sem_num;
short sem_op;
short sem_flg;
};*/

/*SEMAFORI*/
/*alloca semaforo*/
int allocaSemaforo(int numSem){
	if (semget(IPC_PRIVATE, numSem, IPC_CREAT|IPC_EXCL|S_IRUSR | S_IWUSR) == -1) {
		fprintf(stderr, "%s: %d. Errore in semget #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
	      	exit(EXIT_FAILURE);
	}
}
/*modifica semforo*/
int modificaSemaforo(int semaforo, int semNum, union semun arg){
	if (semctl(semaforo, semNum, SETVAL, arg) == -1) {
      		fprintf(stderr, "%s: %d. Errore in semctl #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
		exit(EXIT_FAILURE);
    	}
}
/*operazioni semaforo*/
int reserveSem(int semId, int semNum){					/*DECREMENTO (P)*/
 	struct sembuf sop; // la struttura che definisce l'operazione
	sop.sem_num = semNum;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	if (semop(semId, &sop, 1) == -1) {
      		fprintf(stderr, "%s: %d. Errore in semop #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
     	 	exit(EXIT_FAILURE);
    }
}
int releaseSem(int semId, int semNum){					/*INCREMENTO (V)*/
	struct sembuf sop; // la struttura che definisce l'operazione
    	sop.sem_num = semNum;
    	sop.sem_op = 1;
    	sop.sem_flg = 0;
	if (semop(semId, &sop, 1) == -1) {
      		fprintf(stderr, "%s: %d. Errore in semop #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
      		exit(EXIT_FAILURE);
	}
}
int waitZero_sem(int semId, int semNum){				/*ATTESA ZERO*/
	struct sembuf sop; // la struttura che definisce l'operazione
    	sop.sem_num = semNum;
    	sop.sem_op = 0;
    	sop.sem_flg = 0;
	semop(semId,&sop,1);
}
/*rimuovi semaforo*/
void rimuoviSemaforo(int semaforo){
	if (semctl(semaforo, 0, IPC_RMID) == -1) {
  		fprintf(stderr, "%s: %d. Errore in semctl #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
	    	exit(EXIT_FAILURE);
	}
}

/*MEMORIA CONDIVISA*/
/*alloca memoria condivisa*/
int allocaMemoria(int dimensione){
  	if (shmget(IPC_PRIVATE, dimensione, S_IRUSR | S_IWUSR) == -1) {
    		fprintf(stderr, "%s: %d. Errore in semget #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
    		exit(EXIT_FAILURE);
  	}
}

/*separazione da memoria condivisa*/
void detach(struct student *mem){	//sgancio
	if (shmdt(mem) == -1) {
		fprintf(stderr, "%s: %d. Errore in shmdt #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
		exit(EXIT_FAILURE);				
	}				
}
/*rimuovi meoria condivisa*/
void rimuoviMemoria(int memory){
	if (shmctl(memory, 0, IPC_RMID) == -1) {
		fprintf(stderr, "%s: %d. Errore in semctl #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/*CODE DI MESSAGGI*/
/*alloca coda messaggi*/
int allocaCoda(){
	if (msgget(IPC_PRIVATE, IPC_CREAT|0600) == -1) {
    		fprintf(stderr, "%s: %d. Errore in msgget #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
    		exit(EXIT_FAILURE);
  	}	
}
/*rimozione coda*/
void rimuoviCoda(int queue){
	if (msgctl(queue, IPC_RMID, NULL)<0) {
	    	fprintf(stderr,"%s: %d. Errore in msgctl (IPC_RMID) #%03d: %s\n",__FILE__,__LINE__,errno,strerror(errno));
		exit(EXIT_FAILURE);
	}
}


/*dati di memoria condivisa*/
int stampaStatoMemoria(int shid) {
  struct shmid_ds buf;
  if (shmctl(shid,IPC_STAT,&buf)==-1) {
    fprintf(stderr, "%s: %d. Errore in shmctl #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
    return -1;
  } else {
    printf("\nSTATISTICHE MEMORIA\n");
    printf("AreaId: %d\n",shid);
    printf("Dimensione: %ld\n",buf.shm_segsz);
    printf("Ultima shmat: %s\n",ctime(&buf.shm_atime));
    printf("Ultima shmdt: %s\n",ctime(&buf.shm_dtime));
    printf("Ultimo processo shmat/shmdt: %d\n",buf.shm_lpid);
    printf("Processi connessi: %ld\n",buf.shm_nattch);
    printf("\n");
    return 0;
  }
}
/*dati coda messaggi*/
int stampaStatoCoda(int queue){
struct msqid_ds mq_info;	//per info code messaggi
if (msgctl(queue, IPC_STAT, &mq_info)==-1) {
   	 	fprintf(stderr, "%s: %d. Errore in msgctl (IPC_STAT) #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
    		exit(EXIT_FAILURE);
  	}
	 printf("\nSTATISTICHE CODA\n");
  	printf("Stato di msg_qbytes: %ld\n",mq_info.msg_qbytes);
	printf("Stato di msg_qnum: %ld\n",mq_info.msg_qnum);
  	printf("Stato di msg_qbytes: %ld\n",mq_info.msg_qbytes);
  	printf("Stato di msg_stime: %s\n",ctime(&mq_info.msg_stime));
}

void stampaStudenti(int memoria){
struct student *shared_data = shmat(memoria, NULL, 0);
	printf("id   turno  voto_AdE  voto_SO  nof_elem  ruolo  nAdepti  Adepti \n");
for(int j = 0;POP_SIZE >j ;j++){
	printf("%d      %d       %d       %d       %d         %d      %d       ",	j,
										shared_data->turno[j],
										shared_data->voto_AdE[j],
										shared_data->voto_SO[j],
										shared_data->nof_elements[j],
										shared_data->ruolo[j],
										shared_data->nAdepti[j]);
	int k=0;
	while(k < 3){
		printf("%d ", shared_data->Adepti[j][k]);
		k++;
	}	
	printf("\n");
}
	detach(shared_data);

}

int preferenze(int p2, int p3, int p4){
	int a[100] = {0};
	int i=0;
	for(i;i<p2;i++){a[i]=2;}		//i --> P2
	int j=i;				//j = P2
	for(j;j<(p2+p3);j++){a[j]=3;}		//j --> P3
	int k=j;				//k=P2+P3
	for(k;k<(p2+p3+p4);k++){a[k]=4;}	//k --> 99
	int n=0;
	int rnd=rand()%100;
	if(a[rnd]==2)
		return 2;
	else if(a[rnd]==3)
		return 3;
	else if(a[rnd]==4)
		return 4;
}
