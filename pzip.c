#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>

char* file[2000];
int size[2000];
int SIZE = 100000;
int TOTAL_SIZE = 0;
pthread_mutex_t mutex;
pthread_cond_t cv;
int turn = 0;


void* pzip(void* arg) {

    int *argt = (int*) arg;
    int sf = argt[0];
    int ef = argt[1];
    int sc = argt[2];
    int ec = argt[3];
    int myturn = argt[4];
  //  printf("Thread Number %d\n", myturn);
    free(arg);
    char c_prev = file[sf][sc];;
    char c_curr;

    int *c = malloc(sizeof(int)*(TOTAL_SIZE));
    int *c_free = c;
    char *l = malloc(sizeof(char)*(TOTAL_SIZE));
    char *l_free = l;
    //printf("id %d sf %d ef %d sc %d ec %d csc%c cec%c\n ", myturn, sf, ef, sc, ec, file[sf][sc], file[ef][ec]);
    int count = 1;
    int index = 0;
    for (int i = sf; i <= ef; i++) {
	//printf("id %d curr %d end %d\n", myturn, i, ef);
	int start;
	int end;
	if( i==sf && sf==ef) {
	    start = sc + 1;
	    end = ec;
	} else if (i==sf) {
	    start = sc + 1;
	    end = size[i]-1;
	} else if (i==ef) {
	    start = 0;
	    end = ec;
	} else {
	    start = 0;
	    end = size[i]-1;
	}
//	    printf("start %d end %d\n ", start, end);
	for(int j=start; j<=end ;j++ ) {
            if(file[i][j]=='\0')
                continue;
            c_curr = file[i][j];
            if (c_curr == c_prev) {
                count++;
            } else {
               l[index] = c_prev;
               c[index] = count;
               index = index + 1;
               c_prev = c_curr;
               count = 1;
            }
	}  
//	printf("%d %s\n", myturn, "DONE");
    }
//    printf("Loop Done\n");
    l[index] = c_prev;
    c[index] = count;

    pthread_mutex_lock(&mutex);
    while(myturn != turn) {
         pthread_cond_wait(&cv, &mutex);
    }
  //  printf("Turn arrived\n");
    for(int i=0;i<index+1;i++) {
        fwrite(&c[i], sizeof(int),1, stdout);
        fwrite(&l[i], sizeof(char), 1, stdout);
    }

    turn = turn + 1;
    free(l_free);
    free(c_free);
    pthread_cond_signal(&cv);
//    printf("Signal Sent\n");
    pthread_mutex_unlock(&mutex);
    pthread_exit(0);
}


int main(int argc, char const *argv[])
{   
    int num_of_threads = get_nprocs();
    pthread_t threads[num_of_threads*3];
    pthread_mutex_init(&mutex, NULL);
    int thread_num = 0;
    struct stat s;

    char* error_message = "pzip: file1 [file2 ...]\n"; 

    if(argv[1] == NULL) {
	for(int i=0;i<strlen(error_message);i++)
	    fwrite(&error_message[i], sizeof(char), 1,stdout); 
	exit(1);
    }

    int n = 1;
    while(argv[n]!=NULL) {
        int fd = open (argv[n], O_RDONLY);
        if (fd == -1) {
	    n++;
	    continue;
	}
        int status = fstat (fd, &s);
        if (status == -1) {
	    exit(1);
        }
	if(n==1) {
	    size[n-1] = s.st_size;
	    TOTAL_SIZE += size[n-1];
	    file[n-1] = (char *) mmap (0, size[n-1], PROT_READ, MAP_PRIVATE, fd, 0);
	    n++;
	    continue;
	}

        size[n-1] = s.st_size;
	TOTAL_SIZE += size[n-1];
        file[n-1] = (char *) mmap (0, size[n-1], PROT_READ, MAP_PRIVATE, fd, 0);
	n++;
    }

    int sf = 0;
    int sc = 0;
    int start = 0;
    int tsize = 0;
    for(int i=0;i<n-1;i++) {
	if(TOTAL_SIZE < 100000) {
	    break;
	}
	for(int k=0;k<size[i];k++) {
	    if(start==0 && file[i][k]!='\0') {
                 sf = i;
                 sc = k;
		 start = 1;
		 break;
	    }
        } 
	if(start == 0){
	    continue;
	}

	if(sf == i)
       	    tsize = tsize + size[i] - sc;
	else 
      	    tsize = tsize + (size[i]);


	if(tsize < (TOTAL_SIZE / num_of_threads) && i < (argc)) {
	    continue;
	}
        
	int j = 0;
	if(sf==i) {
	    j = j + sc + (TOTAL_SIZE/4);
	}
  //      printf("%d\n", TOTAL_SIZE);	
	for(;j<size[i]-1;j++) {
	    if(file[i][j]!='\0' && file[i][j]!=file[i][j+1]) {
		int *argt = (int*)malloc((5)*sizeof(int));
		argt[0] = sf; //starting file
		argt[1] = i; //ending file
		argt[2] = sc; //starting character
		argt[3] = j; //ending character
		argt[4] = thread_num;
//	        printf("start file %d, end file %d, sc %d, ec %d\n", argt[0], argt[1], argt[2], argt[3]);
		pthread_create(&threads[thread_num], NULL, pzip, (void*)argt);
                thread_num++;

		sf = i;
		sc = j + 1;
		tsize = 0;
		j = j + (TOTAL_SIZE / 4);
	    }
	}	    
    }
    
    int *argt = (int*)malloc((4)*sizeof(int));
    argt[0] = sf;
    argt[1] = n - 2;
    argt[2] = sc;
    argt[3] = size[n-2] - 1;
    argt[4] = thread_num;
//    printf("start file %d, end file %d, sc %d, ec %d\n", argt[0], argt[1], argt[2], argt[3]);
    pthread_create(&threads[thread_num], NULL, pzip, (void*)argt);

    for(int i=0; i< (thread_num + 1); i++) {
	pthread_cond_broadcast(&cv);
        pthread_join(threads[i], NULL);
    }
}
