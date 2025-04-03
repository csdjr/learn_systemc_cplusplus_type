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
		b = -1;
		s = -1;
		SC_THREAD(writer);
		SC_THREAD(reader_buffer);
		SC_THREAD(reader_signal);
	}

private:
	void writer() {
		int i = 0;
		while (true) {
			b = i / 2;
			s = i / 2;
			i++;
			wait(1, SC_SEC);
		}
	}
	void reader_buffer() {
		while (true) {
			wait(b.default_event());
			LOG(DEBUG) << sc_time_stamp() << " reader_buffer receive " << b;
		}
	}
	void reader_signal() {
		while (true) {
			wait(s.default_event());
			LOG(DEBUG) << "---- " << sc_time_stamp() << " reader_signal receive " << s;
		}
	}

private:
	sc_buffer<int> b; // 多次写入同一个值，default_event触发多次
	sc_signal<int> s; // 多次写入同一个值，default_event只触发一次
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModule m("m");
	sc_start(8, SC_SEC);
	return 0;
}