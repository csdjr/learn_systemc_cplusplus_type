// Learn with Examples, 2020, MIT license

#include <systemc>
#include "utils/single_file_log.h"

using namespace std;
using namespace sc_core;

class MyModule : public sc_module {
public:
	SC_HAS_PROCESS(MyModule);
	MyModule(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(thread1);
		SC_THREAD(thread2);
	}

private:
	void thread1() {
		while (true) {
			if (m.trylock() == -1) {
				m.lock();
				LOG(DEBUG) << sc_time_stamp() << " thread1 obtain mutex by lock()" << endl;
			} else {
				LOG(DEBUG) << sc_time_stamp() << " thread1 obtain mutex by trylock()" << endl;
			}
			wait(1, SC_SEC);
			m.unlock();
			LOG(DEBUG) << sc_time_stamp() << " thread1 release mutex by unlock()" << endl;
			wait(SC_ZERO_TIME);
		}
	}
	void thread2() {
		while (true) {
			if (m.trylock() == -1) {
				m.lock();
				LOG(DEBUG) << sc_time_stamp() << " thread2 obtain mutex by lock()" << endl;
			} else {
				LOG(DEBUG) << sc_time_stamp() << " thread2 obtain mutex by trylock()" << endl;
			}
			wait(1, SC_SEC);
			m.unlock();
			LOG(DEBUG) << sc_time_stamp() << " thread2 release mutex by unlock()" << endl;
			wait(SC_ZERO_TIME);
		}
	}

private:
	sc_mutex m;
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModule mm("mm");
	sc_start(4, SC_SEC);
	return 0;
}