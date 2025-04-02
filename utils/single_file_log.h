#include "log.h"

int LogMessage::fileLen = 0;
int LogMessage::funcLen = 0;
int LogMessage::lineLen = 0;

void init_single_file_log(std::string name) {
	LogGlobal* logGlobalInst = LogGlobal::GetInstance();
	bool ret = logGlobalInst->init(name, LogCategory::DEBUG, LogOutputCategory::StdoutAndFile, true, false);
    assert(ret);
}
