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

This is a dual linear glide/portamento. 
You can have independent Rise and Fall times, or you can link both (to the Rise value).
The ramps can be set at constant speed (time relative to the voltage difference) or at constant time (independent from voltage difference).
It has a Gate-in to produce "legato" portamento, so when receiving note gates only legato notes have portamento.
There's independent trigger outputs for the finished Rise and Fall ramps and a common one that triggers on both.
