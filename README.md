# VCV_moDllz 
VCV Rack plugins

# MIDI to dual CV interface:

Based on : Core MIDI-To-CV

Simulates a duophonic keyboard sending 2 CVs / Vel / Gate (reTriggered) for Lower and Upper notes  
... if only one note is present CVs are the same (Useful for RingMod / Syncing Oscillators)  

Retrigger options are:  
-Upper ReTrigger (All / Only)  
All: retriggers on every Upper changed note.  
Only: retriggers when the incoming Upper note is higher than the previous one.  
-Lower ReTrigger (All / Only)  
All: retriggers on every Lower changed note.  
Only: retriggers when the incoming Lower note is lower than the previous one.  
-Legato: No retrigger (from common Gate output)

It has outputs from PitchWheel, Modulation and AfterTouch like the original CORE: MIDI-TO-CV.  
Pedal (cc64) holds the gate, so only simultaneous pressed notes (one or two) are held.

Example patch...https://github.com/dllmusic/VCV_moDllz/blob/master/MIDIdualCV.vcv  
Wiki...https://www.sequencer.de/synth/index.php/Duophonic

# Twin Glider

Based on : Befaco Slew

Twin Glider is a dual linear glide/portamento module.
It limits the rate of change of the incoming signal with independent Rise and Fall times.

RISE and FALL knobs control the rate/time of change.
CV-in modulation is delimited by the knob values.

MODE: Each ramp can be set at constant time (ramp time independent from voltage difference) or at constant rate (ramp time relative to  voltage difference).

LINK: the Fall (time CV and time mode) to the Rise values.

RGATE/ FGATE: Rise and Fall output Gate while happening. 

RTRIG/FTRIG: Rise and Fall output Triggers when done. 

TRIG: RTRIG and FTRIG
You can use TRIG to output triggers at the starting and ending of a Gate connected to the In, with zero Fall/Rise times

GATE IN: When connected, the Rise/Fall ramps are active only if a Gate is received. Connect note gates here to produce portamento on legato notes only.
