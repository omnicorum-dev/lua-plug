#pragma once

#include <array>
#include <cmath>
#include <cstdint>

/// @file
/// MIDI constants and note/velocity/pitch-bend conversion helpers,
/// including a compile-time MIDI-note-to-frequency table.

namespace omni {

constexpr double A4 = 440.0; ///< Reference pitch for A4, in Hz.
constexpr double SEMITONE =
    1.0594630943592952646; ///< Frequency ratio of one semitone (2^(1/12)).

/// @name MIDI value ranges
/// @{
constexpr int MIDI_MIN_NOTE   = 0;
constexpr int MIDI_MAX_NOTE   = 127;
constexpr int MIDI_NOTE_COUNT = 128;

constexpr int MIDI_MIN_VELOCITY = 0;
constexpr int MIDI_MAX_VELOCITY = 127;

constexpr int MIDI_MIN_VALUE = 0;
constexpr int MIDI_MAX_VALUE = 127;

constexpr int MIDI_MIN_CHANNEL = 1;
constexpr int MIDI_MAX_CHANNEL = 16;

constexpr int MIDI_MIN_PITCHBEND    = 0;
constexpr int MIDI_MAX_PITCHBEND    = 16383;
constexpr int MIDI_CENTER_PITCHBEND = 8192;
/// @}

/// Status byte (high nibble) for common channel voice messages.
enum class MessageType : uint8_t {
    NoteOff           = 0x80,
    NoteOn            = 0x90,
    PolyAftertouch    = 0xA0,
    ControlChange     = 0xB0,
    ProgramChange     = 0xC0,
    ChannelAftertouch = 0xD0,
    PitchBend         = 0xE0,
};

/// Pitch class (note name mod 12). Enharmonic pairs (Cs/Db etc.) share
/// the same numeric value -- both spellings are valid for the same class.
enum class PitchClass {
    C  = 0,
    Cs = 1,
    Db = 1,
    D  = 2,
    Ds = 3,
    Eb = 3,
    E  = 4,
    F  = 5,
    Fs = 6,
    Gb = 6,
    G  = 7,
    Gs = 8,
    Ab = 8,
    A  = 9,
    As = 10,
    Bb = 10,
    B  = 11
};

/// Exact (non-table) note-to-frequency conversion via pow(2,x) -- prefer
/// midiNoteToFreq() for a precomputed, cheaper lookup.
constexpr double midiNoteToFreqExact(int note) {
    return A4 * std::pow(2, (note - 69) / 12.0);
}

/// Compile-time table of all 128 MIDI note frequencies, built outward
/// from A4 (note 69) by repeated multiplication/division by SEMITONE,
/// rather than calling pow() per note.
constexpr auto midiFrequencies = [] {
    std::array<double, MIDI_NOTE_COUNT> frequencies{};

    frequencies[69] = A4;

    for (size_t i = 70; i < MIDI_NOTE_COUNT; ++i)
        frequencies[i] = frequencies[i - 1] * SEMITONE;

    for (int i = 68; i >= 0; --i)
        frequencies[(size_t)i] = frequencies[(size_t)(i + 1)] / SEMITONE;

    return frequencies;
}();

/// Table lookup. @pre note in [0, 127].
constexpr double midiNoteToFreq(int note) {
    return midiFrequencies[(size_t)note];
}

/// MIDI note -> octave, using note 60 (middle C) = C4.
constexpr int midiNoteToOctave(int note) { return note / 12 - 1; }

constexpr PitchClass midiNoteToPitchClass(int note) {
    return (PitchClass)(note % 12);
}

/// @name Normalization helpers
/// Convert between raw MIDI ranges (0-127, or the 14-bit pitch-bend
/// range) and normalized [0,1] (or [-1,1] for pitch bend) doubles.
/// @{
constexpr double normalizeVelocity(int velocity) { return velocity / 127.0; }

constexpr int denormalizeVelocity(double velocity) {
    return (int)(velocity * 127.0);
}

constexpr double normalizeMidiNote(int note) { return note / 127.0; }

constexpr int denormalizeMidiNote(double value) { return (int)(value * 127.0); }

constexpr double normalizePitchBend(int pitchbend) {
    return (pitchbend - (double)MIDI_CENTER_PITCHBEND) /
           (double)MIDI_CENTER_PITCHBEND;
}
/// @}

} // namespace omni
