#include "VoiceManager.h"

void VoiceManager::TriggerNote(uint8_t noteNumber, uint8_t velocity) {
	uint8_t i = mNumVoices;
	while (i--) {
		if (!voices[i].isActive()) {
			voices[i].trigger(noteNumber,velocity);
			break;
		}
	}
}

void VoiceManager::ReleaseNote(uint8_t noteNumber, uint8_t velocity) {
	uint8_t i = mNumVoices;
	while (i--) {
		if (voices[i].mNote == noteNumber) {
			voices[i].release();
		}
	}
}

void VoiceManager::setFs(double fs)
{
	uint8_t i = mNumVoices;
	while (i--) {
		voices[i].setAudioFs(fs*mOversampling);
		voices[i].setModFs(fs/(MOD_FS_RAT+1));
	}
}

void VoiceManager::setNumVoices(uint8_t numVoices)
{
	mNumVoices = numVoices;
	voices = new Voice[numVoices];
}

double VoiceManager::tick() {
	mSampleCount++;
	uint8_t i = mNumVoices;
	uint8_t j;
	double output = 0;
	while (i--) {
		if (voices[i].isActive()) {
			if (!(mSampleCount & MOD_FS_RAT)) {
				voices[i].tickMod();
			}
			j = mOversampling;
			while (j--) {
				voices[i].tickAudio();
				output += voices[i].getOutput()*1.0/mOversampling;
			}
		}
		else {
			voices[i].reset();
		}
	}
	output = output * 1.0/ mNumVoices;
	return output;
}

void Voice::trigger(uint8_t noteNumber, uint8_t velocity)
{
	mNote = noteNumber;
	mVelocity = velocity*0.0078125;
	mOsc[0].setFreq(noteNumberToFrequency(mNote));
	mOsc[1].setFreq(noteNumberToFrequency(mNote));
	mOsc[2].setFreq(noteNumberToFrequency(mNote));
	mOsc[0].setGain(1.0);
	mOsc[1].setGain(1.0);
	mOsc[2].setGain(1.0);
	mOsc[0].sync();
	mOsc[1].sync();
	mOsc[2].sync();
	mLFOPitch.sync();
	mAmpEnv.trigger();
	tickMod();
}

void Voice::release()
{
	mAmpEnv.release();
}

void Voice::reset() {
	mNote = 0;
	mVelocity = 0;
}

void Voice::setAudioFs(double fs)
{
	mOsc[0].setFs(fs);
	mOsc[1].setFs(fs);
	mOsc[2].setFs(fs);
}

void Voice::setModFs(double fs) {
	mAmpEnv.setFs(fs);
	mLFOPitch.setFs(fs);
}

void Voice::tickAudio()
{
	mOsc[0].tick();
	mOsc[1].tick();
	mOsc[2].tick();
}

void Voice::tickMod() {
	mAmpEnv.tick();
	mLFOPitch.tick();

	mOsc[0].modFreq(mLFOPitch.getOutput());
	mOsc[0].applyMods();

	mOsc[1].modFreq(mLFOPitch.getOutput());
	mOsc[1].applyMods();

	mOsc[2].modFreq(mLFOPitch.getOutput());
	mOsc[2].applyMods();
}

double Voice::getOutput() {
	return mVelocity*mAmpEnv.getOutput()*(mOsc[0].getOutput()+mOsc[1].getOutput()+mOsc[2].getOutput());
}

bool Voice::isActive() {
	return !mAmpEnv.mIsDone;
}
