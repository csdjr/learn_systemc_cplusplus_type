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
		SC_THREAD(catcher1);
		SC_THREAD(catcher2);
		SC_THREAD(catcher3);
	}

private:
	void trigger() {
		e1.notify(1, SC_SEC);
		e2.notify(2, SC_SEC);
		e3.notify(3, SC_SEC);
	}

	void catcher1() {
		wait(e1 | e2);
		LOG(DEBUG) << sc_time_stamp() << " catcher1" << endl; //  catch e1
	}

	void catcher2() {
		wait(e1 & e2);
		LOG(DEBUG) << sc_time_stamp() << " catcher2" << endl; //  catch e1 & e2
	}

	void catcher3() {
		wait(sc_time(2, SC_SEC), e3);
		LOG(DEBUG) << sc_time_stamp()
		     << " catcher3: " << (e3.triggered() ? "e3" : "timeout") << endl;
	}

private:
	sc_event e1, e2, e3, e4;
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModule m("m");
	sc_start(4, SC_SEC);
	return 0;
}