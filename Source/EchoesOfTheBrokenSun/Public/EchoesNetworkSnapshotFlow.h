#pragma once

// Author and owner: Angelis Pseftis
#include <cstddef>
#include <optional>

namespace echoes::network {

enum class SnapshotSendDecision { Send, WaitForAcknowledgement, TimedOut };

// Connection-local transport state. The controller validates the digest and
// monotonically increasing snapshot ID before recording acknowledgement progress.
class SnapshotFlowControl final {
public:
    static constexpr std::size_t MaximumPending = 8;
    static constexpr double NoProgressTimeoutSeconds = 45.0;

    [[nodiscard]] SnapshotSendDecision Evaluate(
        std::size_t pending, double now) const {
        if (pending != 0 && progressSeconds_ &&
            now - *progressSeconds_ >= NoProgressTimeoutSeconds) {
            return SnapshotSendDecision::TimedOut;
        }
        return pending >= MaximumPending
            ? SnapshotSendDecision::WaitForAcknowledgement
            : SnapshotSendDecision::Send;
    }

    void RecordSend(std::size_t previouslyPending, double now) {
        // Additional sends and recovery retransmissions cannot extend the lease.
        if (previouslyPending == 0) progressSeconds_ = now;
    }

    void RecordValidAcknowledgement(std::size_t remaining, double now) {
        progressSeconds_ = remaining != 0 ? std::optional<double>(now)
                                         : std::nullopt;
    }

    void Reset() { progressSeconds_.reset(); }

private:
    std::optional<double> progressSeconds_;
};

} // namespace echoes::network
