#include <assert.h>
#include <stdlib.h>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"

#define READER_QUEUE_SIZE 200
#define WORKER_QUEUE_SIZE 200
#define WRITER_QUEUE_SIZE 4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 1000000

int main(int argc, char** argv) {
	assert(argc == 4);

	int n = atoi(argv[1]);
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);

	// TODO: implements main function
	TSQueue<Item*>* Input_Queue;
	TSQueue<Item*>* Worker_Queue;
	TSQueue<Item*>* Writer_Queue;

	Input_Queue = new TSQueue<Item*>;
	Worker_Queue = new TSQueue<Item*>;
	Writer_Queue = new TSQueue<Item*>;

	Transformer* transformer = new Transformer;

	Reader* reader = new Reader(n, input_file_name, Input_Queue);
	Writer* writer = new Writer(n, output_file_name, Writer_Queue);

	Producer* producer_1 = new Producer(Input_Queue, Worker_Queue, transformer);
	Producer* producer_2 = new Producer(Input_Queue, Worker_Queue, transformer);
	Producer* producer_3 = new Producer(Input_Queue, Worker_Queue, transformer);
	Producer* producer_4 = new Producer(Input_Queue, Worker_Queue, transformer);

	int low_threshold = WORKER_QUEUE_SIZE * CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE * 0.01;
	int high_threshold = WORKER_QUEUE_SIZE * CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE * 0.01;
	ConsumerController* controller = new ConsumerController(Worker_Queue, Writer_Queue, transformer, CONSUMER_CONTROLLER_CHECK_PERIOD, low_threshold, high_threshold);

	reader->start();
	writer->start();

	controller->start();

	producer_1->start();
	producer_2->start();
	producer_3->start();
	producer_4->start();

	reader->join();
	writer->join();

	delete producer_1;
	delete producer_2;
	delete writer;
	delete reader;
	delete transformer;
	delete Input_Queue;
	delete Worker_Queue;
	delete Writer_Queue;
	delete controller;

	return 0;
}
