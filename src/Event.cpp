#include "Event.hpp"

#include <map>

PytalInitHandler::PytalInitHandler(std::function<void()> cb)
	: cb(cb) {
		PytalInitHandler::GetHandlers().push_back(this);
}

void PytalInitHandler::RunAll() {
	for (auto h : PytalInitHandler::GetHandlers()) {
		h->cb();
	}
}

std::vector<PytalInitHandler *> &PytalInitHandler::GetHandlers() {
	static std::vector<PytalInitHandler *> handlers;
	return handlers;
}
