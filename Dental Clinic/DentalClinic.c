#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

typedef struct ClientNode { //declaring a struct
	int index;
	struct ClientNode *next, *prev;
} ClientNode;

#define N 10
sem_t dental, Numclinic,r1,r2,r3,r4, w, payment,pay2,sofa,treat; //declaring semaphores
ClientNode *headStand=NULL, *tailStand=NULL, *headSofa=NULL, *tailSofa=NULL,*temp; //declaring pointers for linked list for the clients
int n = 0;

void* Client(void* i) {
	while (1) {
		int index = *(int*)i;
		if (n >= N) { //no place in the clinic, try again soon
			printf("I'm Patient #%d, I'm out of clinic\n", index);
			sleep(1);
			continue;
		}
        sem_wait(&Numclinic);//only 10 Patients can be in the clininc
		sem_wait(&r1);//critical part 1
		n++; //increasing the number of clients
		printf("I'm Patient #%d, I got into the clinic\n", index);
		ClientNode *c = (ClientNode*)malloc(sizeof(ClientNode)); //create a new node
        if(c == NULL) { perror("Error in allocating memory"); exit(1); } //checking memory allocation
        //setting values in the node
		c->index = index;
		//insert to the head of the list
		c->prev = NULL;
        c->next = NULL;
		if (tailStand == NULL) { //first customer in Stand list
			headStand = c;
			tailStand = c;
		}
		else {//other Patients
	        c->next = headStand;
            headStand->prev=c;
			headStand = c;
		}
        sem_post(&r1);// finish critical part 1
        sleep(1);//release cpu
        while (tailStand->index!=index)// loop that ensures only the patien that stood for the longest time sits next on the sofa 
           sleep(1);
		//I'm the tailStand
		sem_wait(&sofa); //no place on the sofa, keep trying get place in the sofa
		//move the client from stand list to sofa list
        sem_wait(&r2);//critical part 2

        //move client to sofa
        ClientNode *temp=tailStand;
        tailStand= tailStand->prev;
        if (tailStand!=NULL)
            tailStand->next = NULL;
        temp->prev=NULL;
        temp->next=headSofa;
        if(headSofa!=NULL)
            headSofa->prev= temp;
        headSofa=temp;
        if(tailSofa==NULL)
            tailSofa= headSofa;
       	printf("I'm Patient #%d, I'm sitting on the sofa\n",index);
       	sem_post(&r2);//finish critical part 2
        while (tailSofa->index!=index)// loop that makes sure the Patient who sat longes on the sofa goes to treatment
           sleep(1);
        sem_wait(&treat);// only 3 treatments are available at a time
        sem_wait(&r3);//critical code part 3
        tailSofa=tailSofa->prev;
        if(tailSofa!=NULL)
            tailSofa->next=NULL;
        printf("I'm Patient #%d, I'm getting treatment\n", index);
        sem_post(&r3);// finish critical code part 3
        sem_post(&sofa);//sofa spot open , a new patient can sit
        sem_post(&dental);// dental hygeninst can begin working
        sleep(1); // getting treatment
        sem_wait(&pay2);//only one patient can pay at a time (1 cashier)
        printf("I'm Patient #%d, I'm paying now.\n", index);
        sem_post(&treat);// new patient can go to treatment
		sem_post(&payment); //dental hygeninst can now recieve payment
        sem_wait(&w);//waiting for dental hygenist to release the cutomer
		sem_wait(&r4);//critical code part 4
		n--; //customer has left the clinic
        sem_post(&Numclinic);// new Patient can enter the clinic
		sem_post(&r4);//finish critical code part 4
        sleep(1);
        free(c);//release memory for leaving patient
	}
}

void* Dental(void* i) {
	while (1) {
		int index = *(int*)i;
		sem_wait(&dental);//block dental hygenist if there are no clients getting treatment
		printf("I'm Dental Hygienist #%d, I'm working now\n", index);
		sleep(1);//treating Patient
		sem_wait(&payment); //waiting for client to pay
		printf("I'm Dental Hygienist #%d, I'm getting a payment\n", index);
        sem_post(&w);//release customer
        sem_post(&pay2);//release cashier
	}
}

int main() {
int i, arrDental[3], arrClient[N + 2];
	pthread_t arrD[3], arrC[N + 2];
    sem_init(&Numclinic,0, N);//number of clients in clinic
	sem_init(&r1,0, 1); //semaphore for the clients(readers) critical parts
    sem_init(&r2,0, 1);
    sem_init(&r3,0, 1);
    sem_init(&r4,0, 1);
    sem_init(&sofa,0,4);// semaphore that allows 4 Paients to be seated on the sofa 
    sem_init(&treat,0,3);// semaphore that allows 3 Patients to recive treatment at a time
	sem_init(&w,0, 1); //semaphore for the dental hygienist on the cashier
	sem_init(&dental,0, 0); //semaphore for the avaliable dental
	sem_init(&payment,0, 0); //semaphore for the dental hygienist to wait for the payment of the client
    sem_init(&pay2,0,1); // semaphore that allows only 1 patient to pay at a time
    for (i = 0; i < N + 2; i++) { //threads for the clients
		arrClient[i] = i; //we need to send the index
		if(pthread_create(&arrC[i], NULL, Client, &arrClient[i]) != 0) { perror("Error creating thread"); exit(1); } //creating threads
	}
	for (i = 0; i < 3; i++) { //threads for the dental
		arrDental[i] = i; // we need to send the index
		if(pthread_create(&arrD[i], NULL, Dental, &arrDental[i]) != 0) { perror("Error creating thread"); exit(1); } //creating threads
	}
    for (i = 0; i < N + 2; i++)
        pthread_join(arrC[i],NULL);
    for (i = 0; i < 3; i++)
        pthread_join(arrD[i],NULL);
	return 0;
}

