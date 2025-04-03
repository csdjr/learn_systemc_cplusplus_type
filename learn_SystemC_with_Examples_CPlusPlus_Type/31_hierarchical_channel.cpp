// Learn with Examples, 2020, MIT license
#include <systemc>
#include "utils/single_file_log.h"

using namespace std;
using namespace sc_core;

class MyChannel : public sc_channel,
                  public sc_signal_inout_if<int> {
public:
	SC_HAS_PROCESS(MyChannel);
	MyChannel(sc_module_name name)
	    : sc_channel(name) {}
	// 实现IF
	void write(const int& v) override { // implements write method
		if (v != m_val) {               // update only if value is new
			m_val = v;                  // update value
			e.notify();                 // trigger event
		}
	}
	const int& read() const override {
		return m_val;
	}
	const sc_event& value_changed_event() const override {
		return e; // return reference to the event
	}
	const sc_event& default_event() const override {
		return value_changed_event(); // allows used in static sensitivity list
	}
	const int& get_data_ref() const override {
		return m_val;
	}
	bool event() const override {
		return true; // dummy implementation, always return true
	}

private:
	int m_val = 0;
	int last_val = 0;
	sc_event e;
};

// 使用channel的类

class MyModule : public sc_module {
public:
	SC_HAS_PROCESS(MyModule);
	MyModule(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(writer);
		SC_THREAD(reader);
	}

private:
	void writer() {
		int i = 0;
		while (true) {
			c.write(i++);
			wait(1, SC_SEC);
		}
	}
	void reader() {
		while (true) {
			wait(c.default_event());
			LOG(DEBUG) << sc_time_stamp() << " reader receive " << c.read();
		}
	}

private:
	MyChannel c{"c"};
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	MyModule m("m");
	sc_start(8, SC_SEC);
	return 0;
}