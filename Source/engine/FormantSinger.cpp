#include "FormantSinger.h"

#include <algorithm>
#include <cmath>

namespace fd
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr int kControlRate = 16; // samples per control update
constexpr float kOpenQuotient = 0.6f;

inline double polyBlep (double t, double dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

inline float lerp (float a, float b, float k) { return a + (b - a) * k; }
} // namespace

//==============================================================================
void FormantSinger::Resonator::set (double f, double bw, double sr)
{
    const double T = 1.0 / sr;
    c = -std::exp (-2.0 * kPi * bw * T);
    b = 2.0 * std::exp (-kPi * bw * T) * std::cos (2.0 * kPi * f * T);
    a = 1.0 - b - c;
}

void FormantSinger::AntiResonator::set (double f, double bw, double sr)
{
    const double T = 1.0 / sr;
    const double C = -std::exp (-2.0 * kPi * bw * T);
    const double B = 2.0 * std::exp (-kPi * bw * T) * std::cos (2.0 * kPi * f * T);
    const double A = 1.0 - B - C;
    a = 1.0 / A;
    b = -B / A;
    c = -C / A;
}

//==============================================================================
FormantSinger::FormantSinger()
{
    cur = frameFor (silenceIndex());
    seg.tgt = cur;
    seg.dur = -1;
}

void FormantSinger::prepare (double sr)
{
    sampleRate = sr > 0 ? sr : 48000.0;
    reset();
}

void FormantSinger::reset()
{
    for (auto& r : casc) r.clear();
    nasalPole.clear();
    nasalZero.clear();
    fr1.clear();
    fr2.clear();
    eventCount = 0;
    qHead = qCount = 0;
    heldCount = 0;
    soundingNote = -1;
    cur = frameFor (silenceIndex());
    seg = Segment {};
    seg.tgt = cur;
    seg.dur = -1;
    segElapsed = 0;
    tiltState = 0;
    controlCounter = 0;
    updateControl();
}

void FormantSinger::setLyrics (std::shared_ptr<const Lyrics> l)
{
    lyrics = std::move (l);
    eventCount = 0;
    restart();
}

void FormantSinger::restart()
{
    nextSyl = 0;
    restCounter = 0;
    waitUntilBar = lastNoteBar = noBar;
    vizWaiting.store (false);
    currentSyl = -1;
    lastScheduledSyl = -1;
    vizSyllable.store (-1);
}

//==============================================================================
// Frames and segments
FormantSinger::Frame FormantSinger::frameFor (int ph) const
{
    const PhonemeDef& d = phoneme (ph);
    Frame f;
    f.F[0] = d.f1; f.F[1] = d.f2; f.F[2] = d.f3; f.F[3] = 3300; f.F[4] = 3750;
    f.B[0] = d.b1; f.B[1] = d.b2; f.B[2] = d.b3; f.B[3] = 250;  f.B[4] = 200;
    f.av = d.av;
    f.af = d.af;
    f.ah = d.ah;
    const FricShape& fs = fricShape (d.fric);
    f.fr1f = fs.f1; f.fr1b = fs.b1; f.fr1a = fs.a1;
    f.fr2f = fs.f2; f.fr2b = fs.b2; f.fr2a = fs.a2;
    f.byp = fs.bypass;
    f.fnz = d.fnz > 0 ? d.fnz : 270.0f; // 270 = cancels the nasal pole
    return f;
}

void FormantSinger::pushSeg (const Segment& s)
{
    if (qCount >= (int) queue.size())
        return;
    queue[(size_t) ((qHead + qCount) % (int) queue.size())] = s;
    ++qCount;
}

void FormantSinger::pushSilence()
{
    Segment s;
    // keep the formants of whatever came last so the decay doesn't sweep
    s.tgt = qCount > 0 ? queue[(size_t) ((qHead + qCount - 1) % (int) queue.size())].tgt : seg.tgt;
    s.tgt.av = s.tgt.ah = s.tgt.af = 0;
    s.dur = -1;
    s.fTauMs = 60;
    s.aTauMs = 20;
    s.ph = silenceIndex();
    pushSeg (s);
}

int64_t FormantSinger::consonantDurEstimate (int ph, bool onset) const
{
    const PhonemeDef& d = phoneme (ph);
    const float sp = params.speed;
    switch (d.kind)
    {
        case Kind::StopVoiceless: return ms ((d.durMs + 12 + (onset ? 50 : 25)) * sp);
        case Kind::StopVoiced: return ms ((d.durMs + 8) * sp);
        case Kind::AffricateVoiceless:
        case Kind::AffricateVoiced: return ms ((d.durMs + 60) * sp);
        case Kind::FricVoiceless:
        case Kind::FricVoiced: return ms (d.durMs * sp * (onset ? 1.0f : 0.9f));
        case Kind::Nasal: return ms (d.durMs * sp * (onset ? 1.0f : 1.3f));
        default: return ms (d.durMs * sp);
    }
}

void FormantSinger::pushConsonant (int ph, int nucleusPh, bool onset, bool lastBeforeVowel)
{
    const PhonemeDef& d = phoneme (ph);
    const float sp = params.speed;
    Frame base = frameFor (ph);
    Frame vowel = frameFor (nucleusPh >= 0 ? nucleusPh : findPhoneme ("ax"));

    auto mk = [&] (const Frame& f, float durMs, float fTau, float aTau) {
        Segment s;
        s.tgt = f;
        s.dur = ms (durMs);
        s.fTauMs = fTau;
        s.aTauMs = aTau;
        s.ph = ph;
        return s;
    };

    switch (d.kind)
    {
        case Kind::StopVoiceless:
        case Kind::StopVoiced:
        {
            const bool voiced = d.kind == Kind::StopVoiced;
            Frame closure = base;
            closure.af = closure.ah = 0;
            closure.av = voiced ? 0.07f : 0.0f; // voice bar
            if (voiced) closure.F[0] = 180;
            pushSeg (mk (closure, d.durMs * sp, 12, 5));

            Frame burst = base;
            burst.av = voiced ? 0.25f : 0.0f;
            burst.ah = 0;
            burst.af = onset ? d.af : d.af * 0.4f;
            pushSeg (mk (burst, voiced ? 8.0f : 12.0f, 4, 1.5f));

            if (! voiced)
            {
                Frame asp = vowel; // aspiration is shaped by the following vowel
                asp.av = 0;
                asp.af = 0;
                asp.ah = onset ? d.ah : d.ah * 0.3f;
                const float aspMs = onset ? (lastBeforeVowel ? 45.0f : 12.0f) : 15.0f;
                pushSeg (mk (asp, aspMs * sp, 25, 4));
            }
            break;
        }
        case Kind::AffricateVoiceless:
        case Kind::AffricateVoiced:
        {
            const bool voiced = d.kind == Kind::AffricateVoiced;
            Frame closure = base;
            closure.af = closure.ah = 0;
            closure.av = voiced ? 0.07f : 0.0f;
            pushSeg (mk (closure, d.durMs * sp, 12, 4));
            Frame fric = base;
            pushSeg (mk (fric, 60.0f * sp, 10, 3));
            (void) voiced;
            break;
        }
        case Kind::FricVoiceless:
        case Kind::FricVoiced:
            pushSeg (mk (base, d.durMs * sp * (onset ? 1.0f : 0.9f), 15, 10));
            break;
        case Kind::Aspirate:
        {
            Frame h = vowel;
            h.av = 0;
            h.af = 0;
            h.ah = d.ah;
            pushSeg (mk (h, d.durMs * sp, 4, 10));
            break;
        }
        case Kind::Nasal:
            pushSeg (mk (base, d.durMs * sp * (onset ? 1.0f : 1.3f), 15, 10));
            break;
        case Kind::Liquid:
        case Kind::Glide:
        default:
            pushSeg (mk (base, d.durMs * sp, 25, 15));
            break;
    }
}

int64_t FormantSinger::estimateOnset (int syl) const
{
    if (! lyrics || syl < 0 || syl >= (int) lyrics->syllables.size()) return 0;
    int64_t t = 0;
    for (int ph : lyrics->syllables[(size_t) syl].onset)
        t += consonantDurEstimate (ph, true);
    return t;
}

int64_t FormantSinger::estimateCoda (int syl) const
{
    if (! lyrics || syl < 0 || syl >= (int) lyrics->syllables.size()) return 0;
    const auto& s = lyrics->syllables[(size_t) syl];
    int64_t t = 0;
    if (phoneme (s.nucleus).offglide != nullptr) t += ms (90);
    for (int ph : s.coda)
        t += consonantDurEstimate (ph, false);
    return t;
}

//==============================================================================
// Sequencing
int FormantSinger::pickNextSyllable (bool tempoMode, int64_t bar)
{
    if (! lyrics || lyrics->syllables.empty())
        return -1;
    const int n = (int) lyrics->syllables.size();

    // after a line break, stay silent until the next bar starts
    if (waitUntilBar != noBar)
    {
        if (bar == noBar || bar >= waitUntilBar)
        {
            waitUntilBar = noBar;
            vizWaiting.store (false);
        }
        else
        {
            vizWaiting.store (true);
            return -3;
        }
    }

    if (tempoMode && restCounter > 0)
    {
        --restCounter;
        vizRestCount.fetch_add (1); // each silent step is a rest
        return -1;
    }

    for (int guard = 0; guard < n + 1; ++guard)
    {
        if (nextSyl >= n)
        {
            if (! params.loop) return -1;
            nextSyl = 0;
        }
        const int idx = nextSyl++;
        const auto& s = lyrics->syllables[(size_t) idx];
        if (s.rest)
        {
            if (s.noteRest)
            {
                // punctuation: this note/step is a silent rest. If a newline follows
                // it, the next line waits for the bar after this rest.
                if (s.lineBreak && bar != noBar)
                    waitUntilBar = bar + 1;
                lastNoteBar = bar;
                vizRestCount.fetch_add (1);
                if (tempoMode)
                {
                    restCounter = std::max (0, s.restSteps - 1);
                    return -1;
                }
                vizSyllable.store (idx); // lets the UI count the rest as a step
                return -2;
            }
            if (s.lineBreak && bar != noBar && lastNoteBar != noBar)
            {
                // newline: the next line starts in the bar after the last sung syllable
                waitUntilBar = lastNoteBar + 1;
                if (bar < waitUntilBar)
                {
                    vizWaiting.store (true);
                    return -3;
                }
                waitUntilBar = noBar;
            }
            continue; // a line break never uses up a note itself
        }
        lastNoteBar = bar;
        return idx;
    }
    return -1;
}

void FormantSinger::schedule (const Event& e)
{
    if (eventCount >= (int) events.size())
        return;
    // insertion sort by time (stable: equal times keep insertion order)
    int i = eventCount++;
    while (i > 0 && events[(size_t) i - 1].time > e.time)
    {
        events[(size_t) i] = events[(size_t) i - 1];
        --i;
    }
    events[(size_t) i] = e;
}

void FormantSinger::heldPush (int note)
{
    heldRemove (note);
    if (heldCount < (int) held.size())
        held[(size_t) heldCount++] = note;
}

void FormantSinger::heldRemove (int note)
{
    for (int i = 0; i < heldCount; ++i)
        if (held[(size_t) i] == note)
        {
            for (int j = i; j < heldCount - 1; ++j) held[(size_t) j] = held[(size_t) j + 1];
            --heldCount;
            return;
        }
}

void FormantSinger::noteOn (int note, float vel, int offset, int64_t bar)
{
    const int64_t now = clock + offset;
    const int64_t T = now + leadSamples; // when the vowel should start
    const bool legato = heldCount > 0;
    heldPush (note);

    const int syl = pickNextSyllable (false, bar);
    if (syl == -2 || syl == -3) // punctuation rest, or waiting for the next bar
    {
        // a rest note: end whatever is still sounding and stay silent
        if (legato || currentSyl >= 0 || lastScheduledSyl >= 0)
        {
            Event r;
            r.type = EvType::Release;
            r.time = T;
            r.syllable = lastScheduledSyl;
            schedule (r);
        }
        lastScheduledSyl = -1;
        soundingNote = -1;
        return;
    }
    if (syl < 0)
        return;

    const int64_t onset = estimateOnset (syl);
    const int64_t startAt = std::max (now, T - onset);
    if (legato)
    {
        const int64_t coda = lastScheduledSyl >= 0 ? estimateCoda (lastScheduledSyl) : 0;
        Event r;
        r.type = EvType::Release;
        r.time = std::max (now, startAt - coda);
        r.syllable = lastScheduledSyl;
        schedule (r);
    }
    Event e;
    e.type = EvType::Start;
    e.time = startAt;
    e.syllable = syl;
    e.note = note;
    e.vel = vel;
    schedule (e);
    lastScheduledSyl = syl;
    soundingNote = note;
}

void FormantSinger::noteOff (int note, int offset)
{
    const int64_t now = clock + offset;
    const int64_t T = now + leadSamples;
    const bool wasTop = heldCount > 0 && held[(size_t) heldCount - 1] == note;
    heldRemove (note);

    if (! wasTop)
        return;
    if (lastScheduledSyl < 0 && heldCount == 0)
        return; // a rest note (or nothing) was sounding: nothing to release

    if (heldCount > 0)
    {
        // last-note priority: glide back to the note still held, same syllable
        Event p;
        p.type = EvType::PitchTo;
        p.time = T;
        p.note = held[(size_t) heldCount - 1];
        schedule (p);
        soundingNote = p.note;
        return;
    }

    // end the vowel early enough that the coda finishes near the note-off
    const int64_t coda = lastScheduledSyl >= 0 ? estimateCoda (lastScheduledSyl) : 0;
    Event r;
    r.type = EvType::Release;
    r.time = std::max (now, T - std::min<int64_t> (coda, leadSamples));
    r.syllable = lastScheduledSyl;
    schedule (r);
    soundingNote = -1;
}

void FormantSinger::triggerStep (int note, float vel, int offset, int gateSamples, int64_t bar)
{
    const int64_t now = clock + offset;
    const int64_t T = now + leadSamples;
    const int syl = pickNextSyllable (true, bar);
    if (syl < 0)
        return;
    const int64_t onset = estimateOnset (syl);
    const int64_t coda = estimateCoda (syl);

    Event e;
    e.type = EvType::Start;
    e.time = std::max (now, T - onset);
    e.syllable = syl;
    e.note = note;
    e.vel = vel;
    schedule (e);

    Event r;
    r.type = EvType::Release;
    r.time = std::max (e.time + onset + ms (40), T + gateSamples - coda);
    r.syllable = syl;
    schedule (r);
    lastScheduledSyl = syl;
}

void FormantSinger::releaseAll (int offset)
{
    heldCount = 0;
    soundingNote = -1;
    Event r;
    r.type = EvType::Release;
    r.time = clock + offset;
    schedule (r);
}

void FormantSinger::fireEvent (const Event& e)
{
    switch (e.type)
    {
        case EvType::Start: startSyllable (e.syllable, e.note, e.vel); break;
        case EvType::Release:
            // a release only applies to the syllable it was scheduled for
            // (with lead time the next onset can fire before this release)
            if (e.syllable < 0 || e.syllable == currentSyl)
                releaseCurrent();
            break;
        case EvType::PitchTo: f0Target = 440.0 * std::pow (2.0, (e.note - 69) / 12.0); break;
    }
}

void FormantSinger::startSyllable (int sylIdx, int note, float vel)
{
    if (! lyrics || sylIdx < 0 || sylIdx >= (int) lyrics->syllables.size())
        return;
    const Syllable& s = lyrics->syllables[(size_t) sylIdx];

    // a syllable still being held is released first (its coda goes before our onset)
    if (currentSyl >= 0)
        releaseCurrent();

    // If old material is still queued (a coda), squeeze it so we stay on time.
    int64_t backlog = 0;
    for (int i = 0; i < qCount; ++i)
    {
        auto& q = queue[(size_t) ((qHead + i) % (int) queue.size())];
        if (q.dur > 0) backlog += q.dur;
    }
    if (backlog > ms (80))
        for (int i = 0; i < qCount; ++i)
        {
            auto& q = queue[(size_t) ((qHead + i) % (int) queue.size())];
            if (q.dur > 0) q.dur = std::max<int64_t> (ms (5), q.dur / 2);
        }
    // drop a trailing silence-hold so the new syllable follows immediately
    if (qCount > 0)
    {
        auto& last = queue[(size_t) ((qHead + qCount - 1) % (int) queue.size())];
        if (last.dur < 0) --qCount;
    }

    // A word ending in a fricative followed by one starting with a fricative
    // ("is the", "this song") would otherwise merge into one long hiss.
    auto isFric = [] (int ph) {
        const Kind k = phoneme (ph).kind;
        return k == Kind::FricVoiceless || k == Kind::FricVoiced || k == Kind::AffricateVoiceless
               || k == Kind::AffricateVoiced || k == Kind::Aspirate;
    };
    Segment* prevTail = nullptr;
    if (qCount > 0)
        prevTail = &queue[(size_t) ((qHead + qCount - 1) % (int) queue.size())];
    else if (seg.dur > 0)
        prevTail = &seg;
    const bool merge = prevTail != nullptr && ! s.onset.empty() && isFric (s.onset.front()) && isFric (prevTail->ph);
    if (merge)
        prevTail->dur = std::max<int64_t> (ms (25), prevTail->dur / 2);
    const int firstOnsetSeg = qCount;

    for (size_t i = 0; i < s.onset.size(); ++i)
        pushConsonant (s.onset[i], s.nucleus, true, i + 1 == s.onset.size());

    if (merge && qCount > firstOnsetSeg)
    {
        auto& first = queue[(size_t) ((qHead + firstOnsetSeg) % (int) queue.size())];
        first.dur = std::max<int64_t> (ms (25), first.dur / 2);
    }

    Segment v;
    v.tgt = frameFor (s.nucleus);
    v.dur = -1;
    v.fTauMs = s.onset.empty() ? 15.0f : 35.0f;
    v.aTauMs = s.onset.empty() ? 12.0f : 18.0f;
    v.ph = s.nucleus;
    v.nucleus = true;
    pushSeg (v);

    // pitch
    const double target = 440.0 * std::pow (2.0, (note - 69) / 12.0);
    const bool sounding = cur.av > 0.05f || qCount > 1;
    f0Target = target;
    if (! sounding || params.glideMs <= 0.5f)
        f0 = target;
    velTarget = 0.35f + 0.65f * std::clamp (vel, 0.0f, 1.0f);

    currentSyl = sylIdx;
    currentNucleus = s.nucleus;
    vizSyllable.store (sylIdx);
}

void FormantSinger::releaseCurrent()
{
    // find the held nucleus: current segment or one still queued
    bool currentIsHold = seg.dur < 0 && seg.nucleus;
    if (! currentIsHold)
    {
        bool found = false;
        for (int i = qCount - 1; i >= 0; --i)
        {
            auto& q = queue[(size_t) ((qHead + i) % (int) queue.size())];
            if (q.dur < 0 && q.nucleus)
            {
                q.dur = ms (70); // a very short note still gets a vowel
                found = true;
                break;
            }
        }
        if (! found && ! (seg.dur < 0 && seg.ph != silenceIndex()))
        {
            if (seg.dur < 0 && seg.ph == silenceIndex() && qCount == 0)
                return; // already silent
        }
    }

    if (currentSyl >= 0 && lyrics && currentSyl < (int) lyrics->syllables.size())
    {
        const Syllable& s = lyrics->syllables[(size_t) currentSyl];
        if (const char* off = phoneme (s.nucleus).offglide)
        {
            Segment g;
            g.tgt = frameFor (findPhoneme (off));
            g.dur = ms (90);
            g.fTauMs = 40;
            g.aTauMs = 20;
            g.ph = findPhoneme (off);
            pushSeg (g);
        }
        for (int ph : s.coda)
            pushConsonant (ph, s.nucleus, false, false);
    }
    pushSilence();
    currentSyl = -1;
}

//==============================================================================
// Rendering
void FormantSinger::advanceSegments (int n)
{
    segElapsed += n;
    for (;;)
    {
        const bool timedOut = seg.dur >= 0 && segElapsed >= seg.dur;
        const bool holdEnded = seg.dur < 0 && qCount > 0;
        if (! (timedOut || holdEnded))
            break;
        if (qCount == 0)
        {
            // ran out of material: hold silence
            Segment s;
            s.tgt = seg.tgt;
            s.tgt.av = s.tgt.ah = s.tgt.af = 0;
            s.dur = -1;
            s.aTauMs = 20;
            s.fTauMs = 60;
            seg = s;
            segElapsed = 0;
            break;
        }
        segElapsed = timedOut ? segElapsed - seg.dur : 0;
        seg = queue[(size_t) qHead];
        qHead = (qHead + 1) % (int) queue.size();
        --qCount;
        if (seg.nucleus)
            nucleusStartClock = clock;
    }
}

void FormantSinger::updateControl()
{
    const double dtMs = 1000.0 * kControlRate / sampleRate;
    const float kF = (float) (1.0 - std::exp (-dtMs / std::max (0.5f, seg.fTauMs)));
    const float kA = (float) (1.0 - std::exp (-dtMs / std::max (0.5f, seg.aTauMs)));
    const Frame& t = seg.tgt;

    for (int i = 0; i < 5; ++i)
    {
        cur.F[i] = lerp (cur.F[i], t.F[i], kF);
        cur.B[i] = lerp (cur.B[i], t.B[i], kF);
    }
    cur.fnz = lerp (cur.fnz, t.fnz, kF);
    cur.fr1f = lerp (cur.fr1f, t.fr1f, kF);
    cur.fr1b = lerp (cur.fr1b, t.fr1b, kF);
    cur.fr2f = lerp (cur.fr2f, t.fr2f, kF);
    cur.fr2b = lerp (cur.fr2b, t.fr2b, kF);
    cur.fr1a = lerp (cur.fr1a, t.fr1a, kA);
    cur.fr2a = lerp (cur.fr2a, t.fr2a, kA);
    cur.byp = lerp (cur.byp, t.byp, kA);
    cur.av = lerp (cur.av, t.av, kA);
    cur.ah = lerp (cur.ah, t.ah, kA);
    cur.af = lerp (cur.af, t.af, kA);
    velGain = lerp (velGain, velTarget, kA);

    // pitch glide
    const double glideMs = std::max (0.5, (double) params.glideMs * 0.4);
    const double kP = 1.0 - std::exp (-dtMs / glideMs);
    f0 += (f0Target - f0) * kP;

    // filters
    const double nyq = sampleRate * 0.45;
    const double shift = params.formantShift;
    for (int i = 0; i < 5; ++i)
    {
        const double f = std::min (nyq, cur.F[i] * shift);
        casc[i].set (f, cur.B[i] * std::sqrt (shift), sampleRate);
    }
    nasalPole.set (270.0 * shift, 100.0, sampleRate);
    nasalZero.set (std::min (nyq, (double) cur.fnz * shift), 100.0, sampleRate);
    fr1.set (std::min (nyq, (double) cur.fr1f * shift), cur.fr1b, sampleRate);
    fr2.set (std::min (nyq, (double) cur.fr2f * shift), cur.fr2b, sampleRate);

    // spectral tilt: glottal source is darker than the buzz
    const double tiltHz = 4500.0 - 2500.0 * params.source;
    tiltCoef = std::exp (-2.0 * kPi * tiltHz / sampleRate);
    fricLpCoef = std::exp (-2.0 * kPi * std::min (7000.0, sampleRate * 0.4) / sampleRate);

    vizF1.store (cur.F[0] * (float) shift);
    vizF2.store (cur.F[1] * (float) shift);
    vizF3.store (cur.F[2] * (float) shift);
    vizVoicing.store (cur.av * velGain);
    vizNoise.store (std::max (cur.af, cur.ah));
    vizPhoneme.store (seg.ph);
}

void FormantSinger::process (float* out, int numSamples, const float* carrier)
{
    int i = 0;
    while (i < numSamples)
    {
        // fire due events
        while (eventCount > 0 && events[0].time <= clock)
        {
            Event e = events[0];
            for (int k = 0; k < eventCount - 1; ++k) events[(size_t) k] = events[(size_t) k + 1];
            --eventCount;
            fireEvent (e);
        }

        if (controlCounter <= 0)
        {
            advanceSegments (kControlRate);
            updateControl();
            controlCounter = kControlRate;
        }

        // render until next control tick or next event
        int n = std::min (controlCounter, numSamples - i);
        if (eventCount > 0)
        {
            const int64_t untilEv = events[0].time - clock;
            if (untilEv > 0 && untilEv < n) n = (int) untilEv;
        }
        if (n <= 0) n = 1;

        // vibrato and jitter (per chunk)
        const double vibFade = std::clamp ((double) (clock - nucleusStartClock) / (0.35 * sampleRate), 0.0, 1.0);
        vibPhase += params.vibRateHz * n / sampleRate;
        if (vibPhase > 1.0) vibPhase -= 1.0;
        rng = rng * 1664525u + 1013904223u;
        jitter = 0.995f * jitter + 0.005f * ((float) (rng >> 9) / 8388608.0f - 1.0f);
        const double cents = params.vibDepthCents * vibFade * std::sin (2.0 * kPi * vibPhase) + jitter * 8.0;
        const double freq = f0 * std::pow (2.0, (cents / 100.0 + bendSemis) / 12.0);
        const double dt = std::min (0.45, freq / sampleRate);

        const float src = params.source;
        const float breath = params.breath;

        for (int k = 0; k < n; ++k)
        {
            phase += dt;
            if (phase >= 1.0) phase -= 1.0;

            // buzz: band-limited falling saw (the classic vocoder "buzz")
            double saw = 1.0 - 2.0 * phase;
            saw += polyBlep (phase, dt);

            // glottal: derivative of KLGLOTT88 flow, with a band-limited closure step
            double glot = 0.0;
            if (phase < kOpenQuotient)
            {
                const double x = phase / kOpenQuotient;
                glot = 2.0 * x - 3.0 * x * x;
            }
            double tc = phase - kOpenQuotient;
            if (tc < 0) tc += 1.0;
            glot += 0.5 * polyBlep (tc, dt);
            glot *= 1.6;

            double voice = (1.0 - src) * saw * 0.7 + src * glot;

            // external carrier (vocoder-style): auto-levelled, crossfaded with the buzz
            carrierMix += (carrierMixTarget - carrierMix) * 0.002f; // ~10 ms fade
            if (carrierMix > 1.0e-4f)
            {
                const double c = carrier != nullptr ? (double) carrier[i + k] : 0.0;
                const double ac = std::abs (c);
                carrierEnv = ac > carrierEnv ? carrierEnv + (ac - carrierEnv) * 0.01 : carrierEnv * 0.99995;
                const double agc = std::min (30.0, 0.7 / std::max (carrierEnv, 0.02));
                voice = voice * (1.0 - carrierMix) + c * agc * carrierMix;
            }
            voice *= cur.av * velGain;

            // noise
            rng = rng * 1664525u + 1013904223u;
            const double n1 = (double) (rng >> 8) / 8388608.0 - 1.0;
            rng = rng * 1664525u + 1013904223u;
            const double n2 = (double) (rng >> 8) / 8388608.0 - 1.0;

            const double openGate = phase < kOpenQuotient ? 1.0 : 0.35;
            const double asp = n1 * (cur.ah + breath * 0.25 * cur.av * velGain * openGate);

            // tilt the source
            double x = voice + asp * 0.5;
            tiltState = (1.0 - tiltCoef) * x + tiltCoef * tiltState;
            x = tiltState * 1.0 + (x - tiltState) * 0.35;

            // cascade: nasal pole/zero then F5..F1
            x = nasalPole.process (x);
            x = nasalZero.process (x);
            for (int r = 4; r >= 0; --r)
                x = casc[r].process (x);

            // parallel frication
            const double fn = n2 * cur.af;
            double fric = fr1.process (fn) * cur.fr1a + fr2.process (fn) * cur.fr2a + fn * cur.byp;
            fricLp = fric + fricLpCoef * (fricLp - fric); // gentle one-pole high cut
            fric = fricLp;

            double y = x * 0.45 + fric * 0.16;

            // lofi: sample-and-hold + bit crush
            if (params.lofi > 0.001f)
            {
                const double targetRate = sampleRate - (sampleRate - 6000.0) * params.lofi;
                lofiPhase += targetRate / sampleRate;
                if (lofiPhase >= 1.0)
                {
                    lofiPhase -= 1.0;
                    const double levels = std::pow (2.0, 16.0 - 9.0 * params.lofi);
                    lofiHold = std::round (y * levels) / levels;
                }
                y = lofiHold;
            }

            y *= params.gain;
            y = std::tanh (y);
            out[i + k] = (float) y;
            levelAcc = std::max (levelAcc * 0.9995f, (float) std::abs (y));
        }

        i += n;
        clock += n;
        controlCounter -= n;
    }
    vizLevel.store (levelAcc);
}
} // namespace fd
