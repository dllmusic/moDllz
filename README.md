#VCV_moDllz

![](https://github.com/dllmusic/VCV_moDllz/blob/master/manual_pics/Header.png)

## MIDI to dual CV interface

Based on : Core MIDI-To-CV

Simulates a duophonic keyboard sending 2 CVs / Vel / Gate (reTriggered) for Lower and Upper notes being played  
... if only one note is played Lower and Upper CVs are the same (Useful for unison / separate OSC with optional RingMod / Oscillator sync) 
There's also different options to retrigger Lower/Upper Gates independently.

![](https://github.com/dllmusic/VCV_moDllz/blob/master/manual_pics/DualCVpic.png)

[Duophonic Wiki](https://www.sequencer.de/synth/index.php/Duophonic)

## Twin Glider

Based on : Befaco Slew

Twin Glider is a dual linear glide/portamento module.
It limits the rate of change of the incoming signal with independent Rise and Fall times.

![](https://github.com/dllmusic/VCV_moDllz/blob/master/manual_pics/TwinGpic.png)

### Example usages: 
* Note portamento (1v/oct Input > Out to VCO)
* Get "slow" noise signals for random modulation (noise Input : S&G, Time mode)

![](https://github.com/dllmusic/VCV_moDllz/blob/master/manual_pics/SlowNoise.png)
* Wave shaping "filter" (signal Input: HiRate mode)

![](https://github.com/dllmusic/VCV_moDllz/blob/master/manual_pics/Waveshape.png)
* Generate Trigger signals at the beginning and ending of a Gate (Gate to Main Input (not the Gate input) zero times (ramps inactive) > rise+fall Trig Out)

![](https://github.com/dllmusic/VCV_moDllz/blob/master/manual_pics/Gate2Trigger.png)
* Generate Gates from Trigger (Trigger Input : with Rise (or Fall) time > Rise (or Fall) Gate Out)
* Envelope Follower (Audio Input : with Fall time

![](https://github.com/dllmusic/VCV_moDllz/blob/master/manual_pics/EnvFollower.png)

### [Example Patches](https://github.com/dllmusic/VCV_moDllz/blob/master/patches/moDllzVCVpatches.zip?raw=true)

### [PDF Manual](https://github.com/dllmusic/VCV_moDllz/blob/master/moDllz_manual.pdf)

![](https://github.com/dllmusic/VCV_moDllz/blob/master/manual_pics/Footer.png)
