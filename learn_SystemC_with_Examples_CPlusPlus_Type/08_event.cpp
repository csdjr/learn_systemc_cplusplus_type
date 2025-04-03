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
		SC_THREAD(trigger);
		SC_THREAD(catcher);
	}

private:
	void trigger() {
		while (true) {
			e.notify(1, SC_SEC);
			if (sc_time_stamp() == sc_time(4, SC_SEC))
				e.cancel(); // cancel the event triggered at time = 4s
			wait(2, SC_SEC);
		}
	}

	void catcher() {
		while (true) {
			wait(e); // cant`t receive at time = 5s
			LOG(DEBUG) << "Event catch at " << sc_time_stamp();
		}
	}

private:
	sc_event e;
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModule m("m");
	sc_start(8, SC_SEC);
	return 0;
}