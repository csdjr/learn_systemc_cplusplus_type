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
		SC_THREAD(thread);
		LOG(DEBUG) << "MyModule constructed" << endl;
	}

public:
	// called by construction_done
	virtual void before_end_of_elaboration() override {
		LOG(DEBUG) << "before_end_of_elaboration" << endl;
	}

	// called by elaboration_done (does nothing by default)
	virtual void end_of_elaboration() override {
		LOG(DEBUG) << "end_of_elaboration" << endl;
	}

	// called by start_simulation (does nothing by default)
	virtual void start_of_simulation() override {
		LOG(DEBUG) << "start_of_simulation" << endl;
	}

	// need call api sc_stop, then end_of_simulation will action.
	// called by simulation_done (does nothing by default)
	virtual void end_of_simulation() override {
		LOG(DEBUG) << "end_of_simulation" << endl;
	}

private:
	void thread() {
		LOG(DEBUG) << sc_time_stamp() << " Execution.initialization" << endl;
		while (true) {
			LOG(DEBUG) << sc_time_stamp() << " Execution.simulation" << endl;
			wait(1, SC_SEC);
		}
	}
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModule m("m");
	sc_start(4, SC_SEC);
	return 0;
}