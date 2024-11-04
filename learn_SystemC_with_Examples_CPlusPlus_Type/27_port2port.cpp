// Learn with Examples, 2020, MIT license
#include <systemc>

using namespace std;
using namespace sc_core;

class SubModule1 : public sc_module {
public:
	SC_HAS_PROCESS(SubModule1);
	SubModule1(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(writer);
	}

public:
	sc_port<sc_signal_out_if<int>> p;

private:
	void writer() {
		int i = 0;
		while (true) {
			p->write(i++); // IF* operator -> (); calls the write method of the outside channel.
			wait(1, SC_SEC);
		}
	}
};

class SubModule2 : public sc_module {
public:
	SC_HAS_PROCESS(SubModule2);
	SubModule2(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(reader);
	}

public:
	sc_port<sc_signal_in_if<int>> p;

private:
	void
	reader() {
		int i;
		while (true) {
			wait(p->default_event()); // IF* operator -> (); calls the write method of the outside channel.
			i = p->read();
			cout << sc_time_stamp() << " reader receive " << i << endl;
		}
	}
};

class MyModule1 : public sc_module {
public:
	SC_HAS_PROCESS(MyModule1);
	MyModule1(sc_module_name name)
	    : sc_module(name) {
		subM1.p.bind(p);
	}

public:
	sc_port<sc_signal_out_if<int>> p;

private:
	SubModule1 subM1{"subM1"};
};

class MyModule2 : public sc_module {
public:
	SC_HAS_PROCESS(MyModule2);
	MyModule2(sc_module_name name)
	    : sc_module(name) {
		subM2.p.bind(p);
	}

public:
	sc_port<sc_signal_in_if<int>> p;

private:
	SubModule2 subM2{"subM2"};
};

int sc_main(int, char*[]) {
	MyModule1 m1("m1");
	MyModule2 m2("m2");
	sc_signal<int> s;
	m1.p(s);
	m2.p(s);
	sc_start(8, SC_SEC);
	return 0;
}

// SubModule1.p bind MyModule1.p
// SubModule2.p bind MyModule2.p
// MyModule1.p bind sc_signal
// MyModule2.p bind sc_signal

// Summary: port bind channel