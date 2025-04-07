#include <systemc>
#include "utils/single_file_log.h"

using namespace sc_core;
using namespace std;

class MyIF : public sc_interface {
public:
	virtual int read(int idx) = 0;
};

// ------------------ Port ------------------
class MyPort : public sc_port<MyIF> {
public:
	MyPort()
	    : sc_port() {}
};

// ------------------ Export ------------------
class FaExport : public sc_export<MyIF> {
public:
	FaExport()
	    : sc_export() {}
};

class ChildExport : public sc_export<MyIF> {
public:
	ChildExport()
	    : sc_export() {}
};

// ------------------ Channel ------------------
class Channel : public MyIF {
public:
	virtual int read(int idx) {
		LOG(DEBUG) << "read index:" << idx << " return index+1=" << idx + 1;
		return idx + 1;
	}
};

// ------------------ Module ------------------
class ChildModule : public sc_module {
public:
	ChildModule(sc_module_name name)
	    : sc_module(name) {
		childEp.bind(chnl); // 下层sc_export绑定chnl
	}

public:
	ChildExport childEp;

private:
	Channel chnl;
};

class FaModuel : public sc_module {
public:
	SC_HAS_PROCESS(FaModuel);
	FaModuel(sc_module_name name)
	    : sc_module(name)
	    , childM((string(name) + string("childM")).c_str()) {
		faEp.bind(childM.childEp); // 上层sc_export绑定下层sc_export
		SC_THREAD(read_thread);
	}

private:
	void read_thread() {
		for (int i = 0; i < 10; ++i) {
			int val = faEp->read(i);
			LOG(DEBUG) << this->name() << " read val:" << val;
		}
	}

public:
	FaExport faEp;

private:
	ChildModule childM;
};

// ------------------------------------
int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	FaModuel fa_m{"faM"};
	sc_start();
	return 0;
}