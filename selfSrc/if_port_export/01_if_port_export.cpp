#include <systemc>
#include "utils/single_file_log.h"

using namespace sc_core;
using namespace std;

// IF
class MyIF : public sc_interface {
public:
	virtual void write(int val) = 0;
};

// port
class MyPort : public sc_port<MyIF, 0> { // one or more
public:
	MyPort(const char* name)
	    : sc_port<MyIF, 0>(name) {}
	virtual const char* kind() const override { return "MyPort"; }
};

// export and implement IF
class MyExport : public sc_export<MyIF>,
                 public MyIF {

public:
	MyExport(const char* name)
	    : sc_export<MyIF>(name) {
		sname = name;
		this->bind(*this);
	}
	virtual const char* kind() const override { return "MyExport"; }
	virtual void write(int val) override {
		LOG(DEBUG) << sname << ":" << val;
	}

private:
	string sname;
};

//==================================================
// class A has MyPort
class A : public sc_module {
public:
	SC_HAS_PROCESS(A);
	A(sc_module_name name)
	    : sc_module(name)
	    , p(name) {
		SC_THREAD(writer);
	}

public:
	MyPort p;

private:
	void writer() {
		int idx = 0;
		while (true) {
			for (int i = 0; i < p.size(); i++)
				p[i]->write(idx); // IF* operator -> (); calls the write method of the outside channel.
			wait(1, SC_SEC);
			idx++;
		}
	}
};

// class B has MyExport
class B : public sc_module {
public:
	SC_HAS_PROCESS(B);
	B(sc_module_name name)
	    : sc_module(name)
	    , p(name) {}

public:
	MyExport p;
};

//==================================================
int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	A a("a");
	B b1("b1"), b2("b2");
	a.p.bind(b1.p);
	a.p.bind(b2.p);
	sc_start(8, SC_SEC);
	LOG(INFO) << "Sim Completed.";
	return 0;
}
