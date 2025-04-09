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
		SC_THREAD(readWrite);
	}

private:
	void readWrite() {
		s.write(3);
		LOG(DEBUG) << "s = " << s << ";" << s.read();
		wait(SC_ZERO_TIME);
		LOG(DEBUG) << "after delta_cycle,s = " << s;
		LOG(DEBUG) << "----------------------------";
		s = 4;
		s = 5;
		int tmp = s;
		LOG(DEBUG) << "s = " << tmp;
		wait(SC_ZERO_TIME);
		LOG(DEBUG) << "after delta_cycle,s = " << s;
	}

private:
	sc_signal<int> s{"s", -1};
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModule m("m");
	sc_start();
	LOG(INFO) << "Sim Completed.";
	return 0;
}