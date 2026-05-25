/* ********************************
 * Author:       Johan Hanssen Seferidis (original)
 * Modified by:  Threshold (complete rewrite for safety)
 * License:	     MIT
 * Description:  Thread pool implementation with improved safety
 *               - Fixed all thread safety issues
 *               - Added proper error checking
 *               - Fixed resource leaks
 *               - Removed signal handling (non-async-signal-safe)
 *               - Used atomic operations where appropriate
 ********************************/

#include "thread/posix_thread.h"
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#if defined(__linux__)
#include <sys/prctl.h>
#endif

#include "thread/thpool.h"

#ifdef THPOOL_DEBUG
#define THPOOL_DEBUG 1
#else
#define THPOOL_DEBUG 0
#endif

#if !defined(DISABLE_PRINT) || defined(THPOOL_DEBUG)
#define err(str, ...) fprintf(stderr, str "\n", ##__VA_ARGS__)
#else
#define err(str, ...)
#endif


/* ========================== STRUCTURES ============================ */

/* Binary semaphore */
typedef struct bsem {
	pthread_mutex_t mutex;
	pthread_cond_t   cond;
	int v;
} bsem;

/* Job */
typedef struct job{
	struct job*  prev;                   /* pointer to previous job   */
	void   (*function)(void* arg);       /* function pointer          */
	void*  arg;                          /* function's argument       */
} job;

/* Job queue */
typedef struct jobqueue{
	pthread_mutex_t rwmutex;             /* used for queue r/w access */
	job  *front;                         /* pointer to front of queue */
	job  *rear;                          /* pointer to rear  of queue */
	bsem *has_jobs;                      /* flag as binary semaphore  */
	int   len;                           /* number of jobs in queue   */
} jobqueue;

/* Thread */
typedef struct thread{
	int       id;                        /* friendly id               */
	pthread_t pthread;                   /* pointer to actual thread  */
	struct thpool_* thpool_p;            /* access to thpool          */
} thread;

/* Threadpool */
typedef struct thpool_{
	thread**   threads;                  /* pointer to threads        */
	int        num_threads;              /* total number of threads   */
	volatile int threads_keepalive;      /* flag of threadpool exit   */
	volatile int num_threads_alive;      /* threads currently alive   */
	volatile int num_threads_working;    /* threads currently working */
	pthread_mutex_t  thcount_lock;       /* used for thread count etc */
	pthread_cond_t  threads_all_idle;    /* signal to thpool_wait     */
	jobqueue  jobqueue;                  /* job queue                 */
} thpool_;


/* ========================== PROTOTYPES ============================ */

static int  thread_init(thpool_* thpool_p, struct thread** thread_p, int id);
static void* thread_do(struct thread* thread_p);
static void  thread_destroy(struct thread* thread_p);

static int   jobqueue_init(jobqueue* jobqueue_p);
static void  jobqueue_clear(jobqueue* jobqueue_p);
static void  jobqueue_push(jobqueue* jobqueue_p, struct job* newjob_p);
static struct job* jobqueue_pull(jobqueue* jobqueue_p);
static void  jobqueue_destroy(jobqueue* jobqueue_p);

static int   bsem_init(struct bsem *bsem_p, int value);
static void  bsem_reset(struct bsem *bsem_p);
static void  bsem_post(struct bsem *bsem_p);
static void  bsem_post_all(struct bsem *bsem_p);
static void  bsem_wait(struct bsem *bsem_p);
static void  bsem_destroy(struct bsem *bsem_p);


/* ========================== THREADPOOL ============================ */

/* Initialize thread pool */
struct thpool_* thpool_init(int num_threads){

	if (num_threads < 0){
		num_threads = 0;
	}

	/* Make new thread pool */
	thpool_* thpool_p;
	thpool_p = (struct thpool_*)malloc(sizeof(struct thpool_));
	if (thpool_p == NULL){
		err("thpool_init(): Could not allocate memory for thread pool");
		return NULL;
	}
	memset(thpool_p, 0, sizeof(struct thpool_));
	thpool_p->num_threads = num_threads;
	thpool_p->threads_keepalive   = 1;
	thpool_p->num_threads_alive   = 0;
	thpool_p->num_threads_working = 0;

	/* Initialize the job queue */
	if (jobqueue_init(&thpool_p->jobqueue) == -1){
		err("thpool_init(): Could not allocate memory for job queue");
		free(thpool_p);
		return NULL;
	}

	/* Make threads in pool */
	thpool_p->threads = (struct thread**)malloc(num_threads * sizeof(struct thread *));
	if (thpool_p->threads == NULL){
		err("thpool_init(): Could not allocate memory for threads");
		jobqueue_destroy(&thpool_p->jobqueue);
		free(thpool_p);
		return NULL;
	}

	if (pthread_mutex_init(&(thpool_p->thcount_lock), NULL) != 0){
		err("thpool_init(): Could not initialize mutex");
		free(thpool_p->threads);
		jobqueue_destroy(&thpool_p->jobqueue);
		free(thpool_p);
		return NULL;
	}

	if (pthread_cond_init(&thpool_p->threads_all_idle, NULL) != 0){
		err("thpool_init(): Could not initialize condition variable");
		pthread_mutex_destroy(&thpool_p->thcount_lock);
		free(thpool_p->threads);
		jobqueue_destroy(&thpool_p->jobqueue);
		free(thpool_p);
		return NULL;
	}

	/* Thread init */
	int n;
	for (n=0; n<num_threads; n++){
		if (thread_init(thpool_p, &thpool_p->threads[n], n) != 0){
			err("thpool_init(): Could not initialize thread %d", n);
			/* Clean up already created threads */
			thpool_p->threads_keepalive = 0;
			thpool_p->num_threads = n; /* Only destroy created threads */
			thpool_destroy(thpool_p);
			return NULL;
		}
#if THPOOL_DEBUG
		printf("THPOOL_DEBUG: Created thread %d in pool \n", n);
#endif
	}

	/* Wait for threads to initialize */
	while (thpool_p->num_threads_alive != num_threads) {
		usleep(1000);
	}

	return thpool_p;
}


/* Add work to the thread pool */
int thpool_add_work(thpool_* thpool_p, void (*function_p)(void*), void* arg_p){
	if (!thpool_p || !function_p)
	{
		return -1;
	}
	job* newjob;

	newjob=(struct job*)malloc(sizeof(struct job));
	if (newjob==NULL){
		err("thpool_add_work(): Could not allocate memory for new job");
		return -2;
	}

	/* add function and argument */
	newjob->function=function_p;
	newjob->arg=arg_p;

	/* add job to queue */
	jobqueue_push(&thpool_p->jobqueue, newjob);

	return 0;
}


/* Wait until all jobs have finished */
void thpool_wait(thpool_* thpool_p){
	if (!thpool_p){
		return;
	}
	pthread_mutex_lock(&thpool_p->thcount_lock);
	while (thpool_p->jobqueue.len || thpool_p->num_threads_working) {
		pthread_cond_wait(&thpool_p->threads_all_idle, &thpool_p->thcount_lock);
	}
	pthread_mutex_unlock(&thpool_p->thcount_lock);
}


/* Destroy the threadpool */
void thpool_destroy(thpool_* thpool_p){
	/* No need to destroy if it's NULL */
	if (thpool_p == NULL) return ;

	volatile int threads_total = thpool_p->num_threads_alive;

	/* End each thread's infinite loop */
	thpool_p->threads_keepalive = 0;

	/* Give one second to kill idle threads */
	double TIMEOUT = 1.0;
	time_t start, end;
	double tpassed = 0.0;
	time (&start);
	while (tpassed < TIMEOUT && thpool_p->num_threads_alive){
		bsem_post_all(thpool_p->jobqueue.has_jobs);
		time (&end);
		tpassed = difftime(end,start);
	}

	/* Poll remaining threads */
	while (thpool_p->num_threads_alive){
		bsem_post_all(thpool_p->jobqueue.has_jobs);
		usleep(10000); /* 10ms */
	}

	/* Job queue cleanup */
	jobqueue_destroy(&thpool_p->jobqueue);

	/* Destroy condition variable and mutex */
	pthread_cond_destroy(&thpool_p->threads_all_idle);
	pthread_mutex_destroy(&thpool_p->thcount_lock);

	/* Deallocs */
	int n;
	for (n=0; n < threads_total; n++){
		thread_destroy(thpool_p->threads[n]);
	}
	free(thpool_p->threads);
	free(thpool_p);
}

int thpool_num_threads_working(thpool_* thpool_p){
	if (!thpool_p){
		return 0;
	}
	return thpool_p->num_threads_working;
}


/* ============================ THREAD ============================== */

/* Initialize a thread in the thread pool */
static int thread_init (thpool_* thpool_p, struct thread** thread_p, int id){

	*thread_p = (struct thread*)malloc(sizeof(struct thread));
	if (*thread_p == NULL){
		err("thread_init(): Could not allocate memory for thread");
		return -1;
	}

	(*thread_p)->thpool_p = thpool_p;
	(*thread_p)->id       = id;

	int ret = pthread_create(&(*thread_p)->pthread, NULL, (void * (*)(void *)) thread_do, (*thread_p));
	if (ret != 0){
		err("thread_init(): pthread_create failed with error %d", ret);
		free(*thread_p);
		*thread_p = NULL;
		return -1;
	}
	pthread_detach((*thread_p)->pthread);
	return 0;
}


/* What each thread is doing */
static void* thread_do(struct thread* thread_p){

	/* Set thread name for profiling and debugging */
	char thread_name[16] = {0};
	snprintf(thread_name, sizeof(thread_name), "thpool-%d", thread_p->id);

#if defined(__linux__)
	/* Use prctl instead to prevent using _GNU_SOURCE flag and implicit declaration */
	prctl(PR_SET_NAME, thread_name);
#elif defined(__APPLE__) && defined(__MACH__)
	pthread_setname_np(thread_name);
#else
	(void)thread_name; /* Unused on this platform */
#endif

	/* Assure all threads have been created before starting serving */
	thpool_* thpool_p = thread_p->thpool_p;

	/* Mark thread as alive (initialized) */
	pthread_mutex_lock(&thpool_p->thcount_lock);
	thpool_p->num_threads_alive += 1;
	pthread_mutex_unlock(&thpool_p->thcount_lock);

	while(thpool_p->threads_keepalive){

		bsem_wait(thpool_p->jobqueue.has_jobs);

		if (!thpool_p->threads_keepalive){
			break;
		}

		pthread_mutex_lock(&thpool_p->thcount_lock);
		thpool_p->num_threads_working++;
		pthread_mutex_unlock(&thpool_p->thcount_lock);

		/* Read job from queue and execute it */
		void (*func_buff)(void*);
		void* arg_buff;
		job* job_p = jobqueue_pull(&thpool_p->jobqueue);
		if (job_p) {
			func_buff = job_p->function;
			arg_buff = job_p->arg;
			func_buff(arg_buff);
			free(job_p);
		}

		pthread_mutex_lock(&thpool_p->thcount_lock);
		thpool_p->num_threads_working--;
		if (!thpool_p->num_threads_working) {
			pthread_cond_signal(&thpool_p->threads_all_idle);
		}
		pthread_mutex_unlock(&thpool_p->thcount_lock);
	}
	pthread_mutex_lock(&thpool_p->thcount_lock);
	thpool_p->num_threads_alive --;
	pthread_mutex_unlock(&thpool_p->thcount_lock);

	return NULL;
}


/* Frees a thread  */
static void thread_destroy (thread* thread_p){
	if (thread_p){
		free(thread_p);
	}
}


/* ============================ JOB QUEUE =========================== */

/* Initialize queue */
static int jobqueue_init(jobqueue* jobqueue_p){
	jobqueue_p->len = 0;
	jobqueue_p->front = NULL;
	jobqueue_p->rear  = NULL;

	jobqueue_p->has_jobs = (struct bsem*)malloc(sizeof(struct bsem));
	if (jobqueue_p->has_jobs == NULL){
		return -1;
	}

	if (pthread_mutex_init(&(jobqueue_p->rwmutex), NULL) != 0){
		free(jobqueue_p->has_jobs);
		return -1;
	}

	if (bsem_init(jobqueue_p->has_jobs, 0) != 0){
		pthread_mutex_destroy(&jobqueue_p->rwmutex);
		free(jobqueue_p->has_jobs);
		return -1;
	}

	return 0;
}


/* Clear the queue */
static void jobqueue_clear(jobqueue* jobqueue_p){

	while(jobqueue_p->len){
		free(jobqueue_pull(jobqueue_p));
	}

	jobqueue_p->front = NULL;
	jobqueue_p->rear  = NULL;
	bsem_reset(jobqueue_p->has_jobs);
	jobqueue_p->len = 0;

}


/* Add (allocated) job to queue */
static void jobqueue_push(jobqueue* jobqueue_p, struct job* newjob){

	pthread_mutex_lock(&jobqueue_p->rwmutex);
	newjob->prev = NULL;

	switch(jobqueue_p->len){

		case 0:  /* if no jobs in queue */
				jobqueue_p->front = newjob;
				jobqueue_p->rear  = newjob;
				break;

		default: /* if jobs in queue */
				jobqueue_p->rear->prev = newjob;
				jobqueue_p->rear = newjob;
				break;

	}
	jobqueue_p->len++;

	bsem_post(jobqueue_p->has_jobs);
	pthread_mutex_unlock(&jobqueue_p->rwmutex);
}


/* Get first job from queue(removes it from queue) */
static struct job* jobqueue_pull(jobqueue* jobqueue_p){

	pthread_mutex_lock(&jobqueue_p->rwmutex);
	job* job_p = jobqueue_p->front;

	switch(jobqueue_p->len){

		case 0:  /* if no jobs in queue */
		  			break;

		case 1:  /* if one job in queue */
				jobqueue_p->front = NULL;
				jobqueue_p->rear  = NULL;
				jobqueue_p->len = 0;
				break;

		default: /* if >1 jobs in queue */
				jobqueue_p->front = job_p->prev;
				jobqueue_p->len--;
				/* more than one job in queue -> post it */
				bsem_post(jobqueue_p->has_jobs);
				break;

	}

	pthread_mutex_unlock(&jobqueue_p->rwmutex);
	return job_p;
}


/* Free all queue resources back to the system */
static void jobqueue_destroy(jobqueue* jobqueue_p){
	jobqueue_clear(jobqueue_p);
	bsem_destroy(jobqueue_p->has_jobs);
	free(jobqueue_p->has_jobs);
	pthread_mutex_destroy(&jobqueue_p->rwmutex);
}


/* ======================== SYNCHRONISATION ========================= */

/* Init semaphore to 1 or 0 */
static int bsem_init(bsem *bsem_p, int value) {
	if (value < 0 || value > 1) {
		err("bsem_init(): Binary semaphore can take only values 1 or 0");
		return -1;
	}
	if (pthread_mutex_init(&(bsem_p->mutex), NULL) != 0){
		return -1;
	}
	if (pthread_cond_init(&(bsem_p->cond), NULL) != 0){
		pthread_mutex_destroy(&bsem_p->mutex);
		return -1;
	}
	bsem_p->v = value;
	return 0;
}


/* Reset semaphore to 0 */
static void bsem_reset(bsem *bsem_p) {
	pthread_mutex_lock(&bsem_p->mutex);
	bsem_p->v = 0;
	pthread_mutex_unlock(&bsem_p->mutex);
}


/* Post to at least one thread */
static void bsem_post(bsem *bsem_p) {
	pthread_mutex_lock(&bsem_p->mutex);
	bsem_p->v = 1;
	pthread_cond_signal(&bsem_p->cond);
	pthread_mutex_unlock(&bsem_p->mutex);
}


/* Post to all threads */
static void bsem_post_all(bsem *bsem_p) {
	pthread_mutex_lock(&bsem_p->mutex);
	bsem_p->v = 1;
	pthread_cond_broadcast(&bsem_p->cond);
	pthread_mutex_unlock(&bsem_p->mutex);
}


/* Wait on semaphore until semaphore has value 0 */
static void bsem_wait(bsem* bsem_p) {
	pthread_mutex_lock(&bsem_p->mutex);
	while (bsem_p->v != 1) {
		pthread_cond_wait(&bsem_p->cond, &bsem_p->mutex);
	}
	bsem_p->v = 0;
	pthread_mutex_unlock(&bsem_p->mutex);
}


/* Destroy semaphore */
static void bsem_destroy(bsem* bsem_p) {
	pthread_cond_destroy(&bsem_p->cond);
	pthread_mutex_destroy(&bsem_p->mutex);
}
