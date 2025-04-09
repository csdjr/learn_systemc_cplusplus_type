// Learn with Examples, 2020, MIT license
#include <systemc>
#include "utils/single_file_log.h"

using namespace sc_core;

SC_MODULE(FIFO) {
  sc_fifo<int> f1, f2, f3;
  SC_CTOR(FIFO) : f1(2), f2(2), f3(2) { // fifo with size 2
    SC_THREAD(generator1);
    SC_THREAD(consumer1);

    SC_THREAD(generator2);
    SC_THREAD(consumer2);

    SC_THREAD(generator3);
    SC_THREAD(consumer3);
  }
  void generator1() { // blocking write
    int v = 0;
    while (true) {
      f1.write(v); // same as f = v, which is not recommended.
      LOG(DEBUG) << sc_time_stamp() << ": generator1 writes " << v++;
      wait(1, SC_SEC); // write every 1 s
    }
  }
  void consumer1() { // blocking read
    int v = -1;
    while (true) {
      f1.read(v); // same as v = int(f), which is not recommended; or, v = f1.read();
      LOG(DEBUG) << sc_time_stamp() << ": consumer1 reads " << v;
      wait(3, SC_SEC); // read every 3 s, fifo will fill up soon
    }
  }
  void generator2() { // non-blocking write
    int v = 0;
    while (true) {
      while (f2.nb_write(v) == false ) { // nb write until succeeded
        wait(f2.data_read_event()); // if not successful, wait for data read (a fifo slot becomes available)
      }
      LOG(DEBUG) << sc_time_stamp() << ": generator2 writes " << v++;
      wait(1, SC_SEC); // write every 1 s
    }
  }
  void consumer2() { // non-blocking read
    int v = -1;
    while (true) {
      while (f2.nb_read(v) == false) {
        wait(f2.data_written_event());
      }
      LOG(DEBUG) << sc_time_stamp() << ": consumer2 reads " << v;
      wait(3, SC_SEC); // read every 3 s, fifo will fill up soon
    }
  }
  void generator3() { // free/available slots before/after write
    int v = 0;
    while (true) {
      LOG(DEBUG) << sc_time_stamp() << ": generator3, before write, #free/#available=" << f3.num_free() << "/" << f3.num_available();
      f3.write(v++);
      LOG(DEBUG) << sc_time_stamp() << ": generator3, after write, #free/#available=" << f3.num_free() << "/" << f3.num_available();
      wait(1, SC_SEC);
    }
  }
  void consumer3() { // free/available slots before/after read
    int v = -1;
    while (true) {
      LOG(DEBUG) << sc_time_stamp() << ": consumer3, before read, #free/#available=" << f3.num_free() << "/" << f3.num_available();
      f3.read(v);
      LOG(DEBUG) << sc_time_stamp() << ": consumer3, after read, #free/#available=" << f3.num_free() << "/" << f3.num_available();
      wait(3, SC_SEC); // read every 3 s, fifo will fill up soon
    }
  }
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
  FIFO fifo("fifo");
  sc_start(10, SC_SEC);
  LOG(INFO) << "Sim Completed.";
	return 0;
}
