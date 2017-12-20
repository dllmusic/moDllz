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
        OUT_OUTPUT=6,
        NUM_OUTPUTS=8
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
    bool newgate[2] = {true};
    bool pulse[2] = {false};
    bool triggerR[2] = {false};
    //int ix=0;

    
    PulseGenerator gatePulse[2];
    
    TwinGlider() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    ~TwinGlider() {
    };
    
    void step() override;
    
};


void ::TwinGlider::step() {
    
    int ix = 0;
    do {
 if (inputs[IN_INPUT + ix].active) {

     in[ix] = inputs[IN_INPUT + ix].value;
         //Check for legato from Gate
    if (inputs[GATE_INPUT + ix ].value> 0.5) {
       // if (params[LEGATO_PARAM].value && newgate) {
         if (newgate[ix]) {
            out[ix] = in[ix] ;
            newgate[ix] = false;}
    }else{
        newgate[ix] = true;
    }

   //Calculate slope  if mode is constant time and new in while gliding
   
       if (((params[RISEONTIME_PARAM + ix].value) || (params[FALLONTIME_PARAM + ix].value)) && (in[ix] != memo[ix]) && (gliding[ix])) {
          docalc[ix] = true;
       }
   ///////

     rise[ix] = inputs[RISE_INPUT + ix].value / 10 + params[RISE_PARAM + ix].value;
 
     
    if (params[LINK_PARAM + ix].value) {
        fall[ix] = rise[ix];
        fallontimeSW[ix] = params[RISEONTIME_PARAM + ix].value;
    }else{
        fall[ix] = inputs[FALL_INPUT + ix].value / 10 + params[FALL_PARAM + ix].value;
        fallontimeSW[ix] = params[FALLONTIME_PARAM + ix].value;
    }
     

       if ( in[ix] > out[ix]) {
 /// Rise
           if (rise[ix] > 0){
                lights[RISING_LIGHT + ix].value = 1;
                lights[FALLING_LIGHT + ix].value = 0;
           
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
 
    outputs[OUT_OUTPUT + ix].value = out[ix];
    
  }///closing if input  ACTIVE
   ix++;
} while (ix < 2);
  
}//closing STEP



TwinGliderWidget::TwinGliderWidget() {
    ::TwinGlider *module = new ::TwinGlider();
    setModule(module);
    box.size = Vec(15*8, 380);
    
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
   addChild(createLight<TinyLight<RedLight>>(Vec(6.75, Ypos+4), module, TwinGlider::RISING_LIGHT + i));
   addChild(createLight<TinyLight<RedLight>>(Vec(109.5, Ypos+65), module, TwinGlider::FALLING_LIGHT + i));
    /// Gliding Leds
   // addChild(createLight<SmallLight<RedLight>>(Vec(5, Ypos+2), module, TwinGlider::RISING_LIGHT + i));
   // addChild(createLight<SmallLight<RedLight>>(Vec(108.5, Ypos+65), module, TwinGlider::FALLING_LIGHT + i));
        
    /// Glide Knobs

    addParam(createParam<RoundSmallBlackKnob>(Vec(20, Ypos), module, TwinGlider::RISE_PARAM + i, 0.0, 1.0, 0.0));
    addParam(createParam<RoundSmallBlackKnob>(Vec(72, Ypos), module, TwinGlider::FALL_PARAM + i, 0.0, 1.0, 0.0));
    //LINK SWITCH;
    Ypos += 28;//12
    addParam(createParam<CKSS>(Vec(53, Ypos), module, TwinGlider::LINK_PARAM + i, 0.0, 1.0, 1.0));
   
    /// Glides CVs
    Ypos += 4;//23
    addInput(createInput<PJ301MPort>(Vec(21, Ypos), module, TwinGlider::RISE_INPUT + i));
    addInput(createInput<PJ301MPort>(Vec(74, Ypos), module, TwinGlider::FALL_INPUT + i));
    /// Constant switches
    Ypos += Ystep + 5;//2
    addParam(createParam<CKSS>(Vec(30, Ypos), module, TwinGlider::RISEONTIME_PARAM + i, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>(Vec(76, Ypos), module, TwinGlider::FALLONTIME_PARAM + i, 0.0, 1.0, 1.0));

    /// TRIGGERS OUT
    Ypos += Ystep - 1;
    addOutput(createOutput<PJ301MPort>(Vec(15, Ypos-4), module, TwinGlider::TRIGRISE_OUTPUT + i));
    addOutput(createOutput<PJ301MPort>(Vec(48, Ypos-4), module, TwinGlider::TRIG_OUTPUT + i));
    addOutput(createOutput<PJ301MPort>(Vec(81, Ypos-4), module, TwinGlider::TRIGFALL_OUTPUT + i));
    
    // IN GATE OUT
    Ypos += Ystep + 2;
    addInput(createInput<PJ301MPort>(Vec(13, Ypos-1), module, TwinGlider::IN_INPUT + i));
    addInput(createInput<PJ301MPort>(Vec(48, Ypos), module, TwinGlider::GATE_INPUT + i));
    addOutput(createOutput<PJ301MPort>(Vec(83, Ypos-1), module, TwinGlider::OUT_OUTPUT + i));
    
    }
    
}
void TwinGliderWidget::step() {
    ModuleWidget::step();
}
