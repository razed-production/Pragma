#pragma once

#include "Pragma/Core/SceneSerializer.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Pragma::Core
{
class SceneHistory
{
public:
    struct Entry
    {
        std::string Label;
        SerializedScene Snapshot;
    };

    void Reset() noexcept
    {
        m_undoStack.clear();
        m_redoStack.clear();
    }

    template <typename TEqual>
    void Capture(std::string label, const SerializedScene& snapshot, TEqual&& equals)
    {
        if (!m_undoStack.empty() && equals(m_undoStack.back().Snapshot, snapshot))
        {
            return;
        }

        m_undoStack.push_back({ std::move(label), snapshot });
        Trim(m_undoStack);
        m_redoStack.clear();
    }

    [[nodiscard]] bool CanUndo() const noexcept
    {
        return !m_undoStack.empty();
    }

    [[nodiscard]] bool CanRedo() const noexcept
    {
        return !m_redoStack.empty();
    }

    [[nodiscard]] const std::string& PeekUndoLabel() const noexcept
    {
        static const std::string kEmpty;
        return m_undoStack.empty() ? kEmpty : m_undoStack.back().Label;
    }

    [[nodiscard]] const std::string& PeekRedoLabel() const noexcept
    {
        static const std::string kEmpty;
        return m_redoStack.empty() ? kEmpty : m_redoStack.back().Label;
    }

    [[nodiscard]] const std::vector<Entry>& GetUndoStack() const noexcept
    {
        return m_undoStack;
    }

    [[nodiscard]] const std::vector<Entry>& GetRedoStack() const noexcept
    {
        return m_redoStack;
    }

    [[nodiscard]] Entry PopUndo(const SerializedScene& currentSnapshot)
    {
        Entry entry = m_undoStack.back();
        m_undoStack.pop_back();
        m_redoStack.push_back({ entry.Label, currentSnapshot });
        Trim(m_redoStack);
        return entry;
    }

    [[nodiscard]] Entry PopRedo(const SerializedScene& currentSnapshot)
    {
        Entry entry = m_redoStack.back();
        m_redoStack.pop_back();
        m_undoStack.push_back({ entry.Label, currentSnapshot });
        Trim(m_undoStack);
        return entry;
    }

private:
    static void Trim(std::vector<Entry>& stack)
    {
        constexpr std::size_t kHistoryLimit = 128;
        if (stack.size() > kHistoryLimit)
        {
            stack.erase(stack.begin(), stack.begin() + static_cast<std::ptrdiff_t>(stack.size() - kHistoryLimit));
        }
    }

private:
    std::vector<Entry> m_undoStack;
    std::vector<Entry> m_redoStack;
};
}
