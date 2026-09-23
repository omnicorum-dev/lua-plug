#pragma once

#include "juce_audio_basics/juce_audio_basics.h"

/// @file
/// Sample-accurate MIDI buffer walker, meant to be interleaved with a
/// processBlock's per-sample loop.

namespace omni {

/// Iterates a juce::MidiBuffer's events in order: advance the cursor
/// past events whose samplePosition matches the current sample index.
class MidiCursor {
  public:
    /// `midi` must outlive this cursor.
    MidiCursor(const juce::MidiBuffer &midi)
        : current(midi.begin()), end(midi.end()) {}

    /// True while events remain.
    bool hasEvent() const { return current != end; }

    /// Current event (sample position + message).
    const juce::MidiMessageMetadata event() const { return *current; }

    /// Moves to the next event.
    void advance() { ++current; }

  private:
    juce::MidiBufferIterator current;
    juce::MidiBufferIterator end;
};

} // namespace omni
