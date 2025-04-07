#include <systemc>
#include "utils/single_file_log.h"

using namespace std;
using namespace sc_core;

class MyIfA : public sc_interface {
public:
	virtual int read() = 0;
	virtual const sc_event& default_event() const = 0;
};

class MyIfB : public sc_interface {
public:
	virtual void write(int val) = 0;
};

// ------------------ Channle ------------------
class Channle : public MyIfA,
                public MyIfB {
public:
	virtual int read() override {
		LOG(DEBUG) << "read val:" << val;
		return val;
	}
	virtual void write(int val) override {
		LOG(DEBUG) << "write val:" << val;
		this->val = val;
		e.notify();
	}
	virtual const sc_event& default_event() const override {
		return e;
	}

private:
	int val = 0;
	sc_event e;
};

// ------------------ Module ------------------
class MyModuleA : public sc_module {
public:
	SC_HAS_PROCESS(MyModuleA);
	MyModuleA(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(read_thread);
	}

	void read_thread() {
		while (true) {
			wait(p->default_event());
			p->read(); // port调用channel读
		}
	}

public:
	sc_port<MyIfA> p;
};

class MyModuleB : public sc_module {
public:
	SC_HAS_PROCESS(MyModuleB);
	MyModuleB(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(write_thread);
	}

	void write_thread() {
		for (int i = 0; i < 10; ++i) {
			ep->write(i); // export调用channel写
			wait(1, SC_SEC);
		}
	}

public:
	sc_export<MyIfB> ep;
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModuleA ma("ma");
	MyModuleB mb("mb");
	Channle chnl;
	ma.p.bind(chnl);
	mb.ep.bind(chnl);
	sc_start();
	return 0;
}