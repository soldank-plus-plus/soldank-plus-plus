module;

#include <span>
#include <variant>
#include <vector>

export module MapEditor.EditorEventRouter;

import MapEditorState;

export namespace Soldank
{
struct EditorToolSelectedEvent
{
    ToolType tool_type;
};

struct EditorCommandExecutedEvent
{
};

struct EditorCommandUndoneEvent
{
};

struct EditorCommandRedoneEvent
{
};

struct EditorToolsActivatedEvent
{
};

struct EditorToolsDeactivatedEvent
{
};

using EditorEvent = std::variant<EditorToolSelectedEvent,
                                 EditorCommandExecutedEvent,
                                 EditorCommandUndoneEvent,
                                 EditorCommandRedoneEvent,
                                 EditorToolsActivatedEvent,
                                 EditorToolsDeactivatedEvent>;

class EditorEventSink
{
public:
    virtual ~EditorEventSink() = default;
    virtual void OnEditorEvents(std::span<const EditorEvent> events) = 0;
};

class EditorEventRouter
{
public:
    void AddSink(EditorEventSink& sink) { sinks_.push_back(&sink); }

    void Emit(EditorEvent event)
    {
        const EditorEvent events[] = { event };
        for (EditorEventSink* sink : sinks_) {
            sink->OnEditorEvents(events);
        }
    }

private:
    std::vector<EditorEventSink*> sinks_;
};
} // namespace Soldank
