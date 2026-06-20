#pragma once
#include <JuceHeader.h>

//==============================================================================
// DAWIcons — Path-based vector icons for the Orpheus Plus toolbar.
// Every icon is a static method returning a juce::Path scaled to fit `area`.
// No external image dependencies — pure vector art.
//==============================================================================
class DAWIcons
{
public:
    //── File ──────────────────────────────────────────────────────────────
    static juce::Path newProject(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        float fold = r.getWidth() * 0.3f;
        p.startNewSubPath(r.getX(), r.getY());
        p.lineTo(r.getRight() - fold, r.getY());
        p.lineTo(r.getRight(), r.getY() + fold);
        p.lineTo(r.getRight(), r.getBottom());
        p.lineTo(r.getX(), r.getBottom());
        p.closeSubPath();
        // Fold triangle
        p.startNewSubPath(r.getRight() - fold, r.getY());
        p.lineTo(r.getRight() - fold, r.getY() + fold);
        p.lineTo(r.getRight(), r.getY() + fold);
        // Plus sign
        float cx = r.getCentreX(), cy = r.getCentreY() + fold * 0.3f;
        float s = r.getWidth() * 0.15f;
        p.addRectangle(cx - s, cy - 1.0f, s * 2, 2.0f);
        p.addRectangle(cx - 1.0f, cy - s, 2.0f, s * 2);
        return p;
    }

    static juce::Path openFile(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        // Folder body
        p.addRoundedRectangle(r.getX(), r.getY() + r.getHeight() * 0.2f,
                              r.getWidth(), r.getHeight() * 0.75f, 2.0f);
        // Folder tab
        p.addRoundedRectangle(r.getX(), r.getY(),
                              r.getWidth() * 0.4f, r.getHeight() * 0.25f, 2.0f, 2.0f, true, true, false, false);
        return p;
    }

    static juce::Path save(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        p.addRoundedRectangle(r, 2.0f);
        // Disk slot
        float slotW = r.getWidth() * 0.5f;
        p.addRectangle(r.getCentreX() - slotW * 0.5f, r.getY(), slotW, r.getHeight() * 0.35f);
        // Label area
        p.addRectangle(r.getX() + 3, r.getBottom() - r.getHeight() * 0.4f,
                       r.getWidth() - 6, r.getHeight() * 0.35f);
        return p;
    }

    static juce::Path exportIcon(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.22f);
        float cx = r.getCentreX();
        // Arrow up
        p.startNewSubPath(cx, r.getY());
        p.lineTo(cx + r.getWidth() * 0.3f, r.getY() + r.getHeight() * 0.35f);
        p.lineTo(cx + 2.0f, r.getY() + r.getHeight() * 0.35f);
        p.lineTo(cx + 2.0f, r.getBottom() - r.getHeight() * 0.2f);
        p.lineTo(cx - 2.0f, r.getBottom() - r.getHeight() * 0.2f);
        p.lineTo(cx - 2.0f, r.getY() + r.getHeight() * 0.35f);
        p.lineTo(cx - r.getWidth() * 0.3f, r.getY() + r.getHeight() * 0.35f);
        p.closeSubPath();
        // Bottom bar
        p.addRectangle(r.getX(), r.getBottom() - 2.5f, r.getWidth(), 2.5f);
        return p;
    }

    //── Edit ──────────────────────────────────────────────────────────────
    static juce::Path undo(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        // Curved arrow pointing left
        p.addArc(r.getX(), r.getCentreY() - r.getHeight() * 0.25f,
                 r.getWidth() * 0.8f, r.getHeight() * 0.5f,
                 juce::MathConstants<float>::pi * 0.2f,
                 juce::MathConstants<float>::pi * 1.2f, true);
        // Arrowhead
        float ax = r.getX() + r.getWidth() * 0.15f;
        float ay = r.getCentreY();
        p.startNewSubPath(ax, ay);
        p.lineTo(ax + r.getWidth() * 0.2f, ay - r.getHeight() * 0.15f);
        p.lineTo(ax + r.getWidth() * 0.2f, ay + r.getHeight() * 0.15f);
        p.closeSubPath();
        return p;
    }

    static juce::Path redo(juce::Rectangle<float> area)
    {
        auto p = undo(area);
        p.applyTransform(juce::AffineTransform::scale(-1.0f, 1.0f,
                         area.getCentreX(), area.getCentreY()));
        return p;
    }

    static juce::Path cut(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        float cx = r.getCentreX();
        float cr = r.getWidth() * 0.16f;
        // Scissors: two circles + crossing lines
        p.addEllipse(cx - cr * 2.5f, r.getBottom() - cr * 2.5f, cr * 2.0f, cr * 2.0f);
        p.addEllipse(cx + cr * 0.5f, r.getBottom() - cr * 2.5f, cr * 2.0f, cr * 2.0f);
        // Blades
        p.startNewSubPath(cx - cr, r.getBottom() - cr * 2);
        p.lineTo(cx + cr * 1.5f, r.getY());
        p.startNewSubPath(cx + cr, r.getBottom() - cr * 2);
        p.lineTo(cx - cr * 1.5f, r.getY());
        return p;
    }

    static juce::Path copy(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.18f);
        float off = r.getWidth() * 0.15f;
        // Back doc
        p.addRoundedRectangle(r.getX() + off, r.getY(), r.getWidth() - off, r.getHeight() - off, 2.0f);
        // Front doc
        p.addRoundedRectangle(r.getX(), r.getY() + off, r.getWidth() - off, r.getHeight() - off, 2.0f);
        return p;
    }

    static juce::Path paste(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.18f);
        // Clipboard
        p.addRoundedRectangle(r.getX(), r.getY() + r.getHeight() * 0.15f,
                              r.getWidth(), r.getHeight() * 0.85f, 2.0f);
        // Clip
        float clipW = r.getWidth() * 0.35f;
        p.addRoundedRectangle(r.getCentreX() - clipW * 0.5f, r.getY(),
                              clipW, r.getHeight() * 0.25f, 2.0f);
        return p;
    }

    //── Tools ─────────────────────────────────────────────────────────────
    static juce::Path selectArrow(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.25f);
        p.startNewSubPath(r.getX(), r.getY());
        p.lineTo(r.getX(), r.getBottom());
        p.lineTo(r.getX() + r.getWidth() * 0.35f, r.getBottom() - r.getHeight() * 0.25f);
        p.lineTo(r.getX() + r.getWidth() * 0.6f, r.getBottom());
        p.lineTo(r.getX() + r.getWidth() * 0.75f, r.getBottom() - r.getHeight() * 0.15f);
        p.lineTo(r.getX() + r.getWidth() * 0.45f, r.getBottom() - r.getHeight() * 0.4f);
        p.lineTo(r.getRight(), r.getY() + r.getHeight() * 0.35f);
        p.closeSubPath();
        return p;
    }

    static juce::Path pencilDraw(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        // Pencil body (rotated rectangle)
        float w = r.getWidth() * 0.25f;
        p.startNewSubPath(r.getRight() - w, r.getY());
        p.lineTo(r.getRight(), r.getY() + w);
        p.lineTo(r.getX() + w, r.getBottom());
        p.lineTo(r.getX(), r.getBottom() - w);
        p.closeSubPath();
        // Tip
        p.startNewSubPath(r.getX() + w, r.getBottom());
        p.lineTo(r.getX(), r.getBottom());
        p.lineTo(r.getX(), r.getBottom() - w);
        p.closeSubPath();
        return p;
    }

    static juce::Path sliceTool(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        // Blade
        p.startNewSubPath(r.getX(), r.getBottom());
        p.lineTo(r.getRight(), r.getY());
        p.lineTo(r.getRight(), r.getY() + r.getHeight() * 0.15f);
        p.lineTo(r.getX() + r.getWidth() * 0.12f, r.getBottom());
        p.closeSubPath();
        return p;
    }

    static juce::Path eraser(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.18f);
        p.addRoundedRectangle(r.getX(), r.getCentreY() - r.getHeight() * 0.2f,
                              r.getWidth(), r.getHeight() * 0.4f, 3.0f);
        // Bottom edge
        p.addRectangle(r.getX() + 2, r.getCentreY() + r.getHeight() * 0.1f,
                       r.getWidth() * 0.35f, r.getHeight() * 0.1f);
        return p;
    }

    static juce::Path muteTool(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        // Speaker body
        float bodyW = r.getWidth() * 0.3f;
        p.addRectangle(r.getX(), r.getCentreY() - r.getHeight() * 0.15f,
                       bodyW, r.getHeight() * 0.3f);
        // Cone
        p.startNewSubPath(r.getX() + bodyW, r.getCentreY() - r.getHeight() * 0.15f);
        p.lineTo(r.getX() + r.getWidth() * 0.55f, r.getY());
        p.lineTo(r.getX() + r.getWidth() * 0.55f, r.getBottom());
        p.lineTo(r.getX() + bodyW, r.getCentreY() + r.getHeight() * 0.15f);
        p.closeSubPath();
        // X (mute)
        float mx = r.getRight() - r.getWidth() * 0.15f;
        float my = r.getCentreY();
        float ms = r.getWidth() * 0.12f;
        p.startNewSubPath(mx - ms, my - ms);
        p.lineTo(mx + ms, my + ms);
        p.startNewSubPath(mx + ms, my - ms);
        p.lineTo(mx - ms, my + ms);
        return p;
    }

    //── View ──────────────────────────────────────────────────────────────
    static juce::Path mixerIcon(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        // 3 vertical fader slots
        for (int i = 0; i < 3; ++i)
        {
            float x = r.getX() + r.getWidth() * (0.15f + i * 0.3f);
            float w = r.getWidth() * 0.12f;
            p.addRoundedRectangle(x, r.getY(), w, r.getHeight(), 1.5f);
            // Fader cap at varying positions
            float capY = r.getY() + r.getHeight() * (0.3f + i * 0.15f);
            p.addRoundedRectangle(x - 2, capY, w + 4, r.getHeight() * 0.12f, 1.0f);
        }
        return p;
    }

    static juce::Path pianoRollIcon(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.18f);
        // Piano keys outline
        p.addRectangle(r);
        int numKeys = 5;
        float keyW = r.getWidth() / numKeys;
        for (int i = 1; i < numKeys; ++i)
            p.addRectangle(r.getX() + i * keyW, r.getY(), 1.0f, r.getHeight());
        // Black keys
        float bkH = r.getHeight() * 0.55f;
        float bkW = keyW * 0.6f;
        for (int i : {0, 1, 3})
            p.addRectangle(r.getX() + (i + 1) * keyW - bkW * 0.5f, r.getY(), bkW, bkH);
        return p;
    }

    static juce::Path sessionViewIcon(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.18f);
        // Grid of clips
        float cellW = r.getWidth() / 3.0f;
        float cellH = r.getHeight() / 3.0f;
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
                p.addRoundedRectangle(r.getX() + col * cellW + 1, r.getY() + row * cellH + 1,
                                      cellW - 2, cellH - 2, 1.5f);
        return p;
    }

    static juce::Path spectrumIcon(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.18f);
        // Frequency bars
        int numBars = 5;
        float barW = r.getWidth() / (numBars * 1.5f);
        float heights[] = { 0.4f, 0.7f, 1.0f, 0.6f, 0.3f };
        for (int i = 0; i < numBars; ++i)
        {
            float x = r.getX() + i * (barW * 1.5f);
            float h = r.getHeight() * heights[i];
            p.addRoundedRectangle(x, r.getBottom() - h, barW, h, 1.0f, 1.0f, true, true, false, false);
        }
        return p;
    }

    //── AI ────────────────────────────────────────────────────────────────
    static juce::Path aiBrain(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        // Simplified brain: two lobes
        p.addEllipse(r.getX(), r.getY() + r.getHeight() * 0.1f,
                     r.getWidth() * 0.55f, r.getHeight() * 0.75f);
        p.addEllipse(r.getX() + r.getWidth() * 0.35f, r.getY(),
                     r.getWidth() * 0.65f, r.getHeight() * 0.8f);
        // Stem
        p.addRoundedRectangle(r.getCentreX() - 2, r.getBottom() - r.getHeight() * 0.2f,
                              4, r.getHeight() * 0.2f, 1.0f);
        return p;
    }

    static juce::Path stemSplit(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        float cx = r.getCentreX();
        // Waveform top
        p.startNewSubPath(r.getX(), r.getCentreY());
        p.cubicTo(r.getX() + r.getWidth() * 0.25f, r.getY(),
                  r.getX() + r.getWidth() * 0.5f, r.getY(),
                  cx, r.getCentreY());
        // Split into two paths
        p.cubicTo(cx + r.getWidth() * 0.15f, r.getY() + r.getHeight() * 0.2f,
                  r.getRight() - r.getWidth() * 0.1f, r.getY() + r.getHeight() * 0.15f,
                  r.getRight(), r.getY() + r.getHeight() * 0.3f);
        p.startNewSubPath(cx, r.getCentreY());
        p.cubicTo(cx + r.getWidth() * 0.15f, r.getBottom() - r.getHeight() * 0.2f,
                  r.getRight() - r.getWidth() * 0.1f, r.getBottom() - r.getHeight() * 0.15f,
                  r.getRight(), r.getBottom() - r.getHeight() * 0.3f);
        return p;
    }

    static juce::Path autoTuneIcon(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.18f);
        // Musical note
        float stemX = r.getRight() - r.getWidth() * 0.3f;
        p.addEllipse(r.getX() + r.getWidth() * 0.1f, r.getBottom() - r.getHeight() * 0.35f,
                     r.getWidth() * 0.35f, r.getHeight() * 0.3f);
        p.addRectangle(stemX - 1, r.getY(), 2.5f, r.getHeight() * 0.7f);
        // Magic sparkles
        float sx = r.getRight() - r.getWidth() * 0.15f;
        float sy = r.getY() + r.getHeight() * 0.15f;
        p.startNewSubPath(sx, sy - 3);
        p.lineTo(sx, sy + 3);
        p.startNewSubPath(sx - 3, sy);
        p.lineTo(sx + 3, sy);
        return p;
    }

    static juce::Path humanizer(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        // Person silhouette
        float cx = r.getCentreX();
        p.addEllipse(cx - r.getWidth() * 0.15f, r.getY(),
                     r.getWidth() * 0.3f, r.getHeight() * 0.3f);
        // Body
        p.startNewSubPath(cx - r.getWidth() * 0.3f, r.getBottom());
        p.lineTo(cx - r.getWidth() * 0.15f, r.getY() + r.getHeight() * 0.35f);
        p.lineTo(cx + r.getWidth() * 0.15f, r.getY() + r.getHeight() * 0.35f);
        p.lineTo(cx + r.getWidth() * 0.3f, r.getBottom());
        p.closeSubPath();
        return p;
    }

    static juce::Path textToSample(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.18f);
        // Text "T"
        p.addRectangle(r.getX(), r.getY(), r.getWidth() * 0.4f, 2.5f);
        p.addRectangle(r.getX() + r.getWidth() * 0.15f, r.getY(),
                       2.5f, r.getHeight() * 0.45f);
        // Arrow right
        float ax = r.getCentreX();
        float ay = r.getCentreY();
        p.startNewSubPath(ax - r.getWidth() * 0.1f, ay);
        p.lineTo(ax + r.getWidth() * 0.1f, ay);
        p.startNewSubPath(ax + r.getWidth() * 0.05f, ay - 3);
        p.lineTo(ax + r.getWidth() * 0.1f, ay);
        p.lineTo(ax + r.getWidth() * 0.05f, ay + 3);
        // Waveform
        float waveX = r.getX() + r.getWidth() * 0.6f;
        float waveW = r.getWidth() * 0.35f;
        for (int i = 0; i < 5; ++i)
        {
            float x = waveX + i * (waveW / 5.0f);
            float h = (i % 2 == 0 ? 0.3f : 0.15f) * r.getHeight();
            p.addRectangle(x, r.getBottom() - h, waveW / 7.0f, h);
        }
        return p;
    }

    static juce::Path autoMix(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.18f);
        // Fader bars with AI sparkle
        for (int i = 0; i < 3; ++i)
        {
            float x = r.getX() + i * r.getWidth() * 0.35f;
            float w = r.getWidth() * 0.18f;
            float h = r.getHeight() * (0.5f + i * 0.15f);
            p.addRoundedRectangle(x, r.getBottom() - h, w, h, 1.0f);
        }
        // Star/sparkle
        float sx = r.getRight() - 3;
        float sy = r.getY() + 3;
        p.startNewSubPath(sx, sy - 3);
        p.lineTo(sx + 1, sy - 1);
        p.lineTo(sx + 3, sy);
        p.lineTo(sx + 1, sy + 1);
        p.lineTo(sx, sy + 3);
        p.lineTo(sx - 1, sy + 1);
        p.lineTo(sx - 3, sy);
        p.lineTo(sx - 1, sy - 1);
        p.closeSubPath();
        return p;
    }

    //── Transport ─────────────────────────────────────────────────────────
    static juce::Path play(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.25f);
        p.addTriangle(r.getX(), r.getY(), r.getX(), r.getBottom(), r.getRight(), r.getCentreY());
        return p;
    }

    static juce::Path stop(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.28f);
        p.addRoundedRectangle(r, 1.5f);
        return p;
    }

    static juce::Path record(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.25f);
        p.addEllipse(r);
        return p;
    }

    static juce::Path rewind(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.22f);
        float mid = r.getCentreX();
        // Two left-pointing triangles
        p.addTriangle(mid, r.getY(), mid, r.getBottom(), r.getX(), r.getCentreY());
        p.addTriangle(r.getRight(), r.getY(), r.getRight(), r.getBottom(), mid, r.getCentreY());
        return p;
    }

    static juce::Path loop(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        float rad = r.getHeight() * 0.35f;
        p.addArc(r.getX(), r.getCentreY() - rad, r.getWidth(), rad * 2,
                 juce::MathConstants<float>::pi * 0.15f,
                 juce::MathConstants<float>::pi * 1.85f, true);
        // Arrowhead
        float ax = r.getX() + r.getWidth() * 0.65f;
        float ay = r.getCentreY() - rad + 1;
        p.addTriangle(ax - 4, ay, ax + 4, ay, ax, ay - 5);
        return p;
    }

    static juce::Path settings(juce::Rectangle<float> area)
    {
        juce::Path p;
        auto r = area.reduced(area.getWidth() * 0.2f);
        float cx = r.getCentreX(), cy = r.getCentreY();
        float outerR = r.getWidth() * 0.45f;
        float innerR = outerR * 0.6f;
        int teeth = 8;
        // Gear teeth
        for (int i = 0; i < teeth; ++i)
        {
            float angle1 = i * juce::MathConstants<float>::twoPi / teeth;
            float angle2 = angle1 + juce::MathConstants<float>::twoPi / (teeth * 3);
            float angle3 = angle1 + juce::MathConstants<float>::twoPi / (teeth * 1.5f);
            if (i == 0) p.startNewSubPath(cx + std::cos(angle1) * outerR, cy + std::sin(angle1) * outerR);
            else        p.lineTo(cx + std::cos(angle1) * outerR, cy + std::sin(angle1) * outerR);
            p.lineTo(cx + std::cos(angle2) * outerR, cy + std::sin(angle2) * outerR);
            p.lineTo(cx + std::cos(angle2) * innerR, cy + std::sin(angle2) * innerR);
            p.lineTo(cx + std::cos(angle3) * innerR, cy + std::sin(angle3) * innerR);
        }
        p.closeSubPath();
        // Center hole
        p.addEllipse(cx - innerR * 0.4f, cy - innerR * 0.4f, innerR * 0.8f, innerR * 0.8f);
        return p;
    }

private:
    DAWIcons() = delete; // Static-only class
};
