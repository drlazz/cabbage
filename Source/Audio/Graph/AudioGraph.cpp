/*
  Copyright (C) 2016 Rory Walsh

  Cabbage is free software; you can redistribute it
  and/or modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Cabbage is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/

#include "AudioGraph.h"
#include "../../Application/CabbageMainComponent.h"

//==============================================================================
class PluginWindow;


const int AudioGraph::midiChannelNumber = 0x1000;

AudioGraph::AudioGraph (CabbageMainComponent& owner_, PropertySet* settingsToUse,
                        bool takeOwnershipOfSettings,
                        const String& preferredDefaultDeviceName,
                        const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)

    :   settings (settingsToUse, takeOwnershipOfSettings),
        graph(),
        owner (owner_),
        lookAndFeel(),
        FileBasedDocument (filenameSuffix,
                           filenameWildcard,
                           "Load a filter graph",
                           "Save a filter graph"),
        formatManager (new AudioPluginFormatManager())
{
    newDocument();
    graph.prepareToPlay (44100, 512);
    graph.setPlayConfigDetails (2, 2, 44100, 512);
    AudioProcessor::setTypeOfNextNewPlugin (AudioProcessor::wrapperType_Standalone);
    createInternalFilters();
    setupAudioDevices (preferredDefaultDeviceName, preferredSetupOptions);
    startPlaying();

}

AudioGraph::~AudioGraph()
{
    deletePlugins();
    shutDownAudioDevices();
    xmlSettings = nullptr;
}

void AudioGraph::createInternalFilters()
{
    AudioProcessorGraph::Node* midiInputNode = createNode (getPluginDescriptor ("Internal", "Midi Input", internalNodeIds[InternalNodes::MIDIInput]), internalNodeIds[InternalNodes::MIDIInput]);
    internalNodeIds.add (midiInputNode->nodeID);
    setNodePosition (internalNodeIds[InternalNodes::MIDIInput], Point<double>(.4, .1));


    AudioProcessorGraph::Node* inputNode =  createNode (getPluginDescriptor ("Internal", "Audio Input", internalNodeIds[InternalNodes::MIDIInput]), internalNodeIds[InternalNodes::AudioInput]);
    internalNodeIds.add (inputNode->nodeID);
    setNodePosition (internalNodeIds[InternalNodes::AudioInput], Point<double>(.6, .1));

    AudioProcessorGraph::Node* outputNode =  createNode (getPluginDescriptor ("Internal", "Audio Output", internalNodeIds[InternalNodes::MIDIInput]), internalNodeIds[InternalNodes::AudioOutput]);
    internalNodeIds.add (outputNode->nodeID);
    setNodePosition (internalNodeIds[InternalNodes::AudioOutput], Point<double>(.5, .8));
}

//==============================================================================
bool AudioGraph::addPlugin (File inputFile, AudioProcessorGraph::NodeID nodeId)
{
    for ( int i = 0 ; i < graph.getNumNodes() ; i++)
    {
        if (graph.getNode (i)->nodeID == nodeId)
        {
            Point<double> position = this->getNodePosition (nodeId);
            ScopedPointer<XmlElement> xml = createConnectionsXml();
            //delete graph.getNodeForId(nodeId)->getProcessor();
            graph.removeNode (nodeId);
            AudioProcessorGraph::Node::Ptr node = createNode (getPluginDescriptor ("Cabbage", "Cabbage", nodeId, inputFile.getFullPathName()), nodeId);
            //if(graph.getNodeForId(nodeId)->getProcessor()->isSuspended())
            //    return false;
            setNodePosition (nodeId, Point<double>(position.getX(), position.getY()));
            restoreConnectionsFromXml (*xml);
            xml = nullptr;
            pluginFiles.add(inputFile.getFullPathName());
            return true;
        }
    }

    setChangedFlag (true);
    const AudioProcessorGraph::Node::Ptr node = createNode (getPluginDescriptor ("Cabbage", "Cabbage", nodeId, inputFile.getFullPathName()), nodeId);
    if(graph.getNodeForId(nodeId)->getProcessor()->isSuspended())
        return false;

    pluginFiles.add(inputFile.getFullPathName());
    setDefaultConnections (nodeId);
    return true;
}
//==============================================================================
const PluginDescription AudioGraph::getPluginDescriptor (String type, String name, AudioProcessorGraph::NodeID nodeId, String inputFile )
{
    PluginDescription descript;
    descript.fileOrIdentifier = inputFile;
    descript.descriptiveName = type + ":" + inputFile;
    descript.name = name;
    descript.numInputChannels = 2;
    descript.numOutputChannels = 2;
    descript.isInstrument = true;
    descript.uid = nodeId.uid;

    descript.manufacturerName = "CabbageAudio";
    descript.pluginFormatName = type;

    return descript;
}
//==============================================================================
void AudioGraph::showCodeEditorForNode (AudioProcessorGraph::NodeID nodeId)
{
    bool foundTabForNode = false;
    for ( int i = 0 ; i < owner.getNumberOfFileTabs() ; i++)
    {
        if (int32 (owner.getFileTab(i)->uniqueFileId == nodeId))
        {
            foundTabForNode = true;
            owner.bringCodeEditorToFront (owner.getFileTabForNodeId(nodeId.uid));
        }
    }

    if(foundTabForNode == false)
    {
        AudioProcessorGraph::Node::Ptr n = graph.getNodeForId(nodeId);
        const String pluginFilename = n->properties.getWithDefault("pluginFile", "").toString();
        owner.openFile(pluginFilename);
        owner.getFileTab(owner.getCurrentFileIndex())->uniqueFileId = nodeId;
    }

}
//==============================================================================
AudioProcessorGraph::Node::Ptr AudioGraph::createNode (const PluginDescription& desc, AudioProcessorGraph::NodeID nodeId)
{
    if (desc.pluginFormatName == "Cabbage")
    {
        AudioProcessor* processor;
        bool isCabbageFile = CabbageUtilities::hasCabbageTags (File (desc.fileOrIdentifier));

        if (isCabbageFile)
            processor = createCabbagePluginFilter (File (desc.fileOrIdentifier));
        else
            processor = createGenericPluginFilter (File (desc.fileOrIdentifier));

        AudioProcessor::setTypeOfNextNewPlugin (AudioProcessor::wrapperType_Undefined);
        jassert (processor != nullptr);
        processor->enableAllBuses();
        //const int inputs = processor->getBusCount (true);
        //const int outputs = processor->getBusCount (false);

        //const int numberOfChannels = static_cast<CsoundPluginProcessor*> (processor)->getNumberOfCsoundChannels();

        processor->disableNonMainBuses();
        processor->setRateAndBufferSizeDetails (44100, 512);

        //AudioProcessor::Bus* ins = processor->getBus (true, 0);
        //ins->setCurrentLayout (AudioChannelSet::discreteChannels (numberOfChannels));

        //AudioProcessor::Bus* outs = processor->getBus (false, 0);
        //outs->setCurrentLayout (AudioChannelSet::discreteChannels (numberOfChannels));

        AudioProcessorGraph::Node* node = graph.addNode (processor, nodeId);
        ScopedPointer<XmlElement> xmlElem;
        xmlElem = desc.createXml();
        node->properties.set ("pluginFile", desc.fileOrIdentifier);
        node->properties.set ("pluginType", isCabbageFile == true ? "Cabbage" : "Csound");
        node->properties.set ("pluginName", "Test");
        node->properties.set ("pluginDesc", xmlElem->createDocument (""));

        return node;

    }
    else if (desc.pluginFormatName == "Internal")
    {
        ScopedPointer<XmlElement> xmlElem;
        xmlElem = desc.createXml();

        if (desc.name == "Midi Input")
        {
            AudioProcessorGraph::AudioGraphIOProcessor* midiNode;
            midiNode = new AudioProcessorGraph::AudioGraphIOProcessor (AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
            AudioProcessorGraph::Node* node = graph.addNode (midiNode, nodeId);
            node->properties.set ("pluginDesc", xmlElem->createDocument (""));
            return node;
        }
        else if (desc.name == "Audio Input")
        {
            AudioProcessorGraph::AudioGraphIOProcessor* inNode;
            inNode = new AudioProcessorGraph::AudioGraphIOProcessor (AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
            AudioProcessorGraph::Node* node = graph.addNode (inNode, nodeId);
            node->properties.set ("pluginDesc", xmlElem->createDocument (""));
            return node;
        }
        else if (desc.name == "Audio Output")
        {
            AudioProcessorGraph::AudioGraphIOProcessor* outNode;
            outNode = new AudioProcessorGraph::AudioGraphIOProcessor (AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
            //outNode->properties.set("pluginDesc", xmlElem->createDocument(""));
            AudioProcessorGraph::Node* node = graph.addNode (outNode, nodeId);
            node->properties.set ("pluginDesc", xmlElem->createDocument (""));
            return node;
        }
    }

    return nullptr;

}
//==============================================================================
void AudioGraph::setDefaultConnections (AudioProcessorGraph::NodeID nodeId)
{
    setNodePosition (nodeId, Point<double>(4 + (graph.getNumNodes() - 3)*.05, .5));
    AudioProcessorGraph::NodeAndChannel inputL = {(AudioProcessorGraph::NodeID) internalNodeIds[InternalNodes::AudioInput], 0};
    AudioProcessorGraph::NodeAndChannel inputR = {(AudioProcessorGraph::NodeID) internalNodeIds[InternalNodes::AudioInput], 1};
    
    AudioProcessorGraph::NodeAndChannel nodeL = {(AudioProcessorGraph::NodeID) nodeId, 0};
    AudioProcessorGraph::NodeAndChannel nodeR = {(AudioProcessorGraph::NodeID) nodeId, 1};
 
    AudioProcessorGraph::NodeAndChannel outputL = {(AudioProcessorGraph::NodeID) internalNodeIds[InternalNodes::AudioOutput], 0};
    AudioProcessorGraph::NodeAndChannel outputR = {(AudioProcessorGraph::NodeID) internalNodeIds[InternalNodes::AudioOutput], 1};
    bool connectInput1 = graph.addConnection ({inputL, nodeL});
    bool connectInput2 = graph.addConnection ({inputR, nodeR});

    bool connection1 = graph.addConnection ({nodeL, outputL});
    bool connection2 = graph.addConnection ({nodeR, outputR});
    //if(connection2 == false && connection1 == true)
    //  graph.addConnection (nodeId, 0, internalNodeIds[InternalNodes::AudioOutput], 1);

    AudioProcessorGraph::NodeAndChannel midiIn = {(AudioProcessorGraph::NodeID) internalNodeIds[InternalNodes::MIDIInput], AudioProcessorGraph::midiChannelIndex};
    AudioProcessorGraph::NodeAndChannel midiOut = {(AudioProcessorGraph::NodeID) nodeId, AudioProcessorGraph::midiChannelIndex};
    
    bool connection3 = graph.addConnection ({midiIn, midiOut});

    //if (connection1 == false  || connectInput1 == false )
    //    jassertfalse;
}


void AudioGraph::updateBusLayout (AudioProcessor* selectedProcessor)
{
    //    if (AudioProcessor* filter = selectedProcessor)
    //    {
    //        if (AudioProcessor::Bus* bus = filter->getBus (isInput, currentBus))
    //        {
    //            bool test = bus->setNumberOfChannels(8);
    //        }
    //    }
}

//==============================================================================
void AudioGraph::deletePlugins()
{
    closeAnyOpenPluginWindows();
    stopPlaying();
    graph.clear();
}

String AudioGraph::getFilePatterns (const String& fileSuffix)
{
    if (fileSuffix.isEmpty())
        return String();

    return (fileSuffix.startsWithChar ('.') ? "*" : "*.") + fileSuffix;
}

void AudioGraph::setXmlAudioSettings (XmlElement* xmlSettingsString)
{
    xmlSettings = xmlSettingsString;
    setupAudioDevices ( String(), nullptr);
    startPlaying();
}

AudioDeviceSelectorComponent* AudioGraph::getAudioDeviceSelector()
{
    const int numInputs = 2;//processor != nullptr ? processor->getTotalNumInputChannels() : 2;
    const int numOutputs = 2;//processor != nullptr ? processor->getTotalNumOutputChannels() : 2;

    return new AudioDeviceSelectorComponent (deviceManager,
                                             numInputs,
                                             numInputs,
                                             numOutputs,
                                             numOutputs,
                                             true, false,
                                             true, false);
}

String AudioGraph::getDeviceManagerSettings()
{
    if (deviceManager.getCurrentAudioDevice())
    {
        ScopedPointer<XmlElement> xml (deviceManager.createStateXml());

        if (xml == nullptr)
            return String::empty;
        else
            return xml->createDocument ("");
    }
    else return String::empty;
}

//==============================================================================
void AudioGraph::startPlaying()
{
    player.setProcessor (&graph);
}

void AudioGraph::stopPlaying()
{
    player.setProcessor (nullptr);
}

void AudioGraph::reloadAudioDeviceState (const String& preferredDefaultDeviceName,
                                         const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
{
    ScopedPointer<XmlElement> savedState;

    if (settings != nullptr)
        savedState = settings->getXmlValue ("audioSetup");

    deviceManager.initialise (256,
                              256,
                              savedState,
                              true,
                              preferredDefaultDeviceName,
                              preferredSetupOptions);
}

//==============================================================================
String AudioGraph::getCsoundOutput (AudioProcessorGraph::NodeID nodeId)
{
    if (graph.getNodeForId (nodeId) != nullptr &&
        graph.getNodeForId (nodeId)->getProcessor() != nullptr)
    {
        if (graph.getNodeForId (nodeId)->properties.getWithDefault ("pluginType", "").toString() == "Cabbage")
            return dynamic_cast<CabbagePluginProcessor*> (graph.getNodeForId (nodeId)->getProcessor())->getCsoundOutput();
        else
            return dynamic_cast<GenericCabbagePluginProcessor*> (graph.getNodeForId (nodeId)->getProcessor())->getCsoundOutput();
    }

    return String::empty;
}

//==============================================================================
const AudioProcessorGraph::Node::Ptr AudioGraph::getNode (const int index) const noexcept
{
    return graph.getNode (index);
}

const AudioProcessorGraph::Node::Ptr AudioGraph::getNodeForId (const AudioProcessorGraph::NodeID nodeId) const noexcept
{
    return graph.getNodeForId (nodeId);
}

//==============================================================================
int AudioGraph::getNumPlugins() const noexcept
{
    return graph.getNumNodes();
}

void AudioGraph::setNodePosition (AudioProcessorGraph::NodeID nodeID, Point<double> pos)
{
    if (auto* n = graph.getNodeForId (nodeID))
    {
    n->properties.set ("x", jlimit (0.0, 1.0, pos.x));
    n->properties.set ("y", jlimit (0.0, 1.0, pos.y));
    }
}

Point<double> AudioGraph::getNodePosition (const AudioProcessorGraph::NodeID nodeID) const
{
    if (auto* n = graph.getNodeForId (nodeID))
        return { static_cast<double> (n->properties ["x"]),

    static_cast<double> (n->properties ["y"]) };

    return {};
}
//==============================================================================
int AudioGraph::getNumConnections() const noexcept
{
    return (int)graph.getConnections().size();
}

const AudioProcessorGraph::Connection* AudioGraph::getConnection (const int index) const noexcept
{
    return &graph.getConnections()[index];
}

const AudioProcessorGraph::Connection* AudioGraph::getConnectionBetween (uint32 sourceFilterUID, int sourceFilterChannel,
                                                                         uint32 destFilterUID, int destFilterChannel) const noexcept
{
    std::vector<AudioProcessorGraph::Connection> currentConnections = graph.getConnections();
    
    for( auto& connection : currentConnections)
    {
        if( connection.source.nodeID == AudioProcessorGraph::NodeID(sourceFilterUID) &&
            connection.source.channelIndex == sourceFilterChannel &&
            connection.destination.nodeID ==  AudioProcessorGraph::NodeID(destFilterUID) &&
            connection.destination.channelIndex == destFilterChannel)
        
            return &connection;
    }
    
    return nullptr;
    
}

bool AudioGraph::canConnect (uint32 sourceFilterUID, int sourceFilterChannel,
                             uint32 destFilterUID, int destFilterChannel) const noexcept
{
    
    AudioProcessorGraph::NodeAndChannel source = {(AudioProcessorGraph::NodeID) sourceFilterUID, sourceFilterChannel};
    AudioProcessorGraph::NodeAndChannel dest = {(AudioProcessorGraph::NodeID) destFilterUID, destFilterChannel};
    
    return graph.canConnect ({source, dest});
}

bool AudioGraph::addConnection (uint32 sourceFilterUID, int sourceFilterChannel,
                                uint32 destFilterUID, int destFilterChannel)
{
    AudioProcessorGraph::NodeAndChannel source = {(AudioProcessorGraph::NodeID) sourceFilterUID, sourceFilterChannel};
    AudioProcessorGraph::NodeAndChannel dest = {(AudioProcessorGraph::NodeID) destFilterUID, destFilterChannel};
    
    const bool result = graph.addConnection ({source, dest});

    if (result)
        changed();

    return result;
}

void AudioGraph::removeFilter (const AudioProcessorGraph::NodeID id)
{
    jassertfalse;
    //closeCurrentlyOpenWindowsFor (id);
//    AudioProcessorGraph::Node::Ptr n = graph.getNodeForId(id);
//
//    const String pluginFilename = n->properties.getWithDefault("pluginFile", "").toString();
//
//    if (graph.removeNode (id))
//    {
////        for ( int i = 0 ; i < owner.getNumberOfFileTabs() ; i++)
////        {
////            if (int32 (owner.getFileTab(i)->uniqueFileId == id))
////            {
////                remove file tab from array;
////                break;
////            }
////        }
//
//        changed();
//
//    }
}

void AudioGraph::disconnectFilter (const AudioProcessorGraph::NodeID id)
{
    if (graph.disconnectNode (id))
        changed();
}

void AudioGraph::removeConnection (const int index)
{
    graph.removeConnection( graph.getConnections()[index]);
    changed();
}

void AudioGraph::removeConnection (uint32 sourceFilterUID, int sourceFilterChannel,
                                   uint32 destFilterUID, int destFilterChannel)
{
    AudioProcessorGraph::NodeAndChannel source = {(AudioProcessorGraph::NodeID) sourceFilterUID, sourceFilterChannel};
    AudioProcessorGraph::NodeAndChannel dest = {(AudioProcessorGraph::NodeID) destFilterUID, destFilterChannel};
    
    if (graph.removeConnection ({source, dest}))
        changed();
}

void AudioGraph::clear()
{
    closeAnyOpenPluginWindows();
    graph.clear();
    changed();
}

Point<int> AudioGraph::getPositionOfCurrentlyOpenWindow (const AudioProcessorGraph::NodeID nodeId)
{
    if(nodeId.uid>3)
    for (auto* w : activePluginWindows)
        if (w->node == graph.getNodeForId(nodeId))
            return w->getPosition();

    return Point<int> (-1000, -1000);
}

PluginWindow* AudioGraph::getOrCreateWindowFor (AudioProcessorGraph::Node* node, PluginWindow::Type type)
{
    jassert (node != nullptr);

#if JUCE_IOS || JUCE_ANDROID
    closeAnyOpenPluginWindows();
#else
    for (auto* w : activePluginWindows)
        if (w->node == node && w->type == type)
            return w;
#endif

    if (auto* processor = node->getProcessor())
    {
        if (auto* plugin = dynamic_cast<AudioPluginInstance*> (processor))
        {
            auto description = plugin->getPluginDescription();

            if (description.pluginFormatName == "Internal")
            {
                //getCommandManager().invokeDirectly (CommandIDs::showAudioSettings, false);
                return nullptr;
            }
        }

        return activePluginWindows.add (new PluginWindow (node, type, activePluginWindows));
    }

    return nullptr;
}

bool AudioGraph::closeAnyOpenPluginWindows()
{
    bool wasEmpty = activePluginWindows.isEmpty();
    activePluginWindows.clear();
    return ! wasEmpty;
}


bool AudioGraph::closeCurrentlyOpenWindowsFor(const AudioProcessorGraph::NodeID nodeId)
{
    for (int i = activePluginWindows.size(); --i >= 0;)
        if (activePluginWindows.getUnchecked (i)->node == graph.getNodeForId(nodeId))
        {
            AudioProcessorGraph::Node::Ptr f = graph.getNodeForId (nodeId);
            f->getProcessor()->editorBeingDeleted(f->getProcessor()->getActiveEditor());
            activePluginWindows.remove(i);
            return true; 
        }
    return false;
}

//==============================================================================
static void readBusLayoutFromXml (AudioProcessor::BusesLayout& busesLayout, AudioProcessor* plugin, const XmlElement& xml, const bool isInput)
{
    Array<AudioChannelSet>& targetBuses = (isInput ? busesLayout.inputBuses : busesLayout.outputBuses);
    int maxNumBuses = 0;

    if (auto* buses = xml.getChildByName (isInput ? "INPUTS" : "OUTPUTS"))
    {
        forEachXmlChildElementWithTagName (*buses, e, "BUS")
        {
            const int busIdx = e->getIntAttribute ("index");
            maxNumBuses = jmax (maxNumBuses, busIdx + 1);

            // the number of buses on busesLayout may not be in sync with the plugin after adding buses
            // because adding an input bus could also add an output bus
            for (int actualIdx = plugin->getBusCount (isInput) - 1; actualIdx < busIdx; ++actualIdx)
                if (! plugin->addBus (isInput)) return;

            for (int actualIdx = targetBuses.size() - 1; actualIdx < busIdx; ++actualIdx)
                targetBuses.add (plugin->getChannelLayoutOfBus (isInput, busIdx));

            const String& layout = e->getStringAttribute ("layout");

            if (layout.isNotEmpty())
                targetBuses.getReference (busIdx) = AudioChannelSet::fromAbbreviatedString (layout);
        }
    }

    // if the plugin has more buses than specified in the xml, then try to remove them!
    while (maxNumBuses < targetBuses.size())
    {
        if (! plugin->removeBus (isInput))
            return;

        targetBuses.removeLast();
    }
}

static XmlElement* createBusLayoutXml (const AudioProcessor::BusesLayout& layout, const bool isInput)
{
    const Array<AudioChannelSet>& buses = (isInput ? layout.inputBuses : layout.outputBuses);

    XmlElement* xml = new XmlElement (isInput ? "INPUTS" : "OUTPUTS");

    const int n = buses.size();

    for (int busIdx = 0; busIdx < n; ++busIdx)
    {
        XmlElement* bus = new XmlElement ("BUS");
        bus->setAttribute ("index", busIdx);

        const AudioChannelSet& set = buses.getReference (busIdx);
        const String layoutName = set.isDisabled() ? "disabled" : set.getSpeakerArrangementAsString();

        bus->setAttribute ("layout", layoutName);

        xml->addChildElement (bus);
    }

    return xml;
}

static XmlElement* createNodeXml (AudioProcessorGraph::Node* const node) noexcept
{
    PluginDescription pd;

    if (AudioPluginInstance* plugin = dynamic_cast <AudioPluginInstance*> (node->getProcessor()))
        plugin->fillInPluginDescription (pd);
    else if (dynamic_cast <CabbagePluginProcessor*> (node->getProcessor()) ||
             dynamic_cast <CsoundPluginProcessor*> (node->getProcessor()))
    {
        //grab description of native plugin for saving...
        String xmlPluginDescriptor = node->properties.getWithDefault ("pluginDesc", "").toString();
        //cUtils::debug(xmlPluginDescriptor);
        XmlElement* xmlElem;
        xmlElem = XmlDocument::parse (xmlPluginDescriptor);
        pd.loadFromXml (*xmlElem);
    }


    XmlElement* e = new XmlElement ("FILTER");
    e->setAttribute ("uid", (int32) node->nodeID.uid);
    e->setAttribute ("x", node->properties ["x"].toString());
    e->setAttribute ("y", node->properties ["y"].toString());
    e->setAttribute ("uiLastX", node->properties ["uiLastX"].toString());
    e->setAttribute ("uiLastY", node->properties ["uiLastY"].toString());
    e->addChildElement (pd.createXml());

    XmlElement* state = new XmlElement ("STATE");

    MemoryBlock m;
    node->getProcessor()->getStateInformation (m);
    state->addTextElement (m.toBase64Encoding());
    e->addChildElement (state);

    return e;
}

void AudioGraph::restoreFromXml (const XmlElement& xml)
{
    clear();

    forEachXmlChildElementWithTagName (xml, e, "FILTER")
    {
        createNodeFromXml (*e);
        changed();
    }

    forEachXmlChildElementWithTagName (xml, e, "CONNECTION")
    {
        addConnection ((uint32) e->getIntAttribute ("srcFilter"),
                       e->getIntAttribute ("srcChannel"),
                       (uint32) e->getIntAttribute ("dstFilter"),
                       e->getIntAttribute ("dstChannel"));
    }

    graph.removeIllegalConnections();
}

void AudioGraph::restoreConnectionsFromXml (const XmlElement& xml)
{
    forEachXmlChildElementWithTagName (xml, e, "CONNECTION")
    {
        addConnection ((uint32) e->getIntAttribute ("srcFilter"),
                       e->getIntAttribute ("srcChannel"),
                       (uint32) e->getIntAttribute ("dstFilter"),
                       e->getIntAttribute ("dstChannel"));
    }

    graph.removeIllegalConnections();
}

void AudioGraph::createNodeFromXml (const XmlElement& xml)
{

    ScopedPointer<XmlElement> xml2 = XmlDocument::parse (File ("/home/rory/Desktop/NodeTest.xml"));
    PluginDescription desc;

    forEachXmlChildElement (xml, e)
    {
        if (desc.loadFromXml (*e))
            break;
    }

    //xml.writeToFile (File ("/home/rory/Desktop/NodeTest.xml"), "");
    AudioProcessorGraph::Node::Ptr node = createNode (desc, AudioProcessorGraph::NodeID(xml.getIntAttribute ("uid")));

    if (node == nullptr)
        jassertfalse;

    if (const XmlElement* const state = xml.getChildByName ("STATE"))
    {
        MemoryBlock m;
        m.fromBase64Encoding (state->getAllSubText());

        node->getProcessor()->setStateInformation (m.getData(), (int) m.getSize());
    }

    node->properties.set ("x", xml.getDoubleAttribute ("x"));
    node->properties.set ("y", xml.getDoubleAttribute ("y"));
    node->properties.set ("uiLastX", xml.getIntAttribute ("uiLastX"));
    node->properties.set ("uiLastY", xml.getIntAttribute ("uiLastY"));
    node->properties.set ("pluginName", desc.name);

    setNodePosition (node->nodeID, Point<double>(xml.getDoubleAttribute ("x"), xml.getDoubleAttribute ("y")));
}

XmlElement* AudioGraph::createXml() const
{
    XmlElement* xml = new XmlElement ("FILTERGRAPH");

    for (int i = 0; i < graph.getNumNodes(); ++i)
        xml->addChildElement (createNodeXml (graph.getNode (i)));

    for (int i = 0; i < graph.getConnections().size(); ++i)
    {
        const AudioProcessorGraph::Connection* const fc = &graph.getConnections()[i];

        XmlElement* e = new XmlElement ("CONNECTION");

        e->setAttribute ("srcFilter", (int) fc->source.nodeID.uid);
        e->setAttribute ("srcChannel", fc->source.channelIndex);
        e->setAttribute ("dstFilter", (int) fc->destination.nodeID.uid);
        e->setAttribute ("dstChannel", fc->destination.channelIndex);

        xml->addChildElement (e);
    }

    return xml;
}

XmlElement* AudioGraph::createConnectionsXml() const
{
//    XmlElement* xml = new XmlElement ("FILTERGRAPH");
//
//    for (int i = 0; i < graph.getConnections().size(); ++i)
//    {
//        const AudioProcessorGraph::Connection* const fc = &graph.getConnections()[i];
//
//        XmlElement* e = new XmlElement ("CONNECTION");
//
//        e->setAttribute ("srcFilter", (int) fc->source.nodeID);
//        e->setAttribute ("srcChannel", fc->source.channelIndex);
//        e->setAttribute ("dstFilter", (int) fc->destination.nodeID);
//        e->setAttribute ("dstChannel", fc->destination.channelIndex);
//
//        xml->addChildElement (e);
//    }
//
//    return xml;
    
    auto* xml = new XmlElement ("FILTERGRAPH");
    
    for (auto* node : graph.getNodes())
        xml->addChildElement (createNodeXml (node));
    
    for (auto& connection : graph.getConnections())
    {
        auto e = xml->createNewChildElement ("CONNECTION");
        
        e->setAttribute ("srcFilter", (int) connection.source.nodeID.uid);
        e->setAttribute ("srcChannel", connection.source.channelIndex);
        e->setAttribute ("dstFilter", (int) connection.destination.nodeID.uid);
        e->setAttribute ("dstChannel", connection.destination.channelIndex);
    }
    
    return xml;
}
//========================================================================================
String AudioGraph::getDocumentTitle()
{
    if (! getFile().exists())
        return "Unnamed";

    return getFile().getFileNameWithoutExtension();
}

void AudioGraph::newDocument()
{
    clear();
    setFile (File());
    setChangedFlag (false);
}

Result AudioGraph::loadDocument (const File& file)
{
    XmlDocument doc (file);
    ScopedPointer<XmlElement> xml (doc.getDocumentElement());

    if (xml == nullptr || ! xml->hasTagName ("FILTERGRAPH"))
        return Result::fail ("Not a valid filter graph file");

    restoreFromXml (*xml);
    return Result::ok();
}

FileBasedDocument::SaveResult AudioGraph::saveGraph (bool saveAs)
{
    FileChooser fc ("Save file as", File::getSpecialLocation (File::SpecialLocationType::userHomeDirectory), "", CabbageUtilities::shouldUseNativeBrowser());

    if (getFile().existsAsFile() && saveAs == false)
    {
        saveDocument (getFile().withFileExtension (".cabbage"));
    }
    else
    {
        if (fc.browseForFileToSave (false))
        {
            if (fc.getResult().existsAsFile())
            {

                const int result = CabbageUtilities::showYesNoMessage ("Do you wish to overwrite\nexiting file?", &lookAndFeel);

                if (result == 1)
                {
                    saveDocument (fc.getResult().withFileExtension (".cabbage"));
                    return FileBasedDocument::SaveResult::savedOk;
                }
                else
                    return FileBasedDocument::SaveResult::userCancelledSave;
            }
            else
            {
                saveDocument (fc.getResult().withFileExtension (".cabbage"));
                return FileBasedDocument::SaveResult::savedOk;
            }

        }
    }

    return FileBasedDocument::SaveResult::savedOk;
}

Result AudioGraph::saveDocument (const File& file)
{
    ScopedPointer<XmlElement> xml (createXml());

    if (! xml->writeToFile (file, String()))
        return Result::fail ("Couldn't write to the file");

    return Result::ok();
}

File AudioGraph::getLastDocumentOpened()
{
    //    RecentlyOpenedFilesList recentFiles;
    //    recentFiles.restoreFromString (getAppProperties().getUserSettings()
    //                                        ->getValue ("recentFilterGraphFiles"));
    //
    //    return recentFiles.getFile (0);
    return File();
}

void AudioGraph::setLastDocumentOpened (const File& file)
{
    //    RecentlyOpenedFilesList recentFiles;
    //    recentFiles.restoreFromString (getAppProperties().getUserSettings()
    //                                        ->getValue ("recentFilterGraphFiles"));
    //
    //    recentFiles.addFile (file);
    //
    //    getAppProperties().getUserSettings()
    //        ->setValue ("recentFilterGraphFiles", recentFiles.toString());
}
//========================================================================================

//static Array <PluginWindow*> activePluginWindows;
//
//PluginWindow::PluginWindow (Component* const pluginEditor,
//                            AudioProcessorGraph::Node* const o,
//                            WindowFormatType t,
//                            AudioProcessorGraph& audioGraph)
//    : DocumentWindow (pluginEditor->getName(), Colours::black,
//                      DocumentWindow::minimiseButton | DocumentWindow::closeButton),
//      graph (audioGraph),
//      owner (o),
//      type (t)
//{
//    setSize (400, 300);
//    setContentOwned (pluginEditor, true);
//    setVisible (true);
//
//    activePluginWindows.add (this);
//}
//
//
//
////==============================================================================
//
//PluginWindow* PluginWindow::getWindowFor (AudioProcessorGraph::Node* const node,
//                                          WindowFormatType type,
//                                          AudioProcessorGraph& audioGraph)
//{
//    jassert (node != nullptr);
//
//    for (int i = activePluginWindows.size(); --i >= 0;)
//        if (activePluginWindows.getUnchecked (i)->owner == node
//            && activePluginWindows.getUnchecked (i)->type == type)
//            return activePluginWindows.getUnchecked (i);
//
//    AudioProcessor* processor = node->getProcessor();
//    AudioProcessorEditor* ui = nullptr;
//
//    if (type == Normal)
//    {
//        ui = processor->createEditorIfNeeded();
//
//        if (ui == nullptr)
//            type = Generic;
//    }
//
//    if (ui == nullptr)
//    {
//        if (type == Generic || type == Parameters)
//            ui = new GenericAudioProcessorEditor (processor);
//
//        //        else if (type == Programs)
//        //            ui = new ProgramAudioProcessorEditor (processor);
//        //        else if (type == AudioIO)
//        //            ui = new FilterIOConfigurationWindow (processor);
//    }
//
//    if (ui != nullptr)
//    {
//        if (AudioPluginInstance* const plugin = dynamic_cast<AudioPluginInstance*> (processor))
//            ui->setName (plugin->getName());
//
//        return new PluginWindow (ui, node, type, audioGraph);
//    }
//
//    return nullptr;
//}
//
//PluginWindow::~PluginWindow()
//{
//    activePluginWindows.removeFirstMatchingValue (this);
//
//    if (AudioProcessorEditor* ed = dynamic_cast<AudioProcessorEditor*> (getContentComponent()))
//    {
//        owner->getProcessor()->editorBeingDeleted (ed);
//        ed->setLookAndFeel(nullptr);
//        clearContentComponent();
//    }
//}
//
//void PluginWindow::moved()
//{
//
//}
//
//void PluginWindow::closeButtonPressed()
//{
//
//    delete this;
//}
