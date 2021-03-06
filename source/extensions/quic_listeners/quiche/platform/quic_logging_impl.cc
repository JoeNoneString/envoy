// NOLINT(namespace-envoy)

// This file is part of the QUICHE platform implementation, and is not to be
// consumed or referenced directly by other Envoy code. It serves purely as a
// porting layer for QUICHE.

#include "extensions/quic_listeners/quiche/platform/quic_logging_impl.h"

#include <atomic>

namespace quic {

namespace {
std::atomic<int> g_verbosity_threshold;

// Pointer to the global log sink, usually it is nullptr.
// If not nullptr, as in some tests, the sink will receive a copy of the log message right after the
// message is emitted from the QUIC_LOG... macros.
std::atomic<QuicLogSink*> g_quic_log_sink;
absl::Mutex g_quic_log_sink_mutex;
} // namespace

QuicLogEmitter::QuicLogEmitter(QuicLogLevel level) : level_(level), saved_errno_(errno) {}

QuicLogEmitter::~QuicLogEmitter() {
  if (is_perror_) {
    // TODO(wub): Change to a thread-safe version of strerror.
    stream_ << ": " << strerror(saved_errno_) << " [" << saved_errno_ << "]";
  }
  GetLogger().log(level_, "{}", stream_.str().c_str());

  // Normally there is no log sink and we can avoid acquiring the lock.
  if (g_quic_log_sink.load(std::memory_order_relaxed) != nullptr) {
    absl::MutexLock lock(&g_quic_log_sink_mutex);
    QuicLogSink* sink = g_quic_log_sink.load(std::memory_order_relaxed);
    if (sink != nullptr) {
      sink->Log(level_, stream_.str());
    }
  }

  if (level_ == FATAL) {
    abort();
  }
}

int GetVerbosityLogThreshold() { return g_verbosity_threshold.load(std::memory_order_relaxed); }

void SetVerbosityLogThreshold(int new_verbosity) {
  g_verbosity_threshold.store(new_verbosity, std::memory_order_relaxed);
}

QuicLogSink* SetLogSink(QuicLogSink* new_sink) {
  absl::MutexLock lock(&g_quic_log_sink_mutex);
  QuicLogSink* old_sink = g_quic_log_sink.load(std::memory_order_relaxed);
  g_quic_log_sink.store(new_sink, std::memory_order_relaxed);
  return old_sink;
}

} // namespace quic
