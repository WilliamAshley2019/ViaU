#include "PluginEditor.h"
#include <cmath>

ViaUAudioProcessorEditor::ViaUAudioProcessorEditor(ViaUAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(360, 180);

    // Display mode selector
    displayModeBox.addItem("LED", 1);
    displayModeBox.addItem("Needle", 2);
    addAndMakeVisible(displayModeBox);

    displayModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "displayMode", displayModeBox);

    startTimerHz(30); // Refresh ~30 FPS
}

void ViaUAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced(12);
    displayModeBox.setBounds(r.removeFromTop(24).removeFromLeft(150));
}

void ViaUAudioProcessorEditor::timerCallback()
{
    vuValue = processor.getCurrentVU();
    repaint();
}

void ViaUAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    auto area = getLocalBounds().toFloat().reduced(12.0f);
    auto header = area.removeFromTop(28.0f);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::Font(juce::FontOptions{ 15.0f }.withStyle("Bold")));
    g.drawText("ViaU — VU Meter", header, juce::Justification::centredLeft, false);

    area.removeFromTop(12.0f);

    // Choose meter style
    const bool useNeedle = (displayModeBox.getSelectedId() == 2);

    if (useNeedle)
        drawNeedleMeter(g, area, vuValue);
    else
        drawLedMeter(g, area, vuValue);
}

void ViaUAudioProcessorEditor::drawLedMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float vu)
{
    // LED bar: range -20 .. +3 VU
    auto meterBounds = bounds.reduced(6.0f);
    auto outline = meterBounds.withHeight(meterBounds.getHeight() - 6.0f);
    g.setColour(juce::Colours::dimgrey);
    g.fillRoundedRectangle(outline, 8.0f);

    // Normalize VU into 0..1
    const float norm = juce::jlimit(0.0f, 1.0f, (vu + 20.0f) / 23.0f);

    // Fill proportionally
    auto fill = outline.withWidth(outline.getWidth() * norm);
    g.setColour(juce::Colours::darkgrey);
    g.fillRoundedRectangle(fill.reduced(2.0f), 6.0f);

    // Colour zones: green (-20..-3), yellow (-3..0), red (0..+3)
    const float totalW = outline.getWidth();
    auto left = outline.getX();

    auto xGreenEnd = left + totalW * juce::jlimit(0.0f, 1.0f, (-3.0f + 20.0f) / 23.0f);
    auto xYellowEnd = left + totalW * juce::jlimit(0.0f, 1.0f, (0.0f + 20.0f) / 23.0f);
    auto xRedEnd = left + totalW;

    auto filledEndX = fill.getRight();

    // Draw filled sections up to filledEndX
    auto section = outline.reduced(2.0f);

    auto drawSection = [&](float x1, float x2, juce::Colour c)
        {
            if (filledEndX <= x1) return;
            float xe = juce::jmin(x2, filledEndX);
            if (xe > x1)
            {
                g.setColour(c);
                g.fillRoundedRectangle(juce::Rectangle<float>(x1, section.getY(), xe - x1, section.getHeight()), 4.0f);
            }
        };

    drawSection(section.getX(), xGreenEnd, juce::Colours::green);
    drawSection(xGreenEnd, xYellowEnd, juce::Colours::yellow);
    drawSection(xYellowEnd, xRedEnd, juce::Colours::red);

    // Ticks
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(juce::Font(juce::FontOptions{ 12.0f }));
    for (float tickVU : { -20.0f, -10.0f, -5.0f, -3.0f, 0.0f, +3.0f })
    {
        float x = outline.getX() + totalW * juce::jlimit(0.0f, 1.0f, (tickVU + 20.0f) / 23.0f);
        g.drawVerticalLine((int)std::round(x), outline.getY() - 4.0f, outline.getBottom() + 4.0f);
        g.drawText(juce::String(tickVU, 0), (int)x - 10, (int)outline.getBottom() + 2, 20, 14, juce::Justification::centred);
    }
}

void ViaUAudioProcessorEditor::drawNeedleMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float vu)
{
    // Draw a semicircular dial with -20..+3 VU mapping to -50..+50 degrees
    auto r = bounds.reduced(8.0f);
    float size = juce::jmin(r.getWidth(), r.getHeight());
    auto dial = juce::Rectangle<float>(0, 0, size, size).withCentre(r.getCentre());

    // Background
    g.setColour(juce::Colours::dimgrey);
    g.fillEllipse(dial);

    g.setColour(juce::Colours::black);
    g.fillEllipse(dial.reduced(6.0f));

    // Scale arc
    g.setColour(juce::Colours::lightgrey);
    juce::Path arc;
    float startDeg = juce::degreesToRadians(230.0f); // left
    float endDeg = juce::degreesToRadians(-50.0f); // right (wrap)
    arc.addArc(dial.getX() + 8.0f, dial.getY() + 8.0f, dial.getWidth() - 16.0f, dial.getHeight() - 16.0f,
        startDeg, endDeg, true);
    g.strokePath(arc, juce::PathStrokeType(2.0f));

    // Ticks and labels
    auto drawTick = [&](float vuTick, float len, float thickness, juce::String label = {})
        {
            const float t = juce::jlimit(0.0f, 1.0f, (vuTick + 20.0f) / 23.0f); // -20..+3 -> 0..1
            const float angle = juce::jmap(t, 230.0f, -50.0f) * juce::MathConstants<float>::pi / 180.0f;

            juce::Point<float> c = dial.getCentre();
            float rOuter = dial.getWidth() * 0.48f;
            float rInner = rOuter - len;

            juce::Point<float> p1(c.x + rInner * std::cos(angle), c.y + rInner * std::sin(angle));
            juce::Point<float> p2(c.x + rOuter * std::cos(angle), c.y + rOuter * std::sin(angle));

            g.drawLine(p1.x, p1.y, p2.x, p2.y, thickness);

            if (label.isNotEmpty())
            {
                auto tx = c.x + (rInner - 14.0f) * std::cos(angle);
                auto ty = c.y + (rInner - 14.0f) * std::sin(angle);
                g.drawFittedText(label, juce::Rectangle<int>((int)tx - 12, (int)ty - 8, 24, 16),
                    juce::Justification::centred, 1);
            }
        };

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    for (float tick : { -20.0f, -10.0f, -5.0f, -3.0f, 0.0f, +3.0f })
        drawTick(tick, 14.0f, 1.5f, juce::String(tick, 0));

    // Needle
    const float t = juce::jlimit(0.0f, 1.0f, (vu + 20.0f) / 23.0f);
    const float angleDeg = juce::jmap(t, 230.0f, -50.0f);
    const float angle = juce::degreesToRadians(angleDeg);

    juce::Point<float> c = dial.getCentre();
    float rNeedle = dial.getWidth() * 0.44f;

    g.setColour(juce::Colours::red);
    g.drawLine(c.x, c.y, c.x + rNeedle * std::cos(angle), c.y + rNeedle * std::sin(angle), 2.0f);

    // Hub
    g.setColour(juce::Colours::darkgrey);
    g.fillEllipse(c.x - 6.0f, c.y - 6.0f, 12.0f, 12.0f);

    // Title
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::Font(juce::FontOptions{ 14.0f }.withStyle("Bold")));
    g.drawText("VU", dial.withSizeKeepingCentre(dial.getWidth(), dial.getHeight() * 0.5f).toNearestInt(),
        juce::Justification::centredBottom, false);
}