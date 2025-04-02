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
		SC_THREAD(producer1);
		SC_THREAD(producer2);
		SC_THREAD(consumer);
	}

private:
	void producer1() {
		int v = 1;
		while (true) {
			s1.write(v++);
			wait(2, SC_SEC);
		}
	}

	void producer2() {
		int v = 1;
		while (true) {
			s2 = v++;
			wait(3, SC_SEC);
		}
	}

	void consumer() {
		while (true) {
			wait(s1.default_event() | s2.default_event()); // note: wait的对象是一个sc_event. 把default_event()去掉,编译通过，运行失败.
			if (s1.event() == true && s2.event() == true)
				LOG(DEBUG) << sc_time_stamp() << " s1&s2 triggered." << endl;
			else if (s1.event() == true)
				LOG(DEBUG) << sc_time_stamp() << " s1 triggered." << endl;
			else
				LOG(DEBUG) << sc_time_stamp() << " s2 triggered." << endl;
		}
	}

private:
	sc_signal<int> s1, s2;
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModule m("m");
	sc_start(10, SC_SEC);
	return 0;
}