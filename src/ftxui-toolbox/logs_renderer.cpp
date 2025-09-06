#include "logs_renderer.hpp"

#include "locked_dequeue.hpp"
#include "scroller.hpp"

class LogsRendererBase : public ftxui::ComponentBase {
 public:
  LogsRendererBase(const LockedDequeue& logs) : Logs_(logs) {
    auto log_renderer = ftxui::Renderer([&] {
      ftxui::Elements elements;
      Logs_.ForEach([&](const std::string& s) { elements.push_back(ftxui::text(s)); });
      auto content = vbox(std::move(elements));
      return content | ftxui::flex_grow;
    });

    auto log_scroller = ftxui::Scroller(log_renderer);

    Add(ftxui::Container::Horizontal({
        ftxui::Renderer([] { return ftxui::separator(); }),
        log_scroller,
    }));
  }

 private:
  const LockedDequeue& Logs_;
};

ftxui::Component LogsRenderer(const LockedDequeue& logs) {
  return ftxui::Make<LogsRendererBase>(logs);
}
