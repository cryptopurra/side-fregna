#include "PluginEditor.h"

RONESidechainAudioProcessorEditor::RONESidechainAudioProcessorEditor(RONESidechainAudioProcessor& p):AudioProcessorEditor(&p),processor(p){
    setSize(900,540); setResizable(true,true); setResizeLimits(700,430,1500,900);
    title.setText("SIDE FREGNA",juce::dontSendNotification); title.setColour(juce::Label::textColourId,juce::Colour(0xffeef0ff)); title.setFont(juce::Font(22,juce::Font::bold)); addAndMakeVisible(title);
    hint.setText("THE DUCK YOU DRAW  ·  EVERY KICK RESTARTS THE CURVE",juce::dontSendNotification); hint.setColour(juce::Label::textColourId,juce::Colour(0xff8992b5)); addAndMakeVisible(hint);
    page.addItem("SIDECHAIN",1); page.addItem("PHASE",2); page.addItem("SPACE",3); page.setSelectedId(1); addAndMakeVisible(page);
    page.onChange=[this]{
        const int selected=page.getSelectedId();
        const bool side=selected==1, phasePage=selected==2, spacePage=selected==3;
        division.setVisible(side); shape.setVisible(side); mode.setVisible(side); depth.setVisible(side); click.setVisible(side); loadKick.setVisible(side); kickStatus.setVisible(side);
        phase.setVisible(phasePage); split.setVisible(phasePage); phaseEnabled.setVisible(phasePage);
        space.setVisible(spacePage); spaceAmount.setVisible(spacePage); release.setVisible(spacePage); spaceFrequency.setVisible(spacePage);
        hint.setText(side ? "THE DUCK YOU DRAW  ·  EVERY KICK RESTARTS THE CURVE" : phasePage ? "PHASE  ·  SUB-BAND HANDOVER" : "SPACE  ·  DYNAMIC ROOM FOR THE KICK",juce::dontSendNotification);
        repaint();
    };
    division.addItemList({"1/16","1/8","1/4","1/2","1 BAR"},1); addAndMakeVisible(division);
    shape.addItemList({"01 STEP","02 GATE","03 RAMP","04 PUMP","05 PLUCK","06 PULSE","07 SLOPE","08 SWELL","09 WAVE","10 RISE","11 NOTCH","12 BOUNCE","13 CUSTOM"},1); addAndMakeVisible(shape);
    mode.addItemList({"FULL","SUB","BAND"},1); addAndMakeVisible(mode);
    auto setup=[this](juce::Slider& s,const juce::String& name){s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,76,20);s.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff7888ff));s.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffe6e8f2));s.setName(name);addAndMakeVisible(s);};
    setup(depth,"DEPTH"); setup(click,"CLICK ms"); setup(phase,"PHASE °"); setup(split,"SPLIT Hz"); setup(spaceAmount,"AMOUNT"); setup(release,"RELEASE ms"); setup(spaceFrequency,"FREQ Hz"); addAndMakeVisible(space); addAndMakeVisible(phaseEnabled);
    loadKick.setButtonText("LOAD KICK…"); addAndMakeVisible(loadKick); kickStatus.setText("Drop a WAV/AIFF kick here",juce::dontSendNotification); kickStatus.setColour(juce::Label::textColourId,juce::Colour(0xff8992b5)); addAndMakeVisible(kickStatus);
    loadKick.onClick=[this]{
        kickChooser=std::make_unique<juce::FileChooser>("Load a kick sample");
        juce::Component::SafePointer<RONESidechainAudioProcessorEditor> safeThis(this);
        kickChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safeThis](const juce::FileChooser& chooser){
            auto* editor=safeThis.getComponent();
            if(editor==nullptr)return;
            if(editor->processor.loadKickSample(chooser.getResult())) editor->kickStatus.setText(editor->processor.getKickSampleName(),juce::dontSendNotification);
            else editor->kickStatus.setText("Could not read that audio file",juce::dontSendNotification);
        });
    };
    space.setButtonText("SPACE ON"); phaseEnabled.setButtonText("PHASE ON");
    auto& s=processor.state; divisionLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s,"division",division); modeLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s,"mode",mode); shapeLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s,"shape",shape); shape.onChange=[this]{ processor.loadShape(shape.getSelectedItemIndex()); repaint(); }; depthLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s,"depth",depth); clickLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s,"click",click); phaseLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s,"phase",phase); splitLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s,"split",split); amountLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s,"spaceAmount",spaceAmount); releaseLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s,"release",release); frequencyLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s,"spaceFrequency",spaceFrequency); spaceLink=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s,"space",space); phaseEnabledLink=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s,"phaseOn",phaseEnabled); page.onChange(); startTimerHz(30);
}
void RONESidechainAudioProcessorEditor::resized(){auto r=getLocalBounds().reduced(22);auto top=r.removeFromTop(44);title.setBounds(top.removeFromLeft(260));division.setBounds(top.removeFromRight(250).reduced(4));shape.setBounds(30,(int)(getHeight()*.42f)+110,getWidth()-60,28);page.setBounds(r.removeFromTop(42).removeFromLeft(300));hint.setBounds(340,52,520,30); r.removeFromTop(12); auto graph=r.removeFromTop((int)(getHeight()*.42f)); r.removeFromBottom(0); mode.setBounds(22,getHeight()-112,210,28); depth.setBounds(28,getHeight()-158,115,92); click.setBounds(143,getHeight()-158,115,92); phase.setBounds(250,getHeight()-158,110,92); split.setBounds(360,getHeight()-158,110,92); phaseEnabled.setBounds(490,getHeight()-130,100,30); space.setBounds(28,getHeight()-130,100,30); spaceAmount.setBounds(140,getHeight()-158,100,92); release.setBounds(250,getHeight()-158,100,92); spaceFrequency.setBounds(360,getHeight()-158,100,92); loadKick.setBounds(490,getHeight()-158,150,32); kickStatus.setBounds(490,getHeight()-122,getWidth()-510,24); juce::ignoreUnused(graph);}
void RONESidechainAudioProcessorEditor::mouseDown(const juce::MouseEvent& e){mouseDrag(e);}
void RONESidechainAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e){ const float left=34.0f, top=120.0f, w=(float)getWidth()-68.0f, h=(float)getHeight()*.42f-32.0f; if(e.x<left||e.x>left+w||e.y<top||e.y>top+h)return; const int i=juce::jlimit(0,32,(int)std::round((e.x-left)/w*32.0f)); processor.setCurvePoint(i,1.0f-(e.y-top)/h); shape.setSelectedId(13,juce::dontSendNotification); if(auto* parameter=processor.state.getParameter("shape")) parameter->setValueNotifyingHost(1.0f); repaint(); }
bool RONESidechainAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files){for(const auto& path:files){const auto ext=juce::File(path).getFileExtension().toLowerCase();if(ext==".wav"||ext==".aif"||ext==".aiff")return true;}return false;}
void RONESidechainAudioProcessorEditor::filesDropped(const juce::StringArray& files,int,int){if(files.isEmpty())return;if(processor.loadKickSample(juce::File(files[0])))kickStatus.setText(processor.getKickSampleName(),juce::dontSendNotification);else kickStatus.setText("Could not read that audio file",juce::dontSendNotification);}
void RONESidechainAudioProcessorEditor::paint(juce::Graphics& g){
    g.fillAll(juce::Colour(0xff111318)); g.setColour(juce::Colour(0xff7888ff)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),10,1.5f);
    auto area=juce::Rectangle<float>(22,104,(float)getWidth()-44,(float)getHeight()*.42f);g.setColour(juce::Colour(0xff1b1e25));g.fillRoundedRectangle(area,10);
    for(int i=0;i<=8;++i){float x=area.getX()+area.getWidth()*i/8;g.setColour(juce::Colour(0xff2b2f3b));g.drawVerticalLine((int)x,area.getY()+12,area.getBottom()-12);}
    for(int i=0;i<=4;++i){float y=area.getY()+area.getHeight()*i/4;g.setColour(juce::Colour(0xff272b35));g.drawHorizontalLine((int)y,area.getX()+12,area.getRight()-12);}
    g.setColour(juce::Colour(0xff464b59));g.drawLine(area.getX()+12,area.getCentreY()+35,area.getRight()-12,area.getCentreY()+35,2);
    if(page.getSelectedId()==1) {
        juce::Path p; auto yAt=[&](float x){const float index=x*32.0f;const int i=juce::jmin(31,(int)index);const float value=juce::jmap(index-(float)i,processor.getCurvePoint(i),processor.getCurvePoint(i+1));return area.getBottom()-16.0f-value*(area.getHeight()-32.0f);};
        p.startNewSubPath(area.getX()+12,yAt(0));for(int i=1;i<=128;++i){float x=(float)i/128;p.lineTo(area.getX()+12+x*(area.getWidth()-24),yAt(x));}
        g.setColour(juce::Colour(0x407888ff));g.strokePath(p,juce::PathStrokeType(8));g.setColour(juce::Colour(0xff8794ff));g.strokePath(p,juce::PathStrokeType(2.2f));
        g.setColour(juce::Colour(0xffdce0ff));g.setFont(12);g.drawText("BEAT",(int)area.getX()+12,(int)area.getY()+8,50,16,juce::Justification::left);g.setColour(juce::Colour(0xff7888ff));g.drawText("BASS  ·  CURVE",(int)area.getX()+60,(int)area.getY()+8,180,16,juce::Justification::left);
    } else {
        g.setColour(juce::Colour(0xff8992b5)); g.setFont(16);
        g.drawText(page.getSelectedId()==2 ? "PHASE ALIGNMENT VIEW  ·  DSP IN DEVELOPMENT" : "SPACE SPECTRUM VIEW  ·  DYNAMIC BELLS IN DEVELOPMENT",area.toNearestInt().reduced(24),juce::Justification::centred);
    }
    g.setColour(juce::Colour(0xff262a34));g.fillRoundedRectangle(22,(float)getHeight()-168,(float)getWidth()-44,146,10);g.setColour(juce::Colour(0xff8992b5));g.setFont(12);g.drawText("SILENT AT THE KICK’S FIRST SAMPLE  ·  CLICK FADE COMPLETES AT THE HIT",40,getHeight()-164,getWidth()-80,20,juce::Justification::centredLeft);
}
