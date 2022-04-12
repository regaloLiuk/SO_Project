#include "util.c"
#include "opt_conf.h"

int main(){
    
	pid_t ufficio_studenti;
  	
	struct student *shared_data; 
    	
	int i;
	
	catch_SIGUSR1(SIGUSR1);
  	
	catch_ALLARM(SIGALRM);

//CODE MESSAGGI
/*coda per ricerca partner*/
  	struct msgbuf messaggio;	
	struct msqid_ds mq_info;
	size_t data_size; 
	int codaPr = allocaCoda();

//SHARED MEMORY
	int shid;
  	shid = allocaMemoria(sizeof(struct student));

	//SEMAFORI
/*semaforo attesa sincronizzazione tra studenti*/
	int semZero = allocaSemaforo(POP_SIZE+1);
	union semun arg_semZero;
    	arg_semZero.val = POP_SIZE;
	modificaSemaforo(semZero, POP_SIZE, arg_semZero);

/*semVoto per attesa voto da gestore*/ 
    	int semVoto = allocaSemaforo(1); 
    	union semun arg_semVoto;
    	arg_semVoto.val = 1;
	modificaSemaforo(semVoto, 0, arg_semVoto);

/*semFine attesa completamento progetto/fine tempo*/
    	int semFine = allocaSemaforo(1); 
	union semun arg_semFine;
	arg_semFine.val = POP_SIZE;
	modificaSemaforo(semFine, 0, arg_semFine);
    
    
	for(i = 0;i < POP_SIZE;i++){
	
		switch(ufficio_studenti = fork()){
			
			case-1:
				/* Handle error */
		    		fprintf(stderr,"Error #%03d: %s\n", errno, strerror(errno));
				break;
	
/********************************************************* STUDENTI ***********************************************************************/
			case 0:
//FASE DI INIZIALIZZAZIONE
				srand(time(NULL)+getpid());
				shared_data = shmat(shid, NULL, 0);
				
//inizializzazione valori nella struttura
	
				shared_data->matricola[i] = getpid();
				shared_data->turno[i] = shared_data->matricola[i]%2;
				shared_data->voto_AdE[i] = rand()%13+18;
				shared_data->voto_SO[i] = 0;
				shared_data->nof_elements[i] = preferenze(P2,P3,P4);
				shared_data->ruolo[i] = -1;
				shared_data->rifiuti[i] = 0;
				shared_data->inviti[i] =0;
				shared_data->nAdepti[i] = 0;
				int k = 0;
				esc = 0;
				while(k < 3){
					shared_data->Adepti[i][k] = -1;;
					k++;
				}
  				
				arg_semZero.val = 0;
				modificaSemaforo(semZero, i, arg_semZero);
				reserveSem(semZero,POP_SIZE);
				waitZero_sem(semZero,POP_SIZE);
				

						/******INIZIO PROGETTO*********/ 

//RUOLO -1 
				while(shared_data->ruolo[i] == -1 && esc == 0 ){
				
				do {
				
					data_size = msgrcv(codaPr, &messaggio, MSG_LEN,shared_data->matricola[i],IPC_NOWAIT);
					msgctl(codaPr, IPC_STAT, &mq_info);
//controllo messaggi
					if (data_size!=-1){
						int matricola_invito= messaggio.testo;
						if(shared_data->voto_AdE[matricola_invito] > shared_data->voto_AdE[i] || 
						shared_data->nof_elements[matricola_invito] >= shared_data->nof_elements[i] ||
						shared_data->rifiuti[i] == max_reject || 
						shared_data->nAdepti[matricola_invito] > 0){
// accetto
						if(esc == 0){
							
							shared_data->ruolo[i] = 0;
							shared_data->ruolo[matricola_invito] = 1;
							
						}
						if(esc == 0){
							
							shared_data->Adepti[matricola_invito][ shared_data->nAdepti[matricola_invito] ] = i;
							shared_data->nAdepti[matricola_invito] = shared_data->nAdepti[matricola_invito]+1; 
												
						}


					reserveSem(semZero,matricola_invito);

				}else{
// rifiuto
					
					shared_data->rifiuti[i] = shared_data->rifiuti[i]+1;
					
					reserveSem(semZero,matricola_invito);
  				}

		  		}

				} 

				while (data_size!=-1 && shared_data->ruolo[i] == -1 && esc == 0);
					releaseSem(semZero,i);
					releaseSem(semZero,i);
		
//invito messaggi
				int matricola_da_invitare;

				do{
					matricola_da_invitare = rand()%POP_SIZE;				
				}while(esc == 0 && (matricola_da_invitare == i 
						|| shared_data->matricola[matricola_da_invitare]%2 != shared_data->turno[i]));

  				if(shared_data->inviti[i] <= nof_invites && 
  				shared_data->ruolo[i] == -1 &&
  		 		shared_data->ruolo[matricola_da_invitare] == -1&& 
  			 	matricola_da_invitare != i &&
  			 	esc == 0 && 
  			      	semctl(semZero, matricola_da_invitare, GETVAL) != 2 ){

					shared_data->inviti[i] = shared_data->inviti[i]+1;  				
  				
					messaggio.mtype = shared_data->matricola[matricola_da_invitare];
					messaggio.testo = i;
							
					if (msgsnd(codaPr, &messaggio, sizeof(messaggio.testo), 0)<0) {
    					fprintf(stderr, "%s: %d. Errore in msgsnd #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
    					exit(EXIT_FAILURE);
  					}
  					msgctl(codaPr,IPC_STAT,&mq_info);
					reserveSem(semZero,i);

				while(semctl(semZero, i, GETVAL) != 0 && esc == 0){
				do {
					data_size = msgrcv(codaPr, &messaggio, MSG_LEN,shared_data->matricola[i],IPC_NOWAIT);
					msgctl(codaPr, IPC_STAT, &mq_info);

					if (data_size!=-1){
						int matricola_invito = messaggio.testo;
						reserveSem(semZero,matricola_invito);
	  				}
  				}while(data_size!=-1);
  				}

  			}else{reserveSem(semZero,i);reserveSem(semZero,i);}
  			
  			}

//RUOLO 0

  			while(shared_data->ruolo[i] == 0 && esc == 0){

			do {
				data_size = msgrcv(codaPr, &messaggio, MSG_LEN,shared_data->matricola[i],IPC_NOWAIT);
				msgctl(codaPr, IPC_STAT, &mq_info);
//rifiuto indiscriminato
				if (data_size!=-1){
					int matricola_invito = messaggio.testo;
					reserveSem(semZero,matricola_invito);
  					}
  			}while(data_size!=-1);


  		}

//RUOLO 1

		while(shared_data->ruolo[i] == 1 && esc == 0  ){
			if(shared_data->nAdepti[i] != shared_data->nof_elements[i]-1){

				releaseSem(semZero,i);
				releaseSem(semZero,i);

				int matricola_da_invitare;

				do{
					matricola_da_invitare = rand()%POP_SIZE;				
				}while(esc == 0 && (matricola_da_invitare == i 
						||shared_data->matricola[matricola_da_invitare]%2 != shared_data->turno[i]));
//inio richiesta
	  			if(shared_data->inviti[i] <= nof_invites  && 
	  			shared_data->ruolo[matricola_da_invitare] == -1 && 
	  			matricola_da_invitare != i && 
	  			esc == 0 && 
	  			semctl(semZero, matricola_da_invitare, GETVAL) != 2 ){

					shared_data->inviti[i] = shared_data->inviti[i]+1;  				
					
	  				
					messaggio.mtype = shared_data->matricola[matricola_da_invitare];
					messaggio.testo = i;
									
					if (msgsnd(codaPr, &messaggio, sizeof(messaggio.testo), 0)<0) {
	    				fprintf(stderr, "%s: %d. Errore in msgsnd #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
	    				exit(EXIT_FAILURE);
	  				}
	  				msgctl(codaPr,IPC_STAT,&mq_info);
	  				
					reserveSem(semZero,i);
//rifiuto richiesta
					while(semctl(semZero, i, GETVAL) != 0 && esc == 0){
					do {
						data_size = msgrcv(codaPr, &messaggio, MSG_LEN,shared_data->matricola[i],IPC_NOWAIT);
						msgctl(codaPr, IPC_STAT, &mq_info);;

					if (data_size!=-1){
						int matricola_invito= messaggio.testo;
						reserveSem(semZero,matricola_invito);
	  				}
  				}while(data_size!=-1);
  			}

  			}else{reserveSem(semZero,i);reserveSem(semZero,i);}

  		}
  		}

// FINE PROGETTO
  				
  				reserveSem(semFine,0);
  				
  				waitZero_sem(semVoto,0);

				//printf("Matricola %d , voto SO = %d\n",shared_data->matricola[i],shared_data->voto_SO[i]);

//SEPARAZIONE SM STUDENTI E USCITA
				detach(shared_data);
				
				exit(EXIT_SUCCESS);

/********************************************************* PROCESSO GESTORE ***************************************************************/
			
			default:
			if(i == POP_SIZE-1){

				catch_SIGUSR1(SIGUSR1);
  					
//ATTSA INIZIALIZZAZIONE STUDENTI
				waitZero_sem(semZero,POP_SIZE);
				printf("gli studenti hanno inizializzato i valori\n\n");
				sleep(1);
				printf("INIZIO PROGETTO...\n\n");
				shared_data = shmat(shid, NULL, 0);
//INIZIO PROGETTO
				alarm(sim_time);


//FINE PROGETTO
				waitZero_sem(semFine,0);
				printf("FINE PROGETTO\n\n");
				printf("CALCOLO VOTI\n\n");
				sleep(1);
				int k = 0;
				for(k = 0; k < POP_SIZE;k++){

					if(shared_data->ruolo[k] == 1){

						int cont = 0;
						int Max = shared_data->voto_AdE[k];
						while(cont < shared_data->nAdepti[k]){
							int Adp = shared_data->Adepti[k][cont];
							if(Max < shared_data->voto_AdE[ Adp ]){
								Max = shared_data->voto_AdE[Adp];					
							}
							cont++;
						}
						cont = 0;
						if(shared_data->nof_elements[k]-1 == shared_data->nAdepti[k]){
							shared_data->voto_SO[k] = Max;

						}else{ shared_data->voto_SO[k] = Max-3; }
						while(cont < shared_data->nAdepti[k]){
							int Adp = shared_data->Adepti[k][cont];
							if(shared_data->nof_elements[Adp]-1 == shared_data->nAdepti[k]){
								shared_data->voto_SO[Adp] = Max;

							}else{ shared_data->voto_SO[Adp] = Max-3; }
							cont++;
						}


					}
				}
				
				printf("VOTI REGISTRATI\n\n");

// ATTESA PRESA VISIONE DEL VOTO
				
				reserveSem(semVoto,0);

				while(ufficio_studenti = wait(NULL) != -1){}
				printf("\ngli studenti sono terminati\n\n");
				
				sleep(1);

				printf("Numero degli studenti per voto di Architettura = %d\n", POP_SIZE);
				int media_AdE = 0;
				for(int j = 0;POP_SIZE >j ;j++){
					media_AdE = media_AdE + shared_data->voto_AdE[j];
				}
				printf("Media per voto di Architettura = %d\n", media_AdE/POP_SIZE);
				printf("\n");
				printf("Numero degli studenti per voto di Sistemi operativi = %d\n", POP_SIZE);
				int media_SO = 0;
				for(int j = 0;POP_SIZE >j ;j++){
					media_SO = media_SO + shared_data->voto_SO[j];
				}
				printf("Media per voto di Sistemi operativi = %d\n", media_SO/POP_SIZE);
				printf("\n");
	
				printf("\n");
				int cont0=0;	
				for(int j = 0;POP_SIZE >j ;j++){
					if(shared_data->voto_SO[j]==0)
						cont0++;
				}
				printf("numero studenti senza gruppo: %d\n",cont0);


//DEALLOCO CODE MESSAGGI
				rimuoviCoda(codaPr);
				
// SEPARAZIONE SM PADRE	
				detach(shared_data);
				
// DEALLOCO SHARED MEMORY
				rimuoviMemoria(shid);
 		
// DEALLOCO SEMAFORI
				rimuoviSemaforo(semZero);

				rimuoviSemaforo(semVoto);

				rimuoviSemaforo(semFine);
			}


		}



    }

}
