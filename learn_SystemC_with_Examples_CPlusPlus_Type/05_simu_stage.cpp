// Learn with Examples, 2020, MIT license

#include <systemc>

using namespace std;
using namespace sc_core;

class MyModule : public sc_module {
public:
	SC_HAS_PROCESS(MyModule);
	MyModule(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(thread);
		cout << "MyModule constructed" << endl;
	}

public:
	// called by construction_done
	virtual void before_end_of_elaboration() override {
		cout << "before_end_of_elaboration" << endl;
	}

	// called by elaboration_done (does nothing by default)
	virtual void end_of_elaboration() override {
		cout << "end_of_elaboration" << endl;
	}

	// called by start_simulation (does nothing by default)
	virtual void start_of_simulation() override {
		cout << "start_of_simulation" << endl;
	}

	// need call api sc_stop, then end_of_simulation will action.
	// called by simulation_done (does nothing by default)
	virtual void end_of_simulation() override {
		cout << "end_of_simulation" << endl;
	}

private:
	void thread() {
		cout << sc_time_stamp() << " Execution.initialization" << endl;
		while (true) {
			cout << sc_time_stamp() << " Execution.simulation" << endl;
			wait(1, SC_SEC);
		}
	}
};

int sc_main(int, char*[]) {
	MyModule m("m");
	sc_start(4, SC_SEC);
	return 0;
}