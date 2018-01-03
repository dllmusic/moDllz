#include "dsp/digital.hpp"
#include "moDllz.hpp"
/*
 * TwinGlider
 */


struct TwinGlider : Module {
    enum ParamIds {
        RISE_PARAM,
        FALL_PARAM=2,
        LINK_PARAM=4,
        RISEMODE_PARAM=6,
        FALLMODE_PARAM=8,
        TRIG_PARAM=10,
        SMPNGLIDE_PARAM=12,
        NUM_PARAMS=14
    };
    enum InputIds {
        RISE_INPUT,
        FALL_INPUT=2,
        GATE_INPUT=4,
        CLOCK_INPUT=6,
        IN_INPUT=8,
        NUM_INPUTS=10
    };
    enum OutputIds {
        TRIGRISE_OUTPUT,
        TRIG_OUTPUT=2,
        TRIGFALL_OUTPUT=4,
        GATERISE_OUTPUT=6,
        GATEFALL_OUTPUT=8,
        OUT_OUTPUT=10,
        NUM_OUTPUTS=12
    };
    enum LightIds {
        RISING_LIGHT,
        FALLING_LIGHT=2,
        NUM_LIGHTS=4
    };
 
    
    float out[2] = {0.0};
    float in[2] = {0.0};
    float memo[2] = {0.0};
    float rise[2];
    float fall[2];
    float glideval[2];
    bool docalc[2] = {true};
    bool gliding[2] = {false};
    bool newgate[2] = {false};
    bool pulse[2] = {false};
    bool triggerR[2] = {false};
    bool thisIn[2] = {false};
    int clocksafe[2] = {0};
    void glidenow(int ix);
    
    PulseGenerator gatePulse[2];
    
   // SchmittTrigger clockTrigger[2];
    
    TwinGlider() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    ~TwinGlider() {
    };
    
    void step() override;
    
};

//////////////// GLIDE FUNCTION ////////////

void ::TwinGlider::glidenow(int ix){
    int risemodeSW = params[RISEMODE_PARAM + ix].value;
    int fallmodeSW = params[RISEMODE_PARAM + ix].value;
    ///// Calculate slope  if mode is constant time and new in while gliding
    if (((risemodeSW == 2) || (fallmodeSW == 2)) && (in[ix] != memo[ix]) && (gliding[ix])) {
        docalc[ix] = true;
    }

        ///////RISE
    if ( in[ix] > out[ix]) {
        ///Check CVin and get Rise value
        if (inputs[RISE_INPUT + ix].active)
            rise[ix] = inputs[RISE_INPUT + ix].value / 10 * params[RISE_PARAM + ix].value;
        else rise[ix] = params[RISE_PARAM + ix].value;
        ///
        if (rise[ix] > 0){
            lights[RISING_LIGHT + ix].value = 1;
            lights[FALLING_LIGHT + ix].value = 0;
            outputs[GATERISE_OUTPUT + ix].value = 10;
            outputs[GATEFALL_OUTPUT + ix].value = 0;
            switch (risemodeSW) {
                case 0: /// Hi Rate
                    glideval[ix] = 1/(1 + rise[ix] * 0.01 * engineGetSampleRate());
                    break;
                case 1: /// Rate
                    glideval[ix] = 1/(1 + rise[ix] * 2 * engineGetSampleRate());
                    break;
                case 2: /// Time
                    if (docalc[ix]){
                        memo[ix] = in[ix];
                        glideval[ix] = (in[ix] - out[ix]) / (engineGetSampleTime()+ rise[ix] * rise[ix] * 10 * engineGetSampleRate());
                        docalc[ix] = false;
                    }
                    break;
                default:
                    break;
            }
            //////////////////RISING//////////
            out[ix] += glideval[ix];
            
            if (out[ix] < in[ix]){
                gliding[ix] = true;
            }else{
                gliding[ix] = false;
                docalc[ix] = true;
                triggerR[ix] = true;
                gatePulse[ix].trigger(1e-3);
                out[ix] = in[ix];
                lights[RISING_LIGHT + ix].value = 0;
            }
        }else{
            out[ix] = in[ix];
            triggerR[ix] = true;
            gatePulse[ix].trigger(1e-3);
        }
        /// FALL
    }else if (in[ix] < out[ix]) {
        /// Check link CVin and get Fall value
        if (params[LINK_PARAM + ix].value) {
            fall[ix] = rise[ix];
        }else{
            if (inputs[FALL_INPUT + ix].active)
                fall[ix] = inputs[FALL_INPUT + ix].value / 10 * params[FALL_PARAM + ix].value;
            else fall[ix] = params[FALL_PARAM + ix].value;
        }
        //////////////
        if (fall[ix] > 0) {
            lights[RISING_LIGHT + ix].value = 0;
            lights[FALLING_LIGHT + ix].value = 1;
            outputs[GATERISE_OUTPUT + ix].value = 0;
            outputs[GATEFALL_OUTPUT + ix].value = 10;
            
            switch (fallmodeSW) {
                case 0:
                    glideval[ix] = 1/(1 + fall[ix] * 0.01 * engineGetSampleRate());
                    break;
                case 1:
                    glideval[ix] = 1/(1 + fall[ix] * 2 * engineGetSampleRate());
                    break;
                case 2:
                    if (docalc[ix]){
                        memo[ix] = in[ix];
                        glideval[ix] = (out[ix] - in[ix]) / (engineGetSampleTime()+ fall[ix] * fall[ix] * 10 * engineGetSampleRate());
                        docalc[ix] = false;
                    }
                    break;
                default:
                    break;
            }
            //////////////////FALLING//////////
            out[ix] -= glideval[ix];
            
            if (out[ix] > in[ix]){
                gliding[ix] = true;
            }else{
                gliding[ix] = false;
                docalc[ix] = true;
                triggerR[ix] = false;
                gatePulse[ix].trigger(1e-3);
                out[ix] = in[ix];
                lights[FALLING_LIGHT + ix].value = 0;
            }
        }else{
            out[ix] = in[ix];
            triggerR[ix] = false;
            gatePulse[ix].trigger(1e-3);
        }
    }else{
        /// stable
        gliding[ix] = false;
        docalc[ix] = true;
        out[ix] = in[ix];
        lights[RISING_LIGHT + ix].value = 0;
        lights[FALLING_LIGHT + ix].value = 0;
        outputs[GATERISE_OUTPUT + ix].value = 0;
        outputs[GATEFALL_OUTPUT + ix].value = 0;
    }
    
    
    if ((outputs[TRIGRISE_OUTPUT + ix].active)||(outputs[TRIG_OUTPUT + ix].active)||(outputs[TRIGFALL_OUTPUT + ix].active)) {
        pulse[ix] = gatePulse[ix].process(1.0 / engineGetSampleRate());
        outputs[TRIG_OUTPUT + ix].value = pulse[ix] ? 10.0 : 0.0;
        if (triggerR[ix]){
            outputs[TRIGRISE_OUTPUT + ix].value = pulse[ix] ? 10.0 : 0.0;
        }else{
            outputs[TRIGFALL_OUTPUT + ix].value = pulse[ix] ? 10.0 : 0.0;
        }
    }
    
}

///////////////////////////////////////////
/////////////// MAIN STEP //////////////////
/////////////////////////////////////////////

void ::TwinGlider::step() {
    
    int ix = 0;
    do {
 if (inputs[IN_INPUT + ix].active) {
 
    
      if (params[SMPNGLIDE_PARAM + ix].value)
        {
            thisIn[ix] = false;
            if (inputs[CLOCK_INPUT + ix].active){
             // External clock
          //   if (clockTrigger[ix].process(inputs[CLOCK_INPUT + ix].value)) thisIn[ix] = true;// allow in
                if ((clocksafe[ix]>8) && (inputs[CLOCK_INPUT + ix].value > 2.5)){
                    thisIn[ix] = true;
                    clocksafe[ix]=0;
                }else if ((clocksafe[ix]<10) && (inputs[CLOCK_INPUT + ix].value < 0.01)) clocksafe[ix]+=1;
            
            }
           else if (!(gliding[ix])) thisIn[ix] = true;
          
        if (thisIn[ix])  in[ix] = inputs[IN_INPUT + ix].value;////<<< input if sample clock or ramp done
        }else {
           in[ix] = inputs[IN_INPUT + ix].value;////<<< input normal
        }
     
    
     
     //Check for legato from Gate
    if (inputs[GATE_INPUT + ix].active){
      
        if (inputs[GATE_INPUT + ix ].value < 0.5) {
            newgate[ix] = true;
            out[ix] = in[ix];
        }else if (newgate[ix]) {
                out[ix] = in[ix] ;
                newgate[ix] = false;
        }else glidenow(ix);/// GLIDE !!!!!
    }else glidenow(ix);//// GLIDE !!!!

    outputs[OUT_OUTPUT + ix].value = out[ix];
 /// else from if input ACTIVE....
 }else{
     //disconnected in...reset Output if connected...
     outputs[OUT_OUTPUT + ix].value = 0;
     outputs[GATERISE_OUTPUT + ix].value = 0;
     outputs[GATEFALL_OUTPUT + ix].value = 0;
     lights[RISING_LIGHT + ix].value = 0;
     lights[FALLING_LIGHT + ix].value = 0;
     out[ix] = 0;
     in[ix] = 0;
     memo[ix] = 0;
     gliding[ix] = false;
     newgate[ix] = false;
     thisIn[ix] = false;
 }/// Closing if input  ACTIVE
   ix++;
} while (ix < 2);
  
}//closing STEP

/////////////////////////////////////////////
////////////////////////////////////////////
///////////////////////////////////////////

TwinGliderWidget::TwinGliderWidget() {
    ::TwinGlider *module = new ::TwinGlider();
    setModule(module);
    box.size = Vec(15*11, 380);
    
    {
        SVGPanel *panel = new SVGPanel();
        //Panel *panel = new DarkPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/TwinGlider.svg")));
        addChild(panel);
    }
    //Screws
    addChild(createScrew<ScrewBlack>(Vec(0, 0)));
    addChild(createScrew<ScrewBlack>(Vec(box.size.x - 15, 0)));
    addChild(createScrew<ScrewBlack>(Vec(0, 365)));
    addChild(createScrew<ScrewBlack>(Vec(box.size.x - 15, 365)));
    
    int Ystep = 30 ;
    int Ypos;
    
    for (int i=0; i<2; i++){
        
        Ypos = 25 + i* 173;  //2nd panel offset
  
    /// Gliding Leds
    addChild(createLight<TinyLight<RedLight>>(Vec(6.75, Ypos+1), module, TwinGlider::RISING_LIGHT + i));
    addChild(createLight<TinyLight<RedLight>>(Vec(154.75, Ypos+1), module, TwinGlider::FALLING_LIGHT + i));
    //// HiRate switch
    //addParam(createParam<moDllzSwitchH>(Vec(60, Ypos), module, TwinGlider::FINETIMING_PARAM + i, 0.0, 1.0, 0.0));
    /// Glide Knobs
    addParam(createParam<moDllzKnobM>(Vec(19, Ypos-4), module, TwinGlider::RISE_PARAM + i, 0.0, 1.0, 0.0));
    addParam(createParam<moDllzKnobM>(Vec(102, Ypos-4), module, TwinGlider::FALL_PARAM + i, 0.0, 1.0, 0.0));
        
    Ypos += Ystep; //55

    // LINK SWITCH//CKSS
    addParam(createParam<moDllzSwitchH>(Vec(72.5, Ypos-12), module, TwinGlider::LINK_PARAM + i, 0.0, 1.0, 1.0));
    /// Glides CVs
    addInput(createInput<moDllzPort>(Vec(23, Ypos+5.5), module, TwinGlider::RISE_INPUT + i));
    addInput(createInput<moDllzPort>(Vec(117.5, Ypos+5.5), module, TwinGlider::FALL_INPUT + i));
    
    Ypos += Ystep; //85

    /// Mode switches
    addParam(createParam<moDllzSwitchT>(Vec(55, Ypos-7), module, TwinGlider::RISEMODE_PARAM + i, 0.0, 2.0, 2.0));
    addParam(createParam<moDllzSwitchT>(Vec(100, Ypos-7), module, TwinGlider::FALLMODE_PARAM + i, 0.0, 2.0, 2.0));
    
    /// GATES OUT
    addOutput(createOutput<moDllzPort>(Vec(10.5, Ypos+14), module, TwinGlider::GATERISE_OUTPUT + i));
    addOutput(createOutput<moDllzPort>(Vec(130.5, Ypos+14), module, TwinGlider::GATEFALL_OUTPUT + i));
    
    Ypos += Ystep; //115
    
    /// TRIGGERS OUT
    addOutput(createOutput<moDllzPort>(Vec(43, Ypos-4.5), module, TwinGlider::TRIGRISE_OUTPUT + i));
    addOutput(createOutput<moDllzPort>(Vec(71, Ypos-4.5), module, TwinGlider::TRIG_OUTPUT + i));
    addOutput(createOutput<moDllzPort>(Vec(98, Ypos-4.5), module, TwinGlider::TRIGFALL_OUTPUT + i));
        
    Ypos += Ystep; //145

    // GATE IN
    addInput(createInput<moDllzPort>(Vec(44, Ypos+7), module, TwinGlider::GATE_INPUT + i));
    // CLOCK IN
    addInput(createInput<moDllzPort>(Vec(75, Ypos+7), module, TwinGlider::CLOCK_INPUT + i));
    // Sample&Glide SWITCH
    addParam(createParam<moDllzSwitch>(Vec(108, Ypos+18.5), module, TwinGlider::SMPNGLIDE_PARAM + i, 0.0, 1.0, 0.0));
    // IN OUT
    addInput(createInput<moDllzPort>(Vec(13.5, Ypos+6.5), module, TwinGlider::IN_INPUT + i));
    addOutput(createOutput<moDllzPort>(Vec(128.5, Ypos+6.5), module, TwinGlider::OUT_OUTPUT + i));

    
    }
    
}
void TwinGliderWidget::step() {
    ModuleWidget::step();
}
