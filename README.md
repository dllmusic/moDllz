# VCV_moDllz 
VCV Rack plugins

# MIDI to dual CV interface:

Based on : Core MIDI-To-CV

Simulates a duophonic keyboard sending 2 CVs / Vel / Gate (reTriggered) for Lower and Upper notes  
... if only one note is present CVs are the same (Useful for RingMod / Syncing Oscillators)  

Retrigger options are:  
-Upper ReTrigger (All / Only)  
All: retriggers on every Upper changed note.  
Only: retriggers when new the new note is higher than the previous one.  
-Lower ReTrigger (All / Only)  
All: retriggers on every Lower changed note.  
Only: retriggers when new the new note is lower than the previous one.  
-Legato: No retrigger (from common Gate output)

It has outputs to PitchWheel, Modulation and AfterTouch like the original CORE: MIDI-TO-CV.  
Pedal (cc64) holds the gate, so only simultaneous pressed notes (one or two) are held.

Example patch...https://github.com/dllmusic/VCV_moDllz/blob/master/MIDIdualCV.vcv  
Wiki...https://www.sequencer.de/synth/index.php/Duophonic