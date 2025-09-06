#pragma once

#include <ftxui/component/component.hpp>

#include "locked_dequeue.hpp"

ftxui::Component LogsRenderer(const LockedDequeue& logs);
