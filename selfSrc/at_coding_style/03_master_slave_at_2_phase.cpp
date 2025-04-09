#include <set>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/peq_with_get.h>
#include "utils/single_file_log.h"

using namespace std;
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

using TRANS = tlm_generic_payload;
using PHASE = tlm_phase;

//========================================================TransIdExt===============================================================
class TransIdExt : public tlm_extension<TransIdExt> {
public:
	virtual tlm_extension_base* clone() const override {
		TransIdExt* ext = new TransIdExt();
		ext->trans_id = this->trans_id;
		return ext;
	}
	virtual void copy_from(tlm_extension_base const& other) override {
		trans_id = reinterpret_cast<const TransIdExt*>(&other)->trans_id;
	}

public:
	int trans_id;
};

//========================================================simple_mm===============================================================
class simple_mm : public tlm_mm_interface {
public:
	simple_mm() {}
	TRANS* acquire_trans(int trans_id) {
		TRANS* trans = new TRANS();
		trans->set_mm(this);
		TransIdExt* ext = new TransIdExt();
		ext->trans_id = trans_id;
		trans->set_extension<TransIdExt>(ext);
		trans->acquire();
		return trans;
	}

	virtual void free(TRANS* trans) override {
		TransIdExt* ext = trans->get_extension<TransIdExt>();
		LOG(DEBUG) << "mm free trans:" << trans << " trans_id:" << ext->trans_id;
		delete(trans);
	}
};

simple_mm mm;
//========================================================utils===============================================================
string trans_id(TRANS* trans) {
	return string("trans_id:") + to_string(trans->get_extension<TransIdExt>()->trans_id) + " ";
}
string trans_id(TRANS& trans) {
	return string("trans_id:") + to_string(trans.get_extension<TransIdExt>()->trans_id) + " ";
}
#define DBGLog LOG(DEBUG) << trans_id(trans)

//========================================================AT_Master===============================================================
class AT_Master : public sc_module {
public:
	SC_HAS_PROCESS(AT_Master);
	AT_Master(sc_module_name name);

	void initiator_thread();
	tlm_sync_enum nb_transport_bw(TRANS& trans, PHASE& phase, sc_time& t);

public:
	simple_initiator_socket<AT_Master> initiator_socket;

private:
	set<TRANS*> m_waitting_bw_path_trans;
};

AT_Master::AT_Master(sc_module_name name)
    : sc_module(name) {
	initiator_socket.register_nb_transport_bw(this, &AT_Master::nb_transport_bw);
	SC_THREAD(initiator_thread);
}

void AT_Master::initiator_thread() {
	for (int i = 0; i < 10; ++i) {
		TRANS* trans = mm.acquire_trans(i);
		trans->set_address(i);
		PHASE phase = BEGIN_REQ;
		sc_time t = SC_ZERO_TIME;
		DBGLog << "Master给Slave发送BEGIN_REQ";
		m_waitting_bw_path_trans.insert(trans);
		tlm_sync_enum ret = initiator_socket->nb_transport_fw(*trans, phase, t);
		if (ret == TLM_UPDATED && phase == END_REQ) {
			DBGLog << "TLM_UPDATED, END_REQ, t.";
			wait(t);
		} else {
			LOG(ERROR) << "2 phase: ret must be TLM_UPDATED, phase must be END_REQ";
			assert(false);
		}
	}
}

tlm_sync_enum AT_Master::nb_transport_bw(TRANS& trans, PHASE& phase, sc_time& t) {
	if (phase == BEGIN_RESP) {
		if (m_waitting_bw_path_trans.find(&trans) == m_waitting_bw_path_trans.end()) {
			LOG(ERROR) << "trans:" << &trans << " trans_id:" << trans.get_extension<TransIdExt>()->trans_id << " is not a outstading trans.";
			assert(false);
		}
		wait(t);
		DBGLog << "get_address:0x" << hex << trans.get_address();
		t = SC_ZERO_TIME;
		DBGLog << "Master bw返回 trans, END_RESP, t";
		m_waitting_bw_path_trans.erase(&trans);
		return TLM_COMPLETED;
	} else {
		LOG(ERROR) << "2 phase: bw phase must be BEGIN_RESP";
		assert(false);
	}
}

//========================================================AT_Slave===============================================================
class AT_Slave : public sc_module {
public:
	SC_HAS_PROCESS(AT_Slave);
	AT_Slave(sc_module_name name);

	tlm_sync_enum nb_transport_fw(TRANS& trans, PHASE& phase, sc_time& t);
	void begin_resp_thread();

public:
	simple_target_socket<AT_Slave> target_socket;

private:
	peq_with_get<TRANS> m_begin_resp_peq{"m_begin_resp_peq"};
	const sc_time DO_END_REQ_FUNC_TIME = sc_time(10, SC_NS);
	const sc_time DO_BEGIN_RESP_FUNC_TIME = sc_time(10, SC_NS);
};

AT_Slave::AT_Slave(sc_module_name name)
    : sc_module(name) {
	target_socket.register_nb_transport_fw(this, &AT_Slave::nb_transport_fw);
	SC_THREAD(begin_resp_thread);
}

tlm_sync_enum AT_Slave::nb_transport_fw(TRANS& trans, PHASE& phase, sc_time& t) {
	if (phase == BEGIN_REQ) {
		phase = END_REQ;
		t = DO_END_REQ_FUNC_TIME;
		m_begin_resp_peq.notify(trans, DO_BEGIN_RESP_FUNC_TIME);
		DBGLog << "Slave fw返回 trans, END_REQ, t. notify m_begin_resp_peq";
		return TLM_UPDATED;
	} else {
		LOG(ERROR) << "2 phase: fw phase must be BEGIN_REQ";
		assert(false);
	}
}

void AT_Slave::begin_resp_thread() {
	TRANS* trans;
	while (true) {
		wait(m_begin_resp_peq.get_event());
		while (trans = m_begin_resp_peq.get_next_transaction()) {
			trans->set_address(trans->get_address() * 2);
			PHASE phase = BEGIN_RESP;
			sc_time t = SC_ZERO_TIME;
			DBGLog << "Slave发送BEGIN_RESP.";
			tlm_sync_enum ret = target_socket->nb_transport_bw(*trans, phase, t);
			if (ret == TLM_COMPLETED) {
				DBGLog << "TLM_COMPLETED, -, t";
				trans->release();
				wait(t);
			} else {
				LOG(ERROR) << "2 phase: ret must be TLM_COMPLETED, phase must be END_RESP";
				assert(false);
			}
		}
	}
}

//========================================================sc_main===============================================================
int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	AT_Master m{"m"};
	AT_Slave s{"s"};
	m.initiator_socket.bind(s.target_socket);
	sc_start();
	LOG(INFO) << "Sim Completed.";
	return 0;
}