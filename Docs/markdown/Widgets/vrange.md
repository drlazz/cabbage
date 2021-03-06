# vrange

Create an on-screen slider with a dragable, and user selectable range. Data can be sent to Csound on the channel specified through the channel string.  


<big></pre>
vrange WIDGET_SYNTAX
</pre></big>

### Specific Identifiers

{! ./markdown/Widgets/Properties/channel_range.md !}   

{! ./markdown/Widgets/Properties/max.md !} 

{! ./markdown/Widgets/Properties/min.md !}  

{! ./markdown/Widgets/Properties/outlinecolour.md !} 

{! ./markdown/Widgets/Properties/range.md !} 

{! ./markdown/Widgets/Properties/text.md !} 

{! ./markdown/Widgets/Properties/textbox.md !} 

{! ./markdown/Widgets/Properties/textcolour.md !} 

{! ./markdown/Widgets/Properties/trackerthickness.md !} 

{! ./markdown/Widgets/Properties/trackercolour.md !} 

### Common Identifiers

{! ./markdown/Widgets/Properties/active.md !} 

{! ./markdown/Widgets/Properties/alpha.md !} 

{! ./markdown/Widgets/Properties/bounds.md !} 



{! ./markdown/Widgets/Properties/colour.md !} 

{! ./markdown/Widgets/Properties/fontcolour.md !}   

{! ./markdown/Widgets/Properties/identchannel.md !} 

{! ./markdown/Widgets/Properties/popup.md !} 

{! ./markdown/Widgets/Properties/rotate.md !} 

{! ./markdown/Widgets/Properties/visible.md !} 

{! ./markdown/Widgets/Properties/tofront.md !} 

{! ./markdown/Widgets/Properties/widgetarray.md !}  
 
<!--(End of identifiers)/-->

>Make sure to use two unique channel names when using hslider2 and vslider2, otherwise min and max will be set to the same value. 


![](../images/vrange.gif)

##Example
<!--(Widget Example)/-->
```csharp
<Cabbage>
form caption("HRange Example") size(400, 300), colour(220, 220, 220), pluginID("def1")
label bounds(8, 6, 368, 20), text("Basic Usage"), fontcolour("black")
groupbox bounds(8, 110, 380, 177), text("Randomly Updated Identifiers")
vrange bounds(166, 34, 20, 70), channel("vrangeL", "vrangeR"), colour(123, 34, 143), range(100, 1000, 200:300, 1, .01)
vrange bounds(110, 140, 62, 162), channel("range1", "range2"), identchannel("widgetIdent"), channel colour(123, 34, 143), corners(40), range(0, 1, .24:.75, 1 0.001)
</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d -+rtmidi=NULL -M0 -m0d 
</CsOptions>
<CsInstruments>
; Initialize the global variables. 
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

seed 0 
;basic usage
instr 1
    aTone1 oscili .25, chnget:k("vrangeL")
    aTone2 oscili .25, chnget:k("vrangeR")
    outs aTone1+aTone2, aTone1+aTone2    
endin

;WIDGET_ADVANCED_USAGE

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f0 z
;starts instrument 1 and runs it for a week
i1 0 z
i2 0 z
</CsScore>
</CsoundSynthesizer> 
```
<!--(End Widget Example)/-->