#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <memory>
#include <algorithm>

namespace {
using Layout=juce::AudioProcessorValueTreeState::ParameterLayout;
using ID=juce::ParameterID;
void f(Layout& l,const char* id,const char* label,float lo,float hi,float init,float step=0.f) {
    l.add(std::make_unique<juce::AudioParameterFloat>(ID{id,1},label,juce::NormalisableRange<float>(lo,hi,step),init));
}
void b(Layout& l,const char* id,const char* label,bool initial) {
    l.add(std::make_unique<juce::AudioParameterBool>(ID{id,1},label,initial));
}
void c(Layout& l,const char* id,const char* label,juce::StringArray names,int initial) {
    l.add(std::make_unique<juce::AudioParameterChoice>(ID{id,1},label,std::move(names),initial));
}
}

SkellAfarAudioProcessor::SkellAfarAudioProcessor():
    AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(),true)
        .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
    parameters(*this,nullptr,"SKELL_AFAR",createLayout()) {
    editorState.setProperty("scale",.75,nullptr);
    editorState.setProperty("preset","INIT",nullptr);
    editorState.setProperty("tourSeen",false,nullptr);
}
Layout SkellAfarAudioProcessor::createLayout() {
    Layout l;
    b(l,"power","Power",true);
    c(l,"space","Space", {"Tunnel","Sewer","Subway Platform","Cave","Cellar / Bunker",
        "Graveyard","Forest","Canyon / Valley","Open Field","City Alley","Parking Garage",
        "Stairwell","Cathedral","Empty Pool","Warehouse"},5);
    f(l,"effort","Effort",0.f,1.f,.5f);
    f(l,"throatTexture","Throat Texture",0.f,1.f,0.f);
    const auto logRange=juce::NormalisableRange<float>(.05f,200.f,
        [](float s,float e,float p){return s*std::pow(e/s,p);},
        [](float s,float e,float v){return std::log(std::max(v,s)/s)/std::log(e/s);});
    l.add(std::make_unique<juce::AudioParameterFloat>(ID{"distance",1},"Distance (m)",logRange,3.f));
    f(l,"angle","Angle",0.f,180.f,0.f);
    c(l,"occlusionType","Occlusion Material",{"Open","Doorway","Wall","Thick Wall","Foliage","Water"},0);
    f(l,"occlusionAmount","Occlusion Amount",0.f,1.f,0.f);
    f(l,"size","Space Size",.5f,2.f,1.f);
    f(l,"ambience","Ambience dB",-60.f,12.f,-20.f);
    f(l,"presenceHold","Presence Hold",0.f,1.f,0.f);
    f(l,"width","Width",0.f,2.f,1.f);
    c(l,"recordedAs","Recorded As",{"Whisper","Soft","Normal","Loud","Auto"},2);
    b(l,"bypassEffort","Bypass Effort Timbre",false);
    c(l,"micType","Microphone",{"Phone Memo","Handheld","Shotgun","Lavalier","Studio Condenser","Dictaphone","Walkie Talkie","Ideal"},7);
    c(l,"micStereo","Mic Stereo Pattern",{"Mono","XY","ORTF","Binaural"},1);
    f(l,"mix","Mix",0.f,1.f,1.f);
    f(l,"output","Output dB",-12.f,12.f,0.f);
    b(l,"delta","Delta",false);
    b(l,"removeDelay","Beat Aligned / Remove Arrival Delay",true);
    b(l,"walkOn","Walk Enabled",false);
    c(l,"walkPath","Walk Path",{"Approach","Retreat","Pass By","Pacing","Run Away"},0);
    f(l,"walkLength","Walk Length (bars)",1.f,16.f,4.f,1.f);
    b(l,"walkSync","Walk Sync",true);
    f(l,"walkScale","Walk Scale",0.1f,2.f,1.f);
    for (int i=1;i<=8;++i) {
        const auto id="bed"+juce::String(i)+"_level";
        const auto name="Bed "+juce::String(i)+" level";
        l.add(std::make_unique<juce::AudioParameterFloat>(ID{id,1},name,juce::NormalisableRange<float>(-60.f,12.f),-20.f));
    }
    f(l,"temperature","Temperature C",-10.f,45.f,20.f);
    f(l,"humidity","Humidity %",0.f,100.f,50.f);
    f(l,"airAbsorption","Air Absorption",0.f,1.f,.7f);
    c(l,"quality","Quality",{"Eco","Normal","High"},1);
    c(l,"oversampling","Oversampling",{"Off","2x","4x"},0);
    f(l,"seed","Ambience Seed",1.f,65535.f,12345.f,1.f);
    b(l,"seedLock","Lock Seed",true);
    b(l,"ambienceDuck","Ambience Duck",false);
    b(l,"muteAmbience","Mute Ambience",false);
    return l;
}
bool SkellAfarAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    const auto in=l.getMainInputChannelSet(), out=l.getMainOutputChannelSet();
    return (in==juce::AudioChannelSet::mono() || in==juce::AudioChannelSet::stereo()) &&
           (out==juce::AudioChannelSet::mono() || out==juce::AudioChannelSet::stereo()) &&
           in.size()<=out.size();
}
void SkellAfarAudioProcessor::prepareToPlay(double sr,int) {
    sampleRateHz=sr;
    engine.prepare(sr,getTotalNumOutputChannels(),static_cast<int>(get("quality")));
    setLatencySamples(engine.latencySamples()); // includes limiter, independently of wet/mix/power
    effortSmooth.reset(sr,.08); distanceSmooth.reset(sr,.15);
    mixSmooth.reset(sr,.02); bypassSmooth.reset(sr,.01);
    effortSmooth.setCurrentAndTargetValue(get("effort"));
    distanceSmooth.setCurrentAndTargetValue(get("distance"));
    mixSmooth.setCurrentAndTargetValue(get("mix"));
    bypassSmooth.setCurrentAndTargetValue(get("power")>.5f ? 0.f:1.f);
    walkBeat=0.; lastScene=-1;
}
void SkellAfarAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals denormals;
    const int n=buffer.getNumSamples();
    if (n==0 || buffer.getNumChannels()==0) return;
    const int inputs=std::max(1,getTotalNumInputChannels());
    const int outputs=std::clamp(getTotalNumOutputChannels(),1,2);
    if (inputs==1 && outputs==2) buffer.copyFrom(1,0,buffer,0,0,n);
    for (int ch=outputs;ch<buffer.getNumChannels();++ch) buffer.clear(ch,0,n);
    auto* l=buffer.getWritePointer(0);
    auto* r=outputs==2 ? buffer.getWritePointer(1):nullptr;
    afar::SceneControls s;
    s.throatTexture=get("throatTexture");
    s.angle=get("angle");s.occlusion=get("occlusionAmount");s.occlusionType=static_cast<int>(get("occlusionType"));
    s.size=get("size");s.ambienceDb=get("ambience");s.presence=get("presenceHold");s.width=get("width");
    s.recordedAs=static_cast<int>(get("recordedAs"));
    s.micType=static_cast<int>(get("micType"));s.micStereo=static_cast<int>(get("micStereo"));
    s.outputDb=get("output");s.absorption=get("airAbsorption");
    s.temperature=get("temperature");s.humidity=get("humidity");
    s.removeDelay=get("removeDelay")>.5f;s.bypassEffort=get("bypassEffort")>.5f;
    s.ambienceDuck=get("ambienceDuck")>.5f;s.muteAmbience=get("muteAmbience")>.5f;
    s.delta=get("delta")>.5f;
    s.bedGain=0.f;
    constexpr const char* bedIDs[] {"bed1_level","bed2_level","bed3_level","bed4_level",
                                    "bed5_level","bed6_level","bed7_level","bed8_level"};
    for (int i=0;i<8;++i) {
        s.beds[static_cast<std::size_t>(i)]=get(bedIDs[i]);
        s.bedGain+=afar::dbGain(s.beds[static_cast<std::size_t>(i)]+20.f)/8.f;
    }
    engine.setSeed(static_cast<std::uint32_t>(get("seed")));
    const int scene=std::clamp(static_cast<int>(get("space")),0,14);
    if (scene!=lastScene) { engine.setScene(scene);lastScene=scene; }
    effortSmooth.setTargetValue(get("effort"));
    distanceSmooth.setTargetValue(get("distance"));
    mixSmooth.setTargetValue(get("mix"));
    bypassSmooth.setTargetValue(get("power")>.5f ? 0.f:1.f);
    // New JUCE transport API; fallback is a continuously running 120 BPM clock.
    double bpm=120.0, beats=walkBeat;
    bool hostPosition=false;
    if (auto* playhead=getPlayHead()) {
        if (const auto position=playhead->getPosition()) {
            if (auto v=position->getBpm()) bpm=std::clamp(*v,20.0,400.0);
            if (auto v=position->getPpqPosition()) {beats=*v;hostPosition=true;}
        }
    }
    const bool walkOn=get("walkOn")>.5f;
    const int walkPath=static_cast<int>(get("walkPath"));
    const float walkLength=std::max(1.f,get("walkLength"));
    const float walkScale=get("walkScale");
    const bool sync=get("walkSync")>.5f;
    const double samplesPerBeat=sampleRateHz*60.0/bpm;
    float peak=0.f;
    for (int i=0;i<n;++i) {
        s.effort=effortSmooth.getNextValue();
        s.distance=distanceSmooth.getNextValue();
        if (walkOn) {
            const double localBeats= sync ? beats+static_cast<double>(i)/samplesPerBeat
                                           : walkBeat+static_cast<double>(i)/samplesPerBeat;
            const float phase=static_cast<float>(std::fmod(std::max(0.0,localBeats),4.0*walkLength)/(4.0*walkLength));
            float path=phase;
            switch(walkPath){
                case 0:path=1.f-phase;break; // Approach
                case 1:path=phase;break;
                case 2:path=std::abs(2.f*phase-1.f);break;
                case 3:path=.5f+.5f*std::sin(2.f*afar::pi*phase*2.f);break;
                default:path=phase*phase;break;
            }
            s.distance=std::clamp(.3f+path*std::max(.2f,s.distance)*6.f*walkScale,.05f,200.f);
        }
        const afar::Frame dry {l[i],r ? r[i]:l[i]};
        const auto wet=engine.processFrame(dry,s,mixSmooth.getNextValue(),bypassSmooth.getNextValue());
        l[i]=wet.left;
        if (r) r[i]=wet.right;
        peak=std::max(peak,std::max(std::abs(l[i]),std::abs(r ? r[i]:l[i])));
    }
    if (!hostPosition) walkBeat+=static_cast<double>(n)/samplesPerBeat;
    else walkBeat=beats+static_cast<double>(n)/samplesPerBeat;
    // Simple peak ballistics: retain short peaks yet decay between blocks.
    const float previousPeak=meterPeak.load(std::memory_order_relaxed);
    const float decay=std::exp(-static_cast<float>(n)/(static_cast<float>(std::max(1.0,sampleRateHz))*.22f));
    meterPeak.store(std::max(peak,previousPeak*decay),std::memory_order_relaxed);
    meterClarity.store(engine.intelligibility(),std::memory_order_relaxed);
    meterDrr.store(engine.directReverb(),std::memory_order_relaxed);
    meterDistance.store(s.distance,std::memory_order_relaxed);
    meterEffort.store(s.effort,std::memory_order_relaxed);
}
juce::AudioProcessorEditor* SkellAfarAudioProcessor::createEditor() {return new SkellAfarAudioProcessorEditor(*this);}
void SkellAfarAudioProcessor::getStateInformation(juce::MemoryBlock& out) {
    auto state=parameters.copyState();
    state.addChild(editorState.createCopy(),-1,nullptr);
    if (auto xml=state.createXml()) copyXmlToBinary(*xml,out);
}
void SkellAfarAudioProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) {
        if (!xml->hasTagName(parameters.state.getType())) return;
        auto state=juce::ValueTree::fromXml(*xml);
        auto ui=state.getChildWithName("EditorState");
        if(ui.isValid()) {editorState=ui.createCopy();state.removeChild(ui,nullptr);}
        parameters.replaceState(state);
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new SkellAfarAudioProcessor();}
