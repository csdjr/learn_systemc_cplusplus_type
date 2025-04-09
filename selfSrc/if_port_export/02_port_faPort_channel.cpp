#include <systemc>
#include "utils/single_file_log.h"

using namespace sc_core;
using namespace std;

class MyIF : public sc_interface {
public:
	virtual void wrt(int val) = 0;
};

// ------------------ Port ------------------
class SubPort : public sc_port<MyIF> {
public:
	SubPort(const char* name)
	    : sc_port<MyIF>(name) {}
	virtual const char* kind() const override { return "SubPort"; }
};

class FaPort : public sc_port<MyIF> {
public:
	FaPort(const char* name)
	    : sc_port<MyIF>(name) {}
	virtual const char* kind() const override { return "FaPort"; }
};

// ------------------ Channel ------------------
class Chnl : public MyIF {
public:
	virtual void wrt(int val) override {
		LOG(DEBUG) << "wrt val = " << val;
	}
};

// ------------------ Module ------------------
class SubM : public sc_module {
public:
	SC_HAS_PROCESS(SubM);
	SubM(sc_module_name name)
	    : sc_module(name)
	    , sub_p("sub_p") {
		SC_THREAD(wrt_thread);
	}

private:
	void wrt_thread() {
		for (int i = 0; i < 10; ++i) {
			sub_p->wrt(i); // 子Module的sc_port，调用sc_interface方法
			wait(1, SC_SEC);
		}
	}

public:
	SubPort sub_p;
};

class FaM : public sc_module {
public:
	FaM(sc_module_name name)
	    : sc_module(name)
	    , fa_p("fa_p")
	    , sub_m("sub_m") {
		sub_m.sub_p.bind(fa_p); // 子Module的sc_port，绑定父Module的sc_port
	}

public:
	FaPort fa_p;

private:
	SubM sub_m;
};

// ------------------------------------
int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	FaM fa_m{"fa_m"};
	Chnl chnl;
	fa_m.fa_p.bind(chnl); // 父Module的sc_port，绑定channel
	sc_start();
	LOG(INFO) << "Sim Completed.";
	return 0;
}