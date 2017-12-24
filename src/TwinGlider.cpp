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
        RISEONTIME_PARAM=6,
        FALLONTIME_PARAM=8,
        TRIG_PARAM=10,
        NUM_PARAMS=12
    };
    enum InputIds {
        RISE_INPUT,
        FALL_INPUT=2,
        GATE_INPUT=4,
        IN_INPUT=6,
        NUM_INPUTS=8
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
    bool fallontimeSW[2];
    bool docalc[2] = {true};
    bool gliding[2] = {false};
    bool newgate[2] = {false};
    bool pulse[2] = {false};
    bool triggerR[2] = {false};
    //int ix=0;

    void glidenow(int ix);
    
    PulseGenerator gatePulse[2];
    
    TwinGlider() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    ~TwinGlider() {
    };
    
    void step() override;
    
};

void ::TwinGlider::glidenow(int ix){
    //Calculate slope  if mode is constant time and new in while gliding
    if (((params[RISEONTIME_PARAM + ix].value) || (params[FALLONTIME_PARAM + ix].value)) && (in[ix] != memo[ix]) && (gliding[ix])) {
        docalc[ix] = true;
    }
    ///////
    
    if (inputs[RISE_INPUT + ix].active)
        rise[ix] = inputs[RISE_INPUT + ix].value / 10 * params[RISE_PARAM + ix].value;
    else rise[ix] = params[RISE_PARAM + ix].value;
    
    
    if (params[LINK_PARAM + ix].value) {
        fall[ix] = rise[ix];
        fallontimeSW[ix] = params[RISEONTIME_PARAM + ix].value;
    }else{
        if (inputs[FALL_INPUT + ix].active)
            fall[ix] = inputs[FALL_INPUT + ix].value / 10 * params[FALL_PARAM + ix].value;
        else fall[ix] = params[FALL_PARAM + ix].value;
        
        fallontimeSW[ix] = params[FALLONTIME_PARAM + ix].value;
    }
    
    
    if ( in[ix] > out[ix]) {
        /// Rise
        if (rise[ix] > 0){
            lights[RISING_LIGHT + ix].value = 1;
            lights[FALLING_LIGHT + ix].value = 0;
            outputs[GATERISE_OUTPUT + ix].value = 10;
            outputs[GATEFALL_OUTPUT + ix].value = 0;
            
            if (params[RISEONTIME_PARAM + ix].value) {
                if (docalc[ix]){
                    memo[ix] = in[ix];
                    glideval[ix] = (in[ix] - out[ix]) / (1/engineGetSampleRate()+ rise[ix] * rise[ix] * 10 * engineGetSampleRate());///////
                    docalc[ix] = false;
                }
            }else glideval[ix] = 1/(1 + rise[ix] * 2 * engineGetSampleRate());
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
        
    }else if (in[ix] < out[ix]) {
        /// Fall
        if (fall[ix] > 0) {
            lights[RISING_LIGHT + ix].value = 0;
            lights[FALLING_LIGHT + ix].value = 1;
            outputs[GATERISE_OUTPUT + ix].value = 0;
            outputs[GATEFALL_OUTPUT + ix].value = 10;
            
            if (fallontimeSW[ix]) {
                if (docalc[ix]){
                    memo[ix] = in[ix];
                    glideval[ix] = (out[ix] - in[ix]) / (1/engineGetSampleRate()+ fall[ix] * fall[ix] * 10 * engineGetSampleRate());///////
                    docalc[ix] = false;
                }
            }else glideval[ix] = 1/(1 + fall[ix] * 2 * engineGetSampleRate());
            
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




void ::TwinGlider::step() {
    
    int ix = 0;
    do {
 if (inputs[IN_INPUT + ix].active) {

     in[ix] = inputs[IN_INPUT + ix].value;
         //Check for legato from Gate
    if (inputs[GATE_INPUT + ix].active){
      
        if (inputs[GATE_INPUT + ix ].value < 0.5) {
            newgate[ix] = true;
            out[ix] = in[ix];
        }else if (newgate[ix]) {
                out[ix] = in[ix] ;
                newgate[ix] = false;
        }else glidenow(ix);
    }else glidenow(ix);

    outputs[OUT_OUTPUT + ix].value = out[ix];
    
  }///closing if input  ACTIVE
   ix++;
} while (ix < 2);
  
}//closing STEP



TwinGliderWidget::TwinGliderWidget() {
    ::TwinGlider *module = new ::TwinGlider();
    setModule(module);
    box.size = Vec(15*9, 380);
    
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
         //2nd panel Ypos = 197
        Ypos = 25 + i* 172;
  
    /// Gliding Leds
    addChild(createLight<TinyLight<RedLight>>(Vec(6.75, Ypos+3), module, TwinGlider::RISING_LIGHT + i));
    addChild(createLight<TinyLight<RedLight>>(Vec(125, Ypos+66), module, TwinGlider::FALLING_LIGHT + i));
        
    /// Glide Knobs
    addParam(createParam<moDLLzKnobM>(Vec(17, Ypos-3), module, TwinGlider::RISE_PARAM + i, 0.0, 1.0, 0.0));
    addParam(createParam<moDLLzKnobM>(Vec(78, Ypos-3), module, TwinGlider::FALL_PARAM + i, 0.0, 1.0, 0.0));
    
    Ypos += Ystep;
        
    // LINK SWITCH//CKSS
    addParam(createParam<moDLLzSwitch>(Vec(60, Ypos-12), module, TwinGlider::LINK_PARAM + i, 0.0, 1.0, 1.0));
   
    /// Glides CVs
    addInput(createInput<moDLLzPort>(Vec(22, Ypos+3), module, TwinGlider::RISE_INPUT + i));
    addInput(createInput<moDLLzPort>(Vec(89, Ypos+3), module, TwinGlider::FALL_INPUT + i));
    
    Ypos += Ystep;
        
    /// Mode switches
    addParam(createParam<moDLLzSwitch>(Vec(35, Ypos+3.5), module, TwinGlider::RISEONTIME_PARAM + i, 0.0, 1.0, 1.0));
    addParam(createParam<moDLLzSwitch>(Vec(85, Ypos+3.5), module, TwinGlider::FALLONTIME_PARAM + i, 0.0, 1.0, 1.0));

    Ypos += Ystep;
    
    /// GATES OUT
    addOutput(createOutput<moDLLzPort>(Vec(8, Ypos-14), module, TwinGlider::GATERISE_OUTPUT + i));
    addOutput(createOutput<moDLLzPort>(Vec(102, Ypos-14), module, TwinGlider::GATEFALL_OUTPUT + i));
    
    /// TRIGGERS OUT
    addOutput(createOutput<moDLLzPort>(Vec(31, Ypos-2), module, TwinGlider::TRIGRISE_OUTPUT + i));
    addOutput(createOutput<moDLLzPort>(Vec(55, Ypos-2), module, TwinGlider::TRIG_OUTPUT + i));
    addOutput(createOutput<moDLLzPort>(Vec(79, Ypos-2), module, TwinGlider::TRIGFALL_OUTPUT + i));
    
    Ypos += Ystep;
    
    // IN GATEin OUT//PJ301MPort
    addInput(createInput<PJ301MPort>(Vec(13, Ypos+5), module, TwinGlider::IN_INPUT + i));
    addInput(createInput<moDLLzPort>(Vec(55, Ypos+7), module, TwinGlider::GATE_INPUT + i));
    addOutput(createOutput<PJ301MPort>(Vec(98, Ypos+5), module, TwinGlider::OUT_OUTPUT + i));
    
    }
    
}
void TwinGliderWidget::step() {
    ModuleWidget::step();
}
