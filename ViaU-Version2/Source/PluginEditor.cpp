#include "PluginEditor.h"
#include <cmath>

ViaUAudioProcessorEditor::ViaUAudioProcessorEditor(ViaUAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(360, 180);
    displayModeBox.addItem("LED", 1);
    displayModeBox.addItem("Needle", 2);
    addAndMakeVisible(displayModeBox);
    displayModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "displayMode", displayModeBox);
    startTimerHz(30);
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

juce::Colour ViaUAudioProcessorEditor::interpolateColour(float vu, float vuStart, float vuEnd, juce::Colour startCol, juce::Colour endCol)
{
    float t = juce::jlimit(0.0f, 1.0f, (vu - vuStart) / (vuEnd - vuStart));
    return startCol.interpolatedWith(endCol, t);
}

void ViaUAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    auto r = getLocalBounds().toFloat().reduced(12.0f);
    auto meterBounds = r.removeFromTop(r.getHeight() - 40.0f);

    int mode = displayModeBox.getSelectedId();
    if (mode == 1)
        drawLedMeter(g, meterBounds, vuValue);
    else
        drawNeedleMeter(g, meterBounds, vuValue);
}

// --- drawLedMeter with gradient ---
void ViaUAudioProcessorEditor::drawLedMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float vu)
{
    auto meterBounds = bounds.reduced(6.0f);
    auto outline = meterBounds.withHeight(meterBounds.getHeight() - 6.0f);

    g.setColour(juce::Colours::dimgrey);
    g.fillRoundedRectangle(outline, 8.0f);

    const float norm = juce::jlimit(0.0f, 1.0f, (vu + 20.0f) / 23.0f);
    auto fill = outline.withWidth(outline.getWidth() * norm);

    juce::Colour fillColor;
    if (vu <= -6.0f)
        fillColor = juce::Colours::green;
    else if (vu <= -3.0f)
        fillColor = interpolateColour(vu, -6.0f, -3.0f, juce::Colours::yellow, juce::Colours::orange);
    else
        fillColor = interpolateColour(vu, -3.0f, 3.0f, juce::Colours::orange, juce::Colours::red);

    if (processor.isPeakHit())
        fillColor = juce::Colours::red;

    g.setColour(fillColor);
    g.fillRoundedRectangle(fill.reduced(2.0f), 6.0f);

    // Ticks
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(juce::Font(12.0f));
    for (float tickVU : { -20.0f, -10.0f, -5.0f, -3.0f, 0.0f, 3.0f })
    {
        float x = outline.getX() + outline.getWidth() * juce::jlimit(0.0f, 1.0f, (tickVU + 20.0f) / 23.0f);
        g.drawVerticalLine((int)std::round(x), outline.getY() - 4.0f, outline.getBottom() + 4.0f);
        g.drawText(juce::String(tickVU, 0), (int)x - 10, (int)outline.getBottom() + 2, 20, 14, juce::Justification::centred);
    }
}

// --- drawNeedleMeter with gradient arcs ---
void ViaUAudioProcessorEditor::drawNeedleMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float vu)
{
    auto r = bounds.reduced(8.0f);
    float size = juce::jmin(r.getWidth(), r.getHeight());
    auto dial = juce::Rectangle<float>(0, 0, size, size).withCentre(r.getCentre());

    g.setColour(juce::Colours::dimgrey);
    g.fillEllipse(dial);
    g.setColour(juce::Colours::black);
    g.fillEllipse(dial.reduced(6.0f));

    auto drawGradientArc = [&](float vuStart, float vuEnd, juce::Colour startCol, juce::Colour endCol)
        {
            const int numSegments = 50;
            for (int i = 0; i < numSegments; ++i)
            {
                float t1 = (float)i / numSegments;
                float t2 = (float)(i + 1) / numSegments;
                float vu1 = juce::jmap(t1, 0.0f, 1.0f, vuStart, vuEnd);
                float vu2 = juce::jmap(t2, 0.0f, 1.0f, vuStart, vuEnd);

                float angle1 = juce::degreesToRadians(juce::jmap(vu1, -20.0f, 3.0f, 230.0f, -50.0f));
                float angle2 = juce::degreesToRadians(juce::jmap(vu2, -20.0f, 3.0f, 230.0f, -50.0f));

                juce::Point<float> c = dial.getCentre();
                float rOuter = dial.getWidth() * 0.48f;
                float rInner = rOuter - 8.0f;

                juce::Point<float> p1(c.x + rInner * std::cos(angle1), c.y + rInner * std::sin(angle1));
                juce::Point<float> p2(c.x + rOuter * std::cos(angle1), c.y + rOuter * std::sin(angle1));
                juce::Point<float> p3(c.x + rOuter * std::cos(angle2), c.y + rOuter * std::sin(angle2));
                juce::Point<float> p4(c.x + rInner * std::cos(angle2), c.y + rInner * std::sin(angle2));

                juce::Path segment;
                segment.startNewSubPath(p1);
                segment.lineTo(p2);
                segment.lineTo(p3);
                segment.lineTo(p4);
                segment.closeSubPath();
                g.setColour(startCol.interpolatedWith(endCol, (float)i / numSegments));
                g.fillPath(segment);
            }
        };

    drawGradientArc(-20.0f, -6.0f, juce::Colours::green, juce::Colours::green);
    drawGradientArc(-6.0f, -3.0f, juce::Colours::yellow, juce::Colours::orange);
    drawGradientArc(-3.0f, 3.0f, juce::Colours::orange, juce::Colours::red);

    // Needle
    auto c = dial.getCentre();
    float rNeedle = dial.getWidth() * 0.45f;
    float angle = juce::degreesToRadians(juce::jmap(vu, -20.0f, 3.0f, 230.0f, -50.0f));
    juce::Point<float> needleTip(c.x + rNeedle * std::cos(angle),
        c.y + rNeedle * std::sin(angle));

    g.setColour(processor.isPeakHit() ? juce::Colours::red : juce::Colours::white);
    g.drawLine(c.x, c.y, needleTip.x, needleTip.y, 2.0f);

    // Center pin
    g.setColour(juce::Colours::darkgrey);
    g.fillEllipse(c.x - 4.0f, c.y - 4.0f, 8.0f, 8.0f);
}
