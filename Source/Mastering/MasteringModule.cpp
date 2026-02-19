#include "MasteringModule.h"

MasteringModule::MasteringModule(AudioEngine& e) : audioEngine(e) {}
MasteringModule::~MasteringModule() {}

void MasteringModule::paint(juce::Graphics&) {}
void MasteringModule::resized() {}

void MasteringModule::prepare(const juce::dsp::ProcessSpec&) {}
void MasteringModule::processBlock(juce::AudioBuffer<float>&, double) {}

void MasteringModule::updateChain() {}
void MasteringModule::processEQ(juce::AudioBuffer<float>&) {}
void MasteringModule::processMidSide(juce::AudioBuffer<float>&, bool) {}
void MasteringModule::processMultibandComp(juce::AudioBuffer<float>&) {}
void MasteringModule::processSaturation(juce::AudioBuffer<float>&) {}
void MasteringModule::processLimiter(juce::AudioBuffer<float>&) {}
void MasteringModule::updateMeters(const juce::AudioBuffer<float>&) {}
float MasteringModule::computeLUFS(const juce::AudioBuffer<float>&) { return -70.0f; }

void MasteringModule::buildUI() {}

void MasteringModule::setEQBand(int, double, double, double, EQBand::Type) {}
void MasteringModule::setMBThreshold(int, float) {}
void MasteringModule::setMBRatio(int, float) {}
void MasteringModule::setMBAttack(int, float) {}
void MasteringModule::setMBRelease(int, float) {}
