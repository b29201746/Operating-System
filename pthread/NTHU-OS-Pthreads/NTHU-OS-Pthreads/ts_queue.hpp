#include <pthread.h>
#include <cstdlib>
#include <stdlib.h>

#ifndef TS_QUEUE_HPP
#define TS_QUEUE_HPP

#define DEFAULT_BUFFER_SIZE 200

template <class T>
class TSQueue
{
public:
	// constructor
	TSQueue();

	explicit TSQueue(int max_buffer_size);

	// destructor
	~TSQueue();

	// add an element to the end of the queue
	void enqueue(T item);

	// remove and return the first element of the queue
	T dequeue();

	// return the number of elements in the queue
	int get_size();

private:
	// the maximum buffer size
	int buffer_size;
	// the buffer containing values of the queue
	T *buffer;
	// the current size of the buffer
	int size;
	// the index of first item in the queue
	int head;
	// the index of last item in the queue
	int tail;

	// pthread mutex lock
	pthread_mutex_t mutex;
	// pthread conditional variable
	pthread_cond_t cond_enqueue, cond_dequeue;
};

// Implementation start

template <class T>
TSQueue<T>::TSQueue() : TSQueue(DEFAULT_BUFFER_SIZE)
{
}

template <class T>
TSQueue<T>::TSQueue(int buffer_size) : buffer_size(buffer_size)
{
	// TODO: implements TSQueue constructor
	buffer = (T*) malloc(buffer_size * sizeof(T));
	head = 0;
	tail = 0;
	size = 0;
}

template <class T>
TSQueue<T>::~TSQueue()
{
	// TODO: implenents TSQueue destructor
	//free(buffer);
	//delete(this);
}

template <class T>
void TSQueue<T>::enqueue(T item)
{
	// TODO: enqueues an element to the end of the queue

	pthread_mutex_lock(&mutex);

	while (size == buffer_size)
	{
		pthread_cond_wait(&cond_enqueue, &mutex);
	}

	buffer[tail] = item;
	if(tail == buffer_size)
		tail = 0;
	else
		tail++;
	size++;

	if(size != 0)	
		pthread_cond_signal(&cond_dequeue);

	pthread_mutex_unlock(&mutex);

}

template <class T>
T TSQueue<T>::dequeue()
{
	// TODO: dequeues the first element of the queue
	pthread_mutex_lock(&mutex);

	while (size == 0)
	{
		pthread_cond_wait(&cond_dequeue, &mutex);
	}

	T item = buffer[head];

	if(head != buffer_size)
		head++;
	else 
		head = 0;
	size--;

	if(size != buffer_size)
		pthread_cond_signal(&cond_enqueue);

	pthread_mutex_unlock(&mutex);

	return item;
}

template <class T>
int TSQueue<T>::get_size()
{
	// TODO: returns the size of the queue
	return size;
}

#endif // TS_QUEUE_HPP
