// Learn with Examples, 2020, MIT license
#include <systemc>

using namespace std;
using namespace sc_core;

// IF
class generator_if : public sc_interface {
public:
	virtual void notify() = 0;
};

class receive_if : public sc_interface {
public:
	virtual const sc_event& default_event() const = 0; // sc_interface has this method //是否是为了解决菱形继承问题？
};

//----------------------------------------------------------------------------
// channel
class InterruptChannel : public sc_prim_channel,
                         public generator_if,
                         public receive_if {
public:
	InterruptChannel(sc_module_name name)
	    : sc_prim_channel(name) {}
	// 实现IF
	virtual void notify() override {
		e.notify();
	}
	virtual const sc_event& default_event() const override {
		return e;
	}

private:
	sc_event e;
};

//----------------------------------------------------------------------------
// 使用port<IF>的类
class GenInterrupt : public sc_module {
public:
	SC_HAS_PROCESS(GenInterrupt);
	GenInterrupt(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(rcv_interrupt);
	}

public:
	sc_port<receive_if> p;

private:
	void rcv_interrupt() {
		while (true) {
			wait(p->default_event());
			cout << sc_time_stamp() << " receive interrupt" << endl;
		}
	}
};

class Receiver : public sc_module {
public:
	SC_HAS_PROCESS(Receiver);
	Receiver(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(gen_interrupt);
	}

public:
	sc_port<generator_if> p;

private:
	void gen_interrupt() {
		while (true) {
			p->notify();
			wait(1, SC_SEC);
		}
	}
};

int sc_main(int, char*[]) {
	InterruptChannel intrChannel("InterruptChannel");
	GenInterrupt gen("gen");
	Receiver rcv("rcv");
	gen.p(intrChannel);
	rcv.p(intrChannel);
	sc_start(8, SC_SEC);
	return 0;
}
