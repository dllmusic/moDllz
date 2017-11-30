# VCV_moDllz
VCV Rack plugins

<h2> MIDI to dual CV interface:</h2>
<p>
Based on Core: MIDI-To-CV
</p>
<p>
Simulates a duophonic keyboard sending 2 CVs / Vel / Gate (reTriggered) for Lower and Upper notes <br/>...
if only one note is present CVs are the same 
(Useful for RingMod / Syncing Oscillators)
<br/><br/>
Retrigger options are: 
<br/>
-Upper ReTrigger (All / Only) <br/> 
All: retriggers on every Upper changed note. <br/>
Only: retriggers when new the new note is higher than the previous one. <br/>
-Lower ReTrigger (All / Only) <br/> 
All: retriggers on every Lower changed note. <br/>
Only: retriggers when new the new note is lower than the previous one.<br/>
-Legato: No retrigger (from common Gate output)
</p>
<p>
It has outputs to PitchWheel, Modulation and AfterTouch like the original CORE: MIDI-TO-CV.<br/>
Pedal (cc64) holds the gate. So only simultaneous pressed notes (one or two) are held.
</p>
Check out included patch...https://github.com/dllmusic/VCV_moDllz/blob/master/MIDIdualCV.vcv
<br/>
Wiki...https://www.sequencer.de/synth/index.php/Duophonic
