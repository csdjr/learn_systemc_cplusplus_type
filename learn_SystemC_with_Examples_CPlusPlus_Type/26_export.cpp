// Learn with Examples, 2020, MIT license
#include <systemc>
#include "utils/single_file_log.h"

using namespace std;
using namespace sc_core;

class MyModule1 : public sc_module {
public:
	SC_HAS_PROCESS(MyModule1);
	MyModule1(sc_module_name name)
	    : sc_module(name) {
		p.bind(s);
		SC_THREAD(writer);
	}

public:
	sc_export<sc_signal<int>> p;
	sc_signal<int> s;

private:
	void writer() {
		int i = 0;
		while (true) {
			p->write(i++); // IF* operator -> (); calls the write method of the outside channel.
			wait(1, SC_SEC);
		}
	}
};

class MyModule2 : public sc_module {
public:
	SC_HAS_PROCESS(MyModule2);
	MyModule2(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(reader);
	}

public:
	sc_port<sc_signal_in_if<int>> p;
	// sc_export<sc_signal<int>> p; //写成这样,程序仍旧OK，可以跑过

private:
	void
	reader() {
		int i;
		while (true) {
			wait(p->default_event()); // IF* operator -> (); calls the write method of the outside channel.
			i = p->read();
			LOG(DEBUG) << sc_time_stamp() << " reader receive " << i;
		}
	}
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModule1 m1("m1");
	MyModule2 m2("m2");
	m2.p.bind(m1.p); // port which has pointer of IF, bind port which has already bind channel that has implemented IF.
	sc_start(8, SC_SEC);
	LOG(INFO) << "Sim Completed.";
	return 0;
}